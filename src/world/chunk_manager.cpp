#include "world/chunk_manager.h"
#include "world/chunk.h"
#include "core/constants.h"
#include "renderer/renderer.h"

namespace SymoCraft{

    static robin_hood::unordered_node_map<glm::ivec2, Chunk> chunks;

    namespace ChunkManager {
        Block GetBlock(const glm::vec3 &worldPosition) {
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk)
                return BlockConstants::NULL_BLOCK;

            return chunk->GetWorldBlock(worldPosition);
        }

        void SetBlock(const glm::vec3 &worldPosition, uint16 block_id) {
            glm::ivec2 chunkCoords = World::ToChunkCoords(worldPosition);
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk) {
                if (worldPosition.y >= 0 && worldPosition.y < 256) {
                    // Assume it's a chunk that's out of bounds
                    AmoLogger_Warning("Tried to set invalid block at position<%2.3f, %2.3f, %2.3f>!", worldPosition.x,
                                      worldPosition.y, worldPosition.z);
                }
                return;
            }

            chunk->SetWorldBlock(worldPosition, block_id);
        }

        void RemoveBLock(const glm::vec3 &worldPosition) {
            Chunk *chunk = GetChunk(worldPosition);

            if (!chunk) {
                if (worldPosition.y >= 0 && worldPosition.y < 256) {
                    // Assume it's a chunk that's out of bounds
                    AmoLogger_Warning("Tried to set invalid block at position<%2.3f, %2.3f, %2.3f>!", worldPosition.x,
                                      worldPosition.y, worldPosition.z);
                }
                return;
            }

            chunk->RemoveWorldBlock(worldPosition);
        }

        Chunk *GetChunk(const glm::vec3 &worldPosition)
        {
            glm::ivec2 chunkCoords = World::ToChunkCoords(worldPosition);
            return GetChunk(chunkCoords);
        }

        Chunk* GetChunk(const glm::ivec2& chunkCoords)
        {
            const auto iter = chunks.find(chunkCoords);
            if (iter != chunks.end())
                return &iter->second;
            else
                return nullptr;
        }

        robin_hood::unordered_node_map<glm::ivec2, Chunk>& GetAllChunks()
        {
            return chunks;
        }

        void CreateChunk(const glm::ivec2 &chunk_coord)
        {
            auto [iter, inserted] = chunks.try_emplace(chunk_coord);
            if (inserted)
            {
                Chunk& chunk = iter->second;
                chunk.m_chunk_coord = chunk_coord;
                chunk.m_draw_command.instanceCount = 1;
                chunk.front_neighbor = GetChunk(chunk_coord + INormals2::Front);
                chunk.back_neighbor = GetChunk(chunk_coord + INormals2::Back);
                chunk.left_neighbor = GetChunk(chunk_coord + INormals2::Left);
                chunk.right_neighbor = GetChunk(chunk_coord + INormals2::Right);
            }
        }

        void RearrangeChunkNeighborPointers()
        {
            for (auto& pair : chunks)
            {
                Chunk& chunk = pair.second;
                auto iter1 = chunks.find(chunk.m_chunk_coord + INormals2::Front);
                chunk.front_neighbor = iter1 == chunks.end() ? nullptr : &iter1->second;
                auto iter2 = chunks.find(chunk.m_chunk_coord + INormals2::Back);
                chunk.back_neighbor = iter2 == chunks.end() ? nullptr : &iter2->second;
                auto iter3 = chunks.find(chunk.m_chunk_coord + INormals2::Left);
                chunk.left_neighbor = iter3 == chunks.end() ? nullptr : &iter3->second;
                auto iter4 = chunks.find(chunk.m_chunk_coord + INormals2::Right);
                chunk.right_neighbor = iter4 == chunks.end() ? nullptr : &iter4->second;

                chunk.m_is_fringe_chunk = chunk.front_neighbor == nullptr || chunk.back_neighbor == nullptr ||
                    chunk.left_neighbor == nullptr || chunk.right_neighbor == nullptr;
            }
        }

        std::size_t UpdateAllChunks()
        {
            std::size_t rebuilt = 0;
            for(auto &pair : chunks)
                if (pair.second.state == ChunkState::Updated || pair.second.m_is_fringe_chunk)
                    continue;
                else if(pair.second.state == ChunkState::ToBeUpdated) {
                    pair.second.GenerateRenderData();
                    ++rebuilt;
                }
                else
                    AmoLogger_Info("Unknown state of chunk updated\n");
            return rebuilt;
        }

        void LoadAllChunks()
        {
            for(auto &pair : chunks)
                if(pair.second.m_vertex_data.empty() || pair.second.m_is_fringe_chunk)
                    continue;
                else
                    chunk_batch.AddVertex(pair.second.m_vertex_data.data(), pair.second.VertexCount());
        }

        void FreeAllChunks()
        {
            chunks.clear();
        }
    }
}
