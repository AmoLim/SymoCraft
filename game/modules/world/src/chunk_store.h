#pragma once

#include "chunk.h"
#include <robin_hood.h>

namespace SymoCraft::ChunkManager {
    robin_hood::unordered_node_map<glm::ivec2, Chunk>& GetAllChunks();
    Chunk* GetChunk(const glm::vec3& world_position);
    Chunk* GetChunk(const glm::ivec2& coordinates);
    void RearrangeChunkNeighborPointers();
    void CreateChunk(const glm::ivec2& coordinates);
}
