#pragma once
#include "chunk.h"
#include <array>

namespace SymoCraft::World::Detail {
    enum class MeshBuildStage { BeforeScratch, BeforeOutput, BeforeValidate };
    struct MesherStatistics {
        std::size_t builds{}, scratch_growths{}, scratch_capacity_bytes{}, candidate_bytes{};
        std::size_t peak_mesh_bytes{};
    };
    class ChunkMesher {
    public:
        ChunkMesher() = default;
        ChunkMesher(const ChunkMesher&) = delete;
        ChunkMesher& operator=(const ChunkMesher&) = delete;
        ChunkMesher(ChunkMesher&&) = delete;
        ChunkMesher& operator=(ChunkMesher&&) = delete;
        MeshData Build(const Chunk& chunk, const std::array<const Chunk*, 4>& neighbors,
            const BlockDefinition& definition);
        MesherStatistics statistics;
        void (*probe)(void*, ChunkCoord, MeshBuildStage) = nullptr;
        void* probe_context = nullptr;
        void (*candidate_probe)(void*, MeshData&) = nullptr;
    private:
        struct Face { BlockCoord position; uint16 texture; uint8 direction; };
        std::vector<Face> faces_;
        bool building_ = false;
    };
}
