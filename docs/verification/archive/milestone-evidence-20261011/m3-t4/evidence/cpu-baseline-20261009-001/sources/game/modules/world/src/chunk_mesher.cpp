#include "chunk_mesher.h"
#include <symocraft/world/constants.h>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace SymoCraft::World::Detail {
    MeshData ChunkMesher::Build(const Chunk& chunk, const std::array<const Chunk*, 4>& neighbors,
        const BlockDefinition& definition)
    {
        if (building_) throw std::logic_error("ChunkMesher Build cannot be re-entered");
        building_ = true;
        struct Restore {
            bool& active;
            std::vector<Face>& faces;
            MesherStatistics& statistics;
            std::size_t starting_capacity;
            ~Restore() {
                statistics.scratch_growths += faces.capacity() != starting_capacity;
                statistics.scratch_capacity_bytes = faces.capacity() * sizeof(Face);
                statistics.peak_mesh_bytes = std::max(statistics.peak_mesh_bytes, statistics.scratch_capacity_bytes);
                faces.clear(); active = false;
            }
        } restore{building_, faces_, statistics, faces_.capacity()};
        if (probe) probe(probe_context, chunk.coordinate, MeshBuildStage::BeforeScratch);
        // Build-local neighboring chunks are borrowed only for this synchronous call.
        const auto read = [&](int x, int y, int z) {
            if (y < 0 || y >= 256) return BlockConstants::NULL_BLOCK;
            const Chunk* target = &chunk;
            if (x >= 16) { target = neighbors[0]; x -= 16; }
            else if (x < 0) { target = neighbors[1]; x += 16; }
            else if (z >= 16) { target = neighbors[2]; z -= 16; }
            else if (z < 0) { target = neighbors[3]; z += 16; }
            return target ? target->blocks[Chunk::Index(x, y, z)] : BlockConstants::NULL_BLOCK;
        };
        constexpr std::array<glm::ivec3, 6> directions{{{1,0,0},{0,0,1},{-1,0,0},{0,0,-1},{0,1,0},{0,-1,0}}};
        for (int y = 0; y < 256; ++y)
            for (int x = 0; x < 16; ++x)
                for (int z = 0; z < 16; ++z) {
                    const auto& block = chunk.blocks[Chunk::Index(x, y, z)];
                    if (block.block_id == 0 || block.block_id == 1) continue;
                    const auto rule = definition.Find(block.block_id);
                    if (!rule) throw std::logic_error("Mesh input has unknown block definition");
                    for (uint8 face = 0; face < 6; ++face) {
                        const auto direction = directions[face];
                        const auto neighbor = read(x + direction.x, y + direction.y, z + direction.z);
                        if (neighbor.block_id == 0 || !neighbor.IsTransparent()) continue;
                        const uint16 texture = face < 4 ? rule->m_side_texture : face == 4 ? rule->m_top_texture : rule->m_bottom_texture;
                        faces_.push_back({{x + chunk.coordinate.x * 16, y, z + chunk.coordinate.y * 16}, texture, face});
                    }
                }
        statistics.scratch_capacity_bytes = faces_.capacity() * sizeof(Face);
        if (faces_.size() > std::numeric_limits<std::size_t>::max() / (6 * sizeof(BlockVertex3D)))
            throw std::length_error("Chunk mesh vertex byte count overflow");
        if (probe) probe(probe_context, chunk.coordinate, MeshBuildStage::BeforeOutput);
        MeshData candidate;
        candidate.vertices.reserve(faces_.size() * 6);
        statistics.candidate_bytes = candidate.vertices.capacity() * sizeof(BlockVertex3D);
        statistics.peak_mesh_bytes = std::max(statistics.peak_mesh_bytes,
            statistics.scratch_capacity_bytes + statistics.candidate_bytes + chunk.mesh.vertices.capacity() * sizeof(BlockVertex3D));
        for (const auto& face : faces_) {
            std::array<BlockVertex3D, 4> corners;
            for (int i = 0; i < 4; ++i)
                corners[i] = {face.position + BlockConstants::pos_coords[BlockConstants::vertex_indices[face.direction * 4 + i]],
                    {BlockConstants::tex_coords[i], static_cast<float>(face.texture)}, 0.0f};
            candidate.vertices.insert(candidate.vertices.end(), {corners[0],corners[1],corners[2],corners[0],corners[2],corners[3]});
        }
        if (candidate_probe) candidate_probe(probe_context, candidate);
        if (probe) probe(probe_context, chunk.coordinate, MeshBuildStage::BeforeValidate);
        ++statistics.builds;
        return candidate;
    }
}
