#include "world_internal.h"
#include <symocraft/world/constants.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace SymoCraft::World {
    namespace {
        int FloorDiv16(int value) noexcept { return value / 16 - (value % 16 < 0 ? 1 : 0); }
        WorldId NextIdentity()
        {
            static std::atomic<WorldId> next{1};
            auto value = next.load(std::memory_order_relaxed);
            do {
                if (value == std::numeric_limits<WorldId>::max())
                    throw std::overflow_error("WorldIdExhausted");
            } while (!next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed));
            return value;
        }
    }
    VoxelWorld::Impl::Impl(BlockDefinition rules, Generation::Settings config, WorldId id)
        : definition(std::move(rules)), settings(config), identity(id) {}
    Detail::Chunk* VoxelWorld::Impl::Lookup(ChunkCoord coordinate) const noexcept
    {
        const int r = settings.radius;
        if (coordinate.x < -r || coordinate.x > r || coordinate.y < -r || coordinate.y > r) return nullptr;
        return chunks[static_cast<std::size_t>((coordinate.x + r) * (r * 2 + 1) + coordinate.y + r)].get();
    }
    Block VoxelWorld::Impl::Read(BlockCoord position) const noexcept
    {
        if (position.y < 0 || position.y >= 256) return BlockConstants::NULL_BLOCK;
        const ChunkCoord coordinate{FloorDiv16(position.x), FloorDiv16(position.z)};
        const auto* chunk = Lookup(coordinate);
        if (!chunk) return BlockConstants::NULL_BLOCK;
        return chunk->blocks[Detail::Chunk::Index(position.x - coordinate.x * 16, position.y, position.z - coordinate.y * 16)];
    }
    void VoxelWorld::Impl::EnsureMutable() const
    {
        if (visiting || rebuilding) throw std::logic_error("World operation rejected during mesh visit/build");
    }
    VoxelWorld::VoxelWorld(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}
    VoxelWorld::~VoxelWorld() = default;
    std::unique_ptr<VoxelWorld> VoxelWorld::Create(BlockDefinition definition,
        Generation::Settings settings, Generation::Timings* timings)
    {
        const Generation::Generator generator(settings);
        for (BlockId id = 1; id <= 11; ++id)
            if (!definition.Find(id)) throw std::invalid_argument("World requires a complete block definition");
        auto impl = std::make_unique<Impl>(std::move(definition), settings, NextIdentity());
        const auto start = std::chrono::steady_clock::now();
        const int radius = settings.radius;
        const auto side = static_cast<std::size_t>(radius * 2 + 1);
        impl->chunks.reserve(side * side);
        for (int x = -radius; x <= radius; ++x)
            for (int z = -radius; z <= radius; ++z)
                impl->chunks.push_back(std::make_unique<Detail::Chunk>(ChunkCoord{x, z},
                    x == -radius || x == radius || z == -radius || z == radius));
        if (timings) timings->allocation_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        impl->Populate(generator, timings);
        return std::unique_ptr<VoxelWorld>(new VoxelWorld(std::move(impl)));
    }
    BlockQueryResult VoxelWorld::QueryBlock(BlockCoord position) const noexcept
    {
        const auto block = impl_->Read(position);
        return {block.block_id == 0 ? BlockQueryStatus::OutsideWorld : BlockQueryStatus::Found, block};
    }
    std::optional<BlockDefinitionEntry> VoxelWorld::DescribeBlock(BlockId id) const { return impl_->definition.Find(id); }
    std::optional<BlockId> VoxelWorld::FindBlockId(std::string_view name) const { return impl_->definition.FindId(name); }
    void VoxelWorld::ValidateTextureLayers(std::size_t count) const { impl_->definition.ValidateTextureLayers(count); }
    std::size_t VoxelWorld::ChunkCount() const noexcept { return impl_->chunks.size(); }
    WorldId VoxelWorld::Identity() const noexcept { return impl_->identity; }
    EditResult VoxelWorld::TryEdit(EditRequest request)
    {
        impl_->EnsureMutable();
        if (request.operation != EditOperation::Set && request.operation != EditOperation::Remove)
            throw std::invalid_argument("Unknown edit operation");
        const auto position = request.position;
        if (position.y < 0 || position.y >= 256) return {EditStatus::Rejected, EditRejection::OutsideWorld};
        const ChunkCoord coordinate{FloorDiv16(position.x), FloorDiv16(position.z)};
        auto* chunk = impl_->Lookup(coordinate);
        if (!chunk) return {EditStatus::Rejected, EditRejection::OutsideWorld};
        const auto id = request.operation == EditOperation::Remove ? BlockId{1} : request.id;
        const auto rule = impl_->definition.Find(id);
        if (!rule) return {EditStatus::Rejected, EditRejection::UnknownBlockId};
        const int x = position.x - coordinate.x * 16, z = position.z - coordinate.y * 16;
        auto& current = chunk->blocks[Detail::Chunk::Index(x, position.y, z)];
        Block candidate = current;
        candidate.block_id = id;
        candidate.SetTransparency(request.operation == EditOperation::Remove ? true : rule->m_is_transparent);
        candidate.SetBlendability(request.operation == EditOperation::Remove ? false : rule->m_is_blendable);
        candidate.SetLightSource(request.operation == EditOperation::Remove ? false : rule->m_is_lightSource);
        if (candidate.block_id == current.block_id && candidate.lightLevel == current.lightLevel &&
            candidate.lightColor == current.lightColor && candidate.bitwise_compressed_data == current.bitwise_compressed_data)
            return {EditStatus::Unchanged};
        std::array<Detail::Chunk*, 3> affected{chunk};
        std::size_t count = 1;
        if (x == 0 || x == 15)
            if (auto* neighbor = impl_->Lookup(coordinate + ChunkCoord{x == 0 ? -1 : 1, 0})) affected[count++] = neighbor;
        if (z == 0 || z == 15)
            if (auto* neighbor = impl_->Lookup(coordinate + ChunkCoord{0, z == 0 ? -1 : 1})) affected[count++] = neighbor;
        if (chunk->content_revision == std::numeric_limits<Revision>::max())
            throw std::overflow_error("RevisionExhausted: TryEdit content at chunk " +
                std::to_string(coordinate.x) + "," + std::to_string(coordinate.y));
        for (std::size_t i = 0; i < count; ++i)
            if (affected[i]->mesh_input_revision == std::numeric_limits<Revision>::max())
                throw std::overflow_error("RevisionExhausted: TryEdit mesh input at chunk " +
                    std::to_string(affected[i]->coordinate.x) + "," + std::to_string(affected[i]->coordinate.y));
        current = candidate;
        ++chunk->content_revision;
        for (std::size_t i = 0; i < count; ++i) ++affected[i]->mesh_input_revision;
        return {EditStatus::Changed};
    }
    std::size_t VoxelWorld::RebuildDirtyMeshes()
    {
        impl_->EnsureMutable();
        impl_->rebuilding = true;
        struct Restore { bool& state; ~Restore() { state = false; } } restore{impl_->rebuilding};
        std::size_t published = 0;
        for (const auto& owner : impl_->chunks) {
            auto& chunk = *owner;
            if (chunk.fringe || chunk.published_mesh_revision == chunk.mesh_input_revision) continue;
            const auto c = chunk.coordinate;
            const std::array<const Detail::Chunk*, 4> neighbors{{impl_->Lookup(c + ChunkCoord{1,0}),
                impl_->Lookup(c + ChunkCoord{-1,0}), impl_->Lookup(c + ChunkCoord{0,1}), impl_->Lookup(c + ChunkCoord{0,-1})}};
            MeshData candidate;
            try {
                candidate = impl_->mesher.Build(chunk, neighbors, impl_->definition);
            } catch (const std::bad_alloc&) {
                throw;
            } catch (const std::exception& error) {
                throw std::runtime_error("ChunkMesher build at " + std::to_string(c.x) + "," +
                    std::to_string(c.y) + ": " + error.what());
            }
            if (candidate.vertices.size() % 6 || candidate.vertices.size() >
                std::numeric_limits<std::size_t>::max() / sizeof(BlockVertex3D))
                throw std::length_error("Invalid mesh size at chunk " + std::to_string(c.x) + "," + std::to_string(c.y));
            chunk.mesh.vertices.swap(candidate.vertices);
            chunk.published_mesh_revision = chunk.mesh_input_revision;
            ++published;
        }
        return published;
    }
    void VoxelWorld::VisitMeshes(const std::function<void(const WorldMeshRecord&)>& visitor) const
    {
        if (impl_->visiting || impl_->rebuilding) throw std::logic_error("World mesh visit cannot be re-entered");
        impl_->visiting = true;
        struct Restore { bool& state; ~Restore() { state = false; } } restore{impl_->visiting};
        for (const auto& chunk : impl_->chunks)
            if (!chunk->fringe && chunk->published_mesh_revision)
                visitor({{impl_->identity, chunk->coordinate}, *chunk->published_mesh_revision,
                    chunk->mesh_input_revision, chunk->mesh.vertices});
    }
    std::string VoxelWorld::Digest() const
    {
        std::uint64_t hash = 14695981039346656037ull;
        const auto append = [&hash](std::uint32_t value, unsigned bytes) {
            for (unsigned i = 0; i < bytes; ++i) { hash ^= (value >> (i * 8)) & 0xffu; hash *= 1099511628211ull; }
        };
        for (const auto& chunk : impl_->chunks) {
            append(static_cast<std::uint32_t>(chunk->coordinate.x), 4);
            append(static_cast<std::uint32_t>(chunk->coordinate.y), 4);
            for (const auto& block : chunk->blocks) {
                append(block.block_id, 2); append(block.lightLevel, 2);
                append(static_cast<uint16>(block.lightColor), 2); append(block.bitwise_compressed_data, 2);
            }
        }
        std::ostringstream out;
        out << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << hash;
        return out.str();
    }
    glm::vec3 VoxelWorld::FindSpawn() const
    {
        for (int radius = 0; radius <= 64; radius += 4)
            for (int x = -radius; x <= radius; x += 4)
                for (int z = -radius; z <= radius; z += 4) {
                    if (std::max(std::abs(x), std::abs(z)) != radius) continue;
                    for (int y = k_chunk_height - 4; y >= 0; --y) {
                        const auto block = impl_->Read({x,y,z});
                        if (block.block_id == 9) break;
                        const auto rule = impl_->definition.Find(block.block_id);
                        if (!rule || !rule->m_is_solid) continue;
                        if (block.block_id >= 2 && block.block_id <= 5) return {x + 0.5f, y + 1.95f, z + 0.5f};
                        break;
                    }
                }
        throw std::runtime_error("Cannot find a safe player spawn near the world center");
    }
}
