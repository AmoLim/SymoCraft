#include "symocraft/world/generation.h"
#include "symocraft/world/test_scene.h"
#if defined(SYMOCRAFT_T1_OLD)
#include "chunk.h"
#include "chunk_store.h"
#include "symocraft/world/chunk_manager.h"
#else
#include "symocraft/world/world.h"
#include "symocraft/world/block_definition.h"
#include "world_test_access.h"
#endif
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <malloc.h>
#include <new>
#include <sstream>
#include <span>
#include <stdexcept>

namespace {
    struct AllocationCounter { std::size_t count{}, requested_bytes{}; };
    thread_local AllocationCounter* counter = nullptr;
    void Count(std::size_t bytes) noexcept
    {
        if (counter) { ++counter->count; counter->requested_bytes += bytes; }
    }
    struct Measurement {
        AllocationCounter allocations;
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        Measurement() { counter = &allocations; }
        double Finish() { counter = nullptr; return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin).count(); }
        ~Measurement() { counter = nullptr; }
    };
    std::string ReadText(const char* path)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("Cannot read block definition");
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }
    std::uint64_t HashBytes(const void* data, std::size_t bytes)
    {
        std::uint64_t hash = 14695981039346656037ull;
        for (std::size_t i = 0; i < bytes; ++i) { hash ^= static_cast<const unsigned char*>(data)[i]; hash *= 1099511628211ull; }
        return hash;
    }
    void MeshRecord(int x, int z, std::span<const SymoCraft::BlockVertex3D> mesh, bool& first)
    {
        if (!first) std::cout << ',';
        first = false;
        std::cout << "{\"x\":" << x << ",\"z\":" << z << ",\"vertices\":" << mesh.size()
            << ",\"fnv1a64\":\"" << std::hex << HashBytes(mesh.data(), mesh.size_bytes()) << std::dec << "\"}";
    }
}

void* operator new(std::size_t bytes)
{
    Count(bytes);
    if (void* result = std::malloc(bytes == 0 ? 1 : bytes)) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* pointer) noexcept { std::free(pointer); }
void operator delete[](void* pointer) noexcept { std::free(pointer); }
void operator delete(void* pointer, std::size_t) noexcept { std::free(pointer); }
void operator delete[](void* pointer, std::size_t) noexcept { std::free(pointer); }
void* operator new(std::size_t bytes, std::align_val_t alignment)
{
    Count(bytes);
    if (void* result = _aligned_malloc(bytes == 0 ? 1 : bytes, static_cast<std::size_t>(alignment))) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t bytes, std::align_val_t alignment) { return ::operator new(bytes, alignment); }
