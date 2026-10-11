#pragma once
#include "world_internal.h"
#include "symocraft/world/constants.h"
#include <algorithm>
#include <stdexcept>

namespace SymoCraft::World {
    // Only test executables include this white-box adapter; production exposes no setter.
    struct WorldTestAccess {
        static Detail::Chunk& Chunk(VoxelWorld& world, ChunkCoord coordinate)
        {
            auto* chunk = world.impl_->Lookup(coordinate);
            if (!chunk) throw std::runtime_error("Test chunk outside world");
            return *chunk;
        }
        static const auto& Chunks(const VoxelWorld& world) { return world.impl_->chunks; }
        static auto Statistics(const VoxelWorld& world) { return world.impl_->mesher.statistics; }
        static void Probe(VoxelWorld& world, void (*callback)(void*, ChunkCoord, Detail::MeshBuildStage), void* context)
        {
            world.impl_->mesher.probe = callback;
            world.impl_->mesher.probe_context = context;
        }
        static void CandidateProbe(VoxelWorld& world, void (*callback)(void*, MeshData&))
        { world.impl_->mesher.candidate_probe = callback; }
        static void ResetToAir(VoxelWorld& world)
        {
            world.impl_->EnsureMutable();
            auto air = BlockConstants::AIR_BLOCK;
            air.SetTransparency(true);
            air.SetBlendability(false);
            air.SetLightSource(false);
            for (const auto& chunk : world.impl_->chunks) {
                std::fill(chunk->blocks.begin(), chunk->blocks.end(), air);
                chunk->mesh.vertices.clear();
                chunk->content_revision = chunk->mesh_input_revision = 1;
                chunk->published_mesh_revision.reset();
            }
        }
        static Block& RawBlock(VoxelWorld& world, BlockCoord coordinate)
        {
            const auto divide = [](int value) { return value / 16 - (value % 16 < 0 ? 1 : 0); };
            const ChunkCoord chunk_coordinate{divide(coordinate.x), divide(coordinate.z)};
            auto& chunk = Chunk(world, chunk_coordinate);
            return chunk.blocks[Detail::Chunk::Index(coordinate.x - chunk_coordinate.x * 16,
                coordinate.y, coordinate.z - chunk_coordinate.y * 16)];
        }
    };
}
