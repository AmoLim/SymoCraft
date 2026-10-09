#pragma once
#include <symocraft/world/world.h>
#include <vector>

namespace SymoCraft::World::Detail {
    class Chunk {
    public:
        explicit Chunk(ChunkCoord position, bool edge);
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk(Chunk&&) = delete;
        Chunk& operator=(Chunk&&) = delete;
        const ChunkCoord coordinate;
        const bool fringe;
        std::vector<Block> blocks;
        MeshData mesh;
        Revision content_revision = 1;
        Revision mesh_input_revision = 1;
        std::optional<Revision> published_mesh_revision;
        static std::size_t Index(int x, int y, int z) noexcept
        { return static_cast<std::size_t>((y * 16 + x) * 16 + z); }
    };
}