void operator delete(void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete(void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }
void operator delete[](void* pointer, std::size_t, std::align_val_t) noexcept { _aligned_free(pointer); }

int main(int argc, char** argv)
{
    using namespace SymoCraft;
    try {
        if (argc != 2) throw std::runtime_error("Expected real block configuration path");
#if defined(SYMOCRAFT_T1_OLD)
        LoadBlocks(argv[1]);
#else
        const auto definition = World::BlockDefinition::FromConfig(ReadText(argv[1]));
#endif
        std::cout << std::setprecision(17) << "{\"protocol\":\"world-cost-v1\",\"allocation_scope\":\"C++ operator new/new[] requests; excludes C malloc/calloc and OS allocations\",\"runs\":[";
        bool first_run = true;
        const std::array<Generation::Settings,7> configurations{{{424242,2,true},{424242,2,false},
            {424242,3,true},{424242,3,false},{424242,10,true},{424242,10,false},{424243,3,true}}};
        for (const auto settings : configurations) for (int repetition = 1; repetition <= 3; ++repetition) {
            const auto radius = settings.radius;
            Generation::Timings stages;
            Measurement generation;
#if defined(SYMOCRAFT_T1_OLD)
            Generation::Build(settings, &stages);
#else
            auto world = World::VoxelWorld::Create(definition, settings, &stages);
#endif
            const double generation_ms = generation.Finish();
            Measurement meshing;
#if defined(SYMOCRAFT_T1_OLD)
            const auto rebuilt = ChunkManager::UpdateAllChunks();
#else
            const auto rebuilt = world->RebuildDirtyMeshes();
#endif
            const double mesh_ms = meshing.Finish();
            if (!first_run) std::cout << ',';
            first_run = false;
#if defined(SYMOCRAFT_T1_OLD)
            const auto digest = Generation::BlockDigest();
            const auto spawn = Generation::FindSpawnPosition();
#else
            const auto digest = world->Digest();
            const auto spawn = world->FindSpawn();
#endif
            std::cout << "{\"seed\":" << settings.seed << ",\"vegetation\":" << (settings.vegetation ? "true" : "false") << ",\"radius\":" << radius << ",\"repetition\":" << repetition
                << ",\"digest\":\"" << digest << "\",\"spawn\":[" << spawn.x << ',' << spawn.y << ',' << spawn.z << ']'
                << ",\"generation_ms\":" << generation_ms << ",\"allocation_ms\":" << stages.allocation_ms
                << ",\"terrain_ms\":" << stages.terrain_ms << ",\"vegetation_ms\":" << stages.vegetation_ms
                << ",\"generation_new_count\":" << generation.allocations.count << ",\"generation_new_requested_bytes\":" << generation.allocations.requested_bytes
                << ",\"mesh_ms\":" << mesh_ms << ",\"mesh_new_count\":" << meshing.allocations.count
                << ",\"mesh_new_requested_bytes\":" << meshing.allocations.requested_bytes << ",\"rebuilt\":" << rebuilt;
#if !defined(SYMOCRAFT_T1_OLD)
            const auto statistics = World::WorldTestAccess::Statistics(*world);
            std::cout << ",\"scratch_capacity_bytes\":" << statistics.scratch_capacity_bytes
                << ",\"candidate_bytes_last_build\":" << statistics.candidate_bytes
                << ",\"peak_single_build_scratch_candidate_old_mesh_bytes\":" << statistics.peak_mesh_bytes;
#else
            std::cout << ",\"legacy_fixed_geometry_scratch_bytes\":"
                << sizeof(float) + sizeof(std::array<std::array<BlockVertex3D,4>,6>);
#endif
            std::size_t mesh_capacity = 0, block_capacity = 0, max_mesh_capacity = 0;
#if defined(SYMOCRAFT_T1_OLD)
            for (const auto& [coordinate, chunk] : ChunkManager::GetAllChunks()) {
                const auto bytes = chunk.m_vertex_data.capacity() * sizeof(BlockVertex3D);
                mesh_capacity += bytes; max_mesh_capacity = std::max(max_mesh_capacity,bytes);
                block_capacity += chunk.m_local_blocks.capacity() * sizeof(Block);
            }
#else
            for (const auto& chunk : World::WorldTestAccess::Chunks(*world)) {
                const auto bytes = chunk->mesh.vertices.capacity() * sizeof(BlockVertex3D);
                mesh_capacity += bytes; max_mesh_capacity = std::max(max_mesh_capacity,bytes);
                block_capacity += chunk->blocks.capacity() * sizeof(Block);
            }
#endif
            std::cout << ",\"retained_world_mesh_capacity_bytes\":" << mesh_capacity
                << ",\"retained_world_block_capacity_bytes\":" << block_capacity
                << ",\"max_retained_chunk_mesh_capacity_bytes\":" << max_mesh_capacity;
            std::cout << ",\"meshes\":[";
            bool first_mesh = true;
#if defined(SYMOCRAFT_T1_OLD)
            for (int x = -radius + 1; x < radius; ++x) for (int z = -radius + 1; z < radius; ++z)
                MeshRecord(x, z, ChunkManager::GetChunk(glm::ivec2{x,z})->m_vertex_data, first_mesh);
#else
            world->VisitMeshes([&](const World::WorldMeshRecord& record) { MeshRecord(record.identity.chunk.x, record.identity.chunk.y, record.vertices, first_mesh); });
#endif
            std::cout << "]}";
#if defined(SYMOCRAFT_T1_OLD)
            ChunkManager::FreeAllChunks();
#endif
        }
        std::cout << "]}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
