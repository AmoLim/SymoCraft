#ifndef SYMOCRAFT_CHUNK_MANAGER_H
#define SYMOCRAFT_CHUNK_MANAGER_H

#include <symocraft/world/block.h>
#include <symocraft/world/world.h>
#include <symocraft/scene/mesh.h>
#include <functional>
#include <span>


namespace SymoCraft{

    // A chunk is 16 * 16 * 256
    static constexpr uint16 k_chunk_length = 16;
    static constexpr uint16 k_chunk_width = 16;
    static constexpr uint16 k_chunk_height = 256;

    static constexpr int max_biome_height = 145;
    static constexpr int min_biome_height = 55;
    static constexpr int sea_level = 85;

    namespace ChunkManager
    {
        Block GetBlock(const glm::vec3& worldPosition);
        void SetBlock(const glm::vec3& worldPosition, uint16 block_id);
        bool TrySetBlock(const glm::vec3& worldPosition, uint16 block_id);
        void RemoveBLock(const glm::vec3& worldPosition);

        std::size_t ChunkCount();
        std::size_t UpdateAllChunks();
        // Views are valid only during the callback. Do not mutate the world from it.
        void VisitMeshes(const std::function<void(std::span<const BlockVertex3D>)>& visitor);
        void FreeAllChunks();
    }
}
#endif //SYMOCRAFT_CHUNK_MANAGER_H
