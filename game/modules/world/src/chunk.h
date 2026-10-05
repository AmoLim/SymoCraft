#ifndef SYMOCRAFT_CHUNK_H
#define SYMOCRAFT_CHUNK_H

#include <symocraft/world/block.h>
#include <symocraft/world/chunk_manager.h>
#include <symocraft/scene/mesh.h>
#include <vector>

namespace SymoCraft {

    enum class ChunkState : uint8
    {
        None,
        ToBeUpdated,
        Updated,
    };

    namespace Generation { class Generator; }

    class Chunk {
    public:
        Chunk();
        Chunk(const Chunk&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk(Chunk&&) noexcept = default;
        Chunk& operator=(Chunk&&) noexcept = default;

        std::vector<Block> m_local_blocks;
        std::vector<BlockVertex3D> m_vertex_data;
        glm::ivec2 m_chunk_coord{};
        ChunkState state{ChunkState::ToBeUpdated};

        Chunk* front_neighbor{};
        Chunk* back_neighbor{};
        Chunk* left_neighbor{};
        Chunk* right_neighbor{};

        bool m_is_fringe_chunk{false};


        inline bool operator==(const Chunk &other) const {
            return m_chunk_coord == other.m_chunk_coord;
        }

        inline bool operator!=(const Chunk &other) const {
            return m_chunk_coord != other.m_chunk_coord;
        }

        inline bool operator==(const glm::ivec2 &other) const {
            return m_chunk_coord == other;
        }

        inline bool operator!=(const glm::ivec2 &other) const {
            return m_chunk_coord != other;
        }

        struct HashFunction {
            inline std::size_t operator()(const Chunk &key) const {
                return std::hash<int>()(key.m_chunk_coord.x) ^
                       std::hash<int>()(key.m_chunk_coord.y);
            }
        };

        Block GetWorldBlock(const glm::vec3 &world_coord);
        bool SetWorldBlock(const glm::vec3 &world_coord, uint16 block_id);
        bool RemoveWorldBlock(const glm::vec3 &world_coord);

        void GenerateTerrain(const Generation::Generator& generator);
        void GenerateVegetation(const Generation::Generator& generator);
        void GenerateRenderData();
        void Free();
        std::size_t VertexCount() const noexcept { return m_vertex_data.size(); }
        void UpdateChunkLocalBlocks(const glm::vec3& block_world_coord);

    private:
        Block GetLocalBlock(int x, int y, int z);
        bool SetLocalBlock(int x, int y, int z, uint16 block_id);
        bool RemoveLocalBlock(int x, int y, int z);

        inline int GetLocalBlockIndex(int x, int y ,int z)
        {
            return (y * k_chunk_length + x) * k_chunk_width + z;
        }

    };
}



#endif //SYMOCRAFT_CHUNK_H
