#ifndef SYMOCRAFT_CHUNK_MANAGER_H
#define SYMOCRAFT_CHUNK_MANAGER_H

#include "core.h"
#include "world.h"
#include "renderer/shader_program.h"


namespace SymoCraft{
    class Block;
    class Frustum;
    class Chunk;
    enum class ChunkState : uint8;

    // A chunk is 16 * 16 * 256
    static constexpr uint16 k_chunk_length = 16;
    static constexpr uint16 k_chunk_width = 16;
    static constexpr uint16 k_chunk_height = 256;

    static constexpr int max_biome_height = 145;
    static constexpr int min_biome_height = 55;
    static constexpr int sea_level = 85;

    namespace ChunkManager
    {
        robin_hood::unordered_node_map<glm::ivec2, Chunk>& GetAllChunks();

        Block GetBlock(const glm::vec3& worldPosition);
        void SetBlock(const glm::vec3& worldPosition, uint16 block_id);
        void RemoveBLock(const glm::vec3& worldPosition);

        Chunk* GetChunk(const glm::vec3& worldPosition);
        Chunk* GetChunk(const glm::ivec2& chunkCoords);

        void RearrangeChunkNeighborPointers();

        void CreateChunk(const glm::ivec2& chunk_coord);
        std::size_t UpdateAllChunks();
        void LoadAllChunks();
        void FreeAllChunks();
    }
}
#endif //SYMOCRAFT_CHUNK_MANAGER_H
