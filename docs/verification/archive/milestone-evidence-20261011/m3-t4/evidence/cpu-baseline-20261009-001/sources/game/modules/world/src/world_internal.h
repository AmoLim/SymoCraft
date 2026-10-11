#pragma once
#include <symocraft/world/world.h>
#include "chunk_mesher.h"
#include <vector>

namespace SymoCraft::World {
    struct VoxelWorld::Impl {
        Impl(BlockDefinition definition, Generation::Settings settings, WorldId identity);
        BlockDefinition definition;
        const Generation::Settings settings;
        const WorldId identity;
        std::vector<std::unique_ptr<Detail::Chunk>> chunks;
        Detail::ChunkMesher mesher;
        std::array<int, 3> noise_seeds{};
        mutable bool visiting = false;
        bool rebuilding = false;
        Detail::Chunk* Lookup(ChunkCoord coordinate) const noexcept;
        Block Read(BlockCoord position) const noexcept;
        void Populate(const Generation::Generator& generator, Generation::Timings* timings);
        void WriteGenerated(BlockCoord position, BlockId id);
        void EnsureMutable() const;
    };
}
