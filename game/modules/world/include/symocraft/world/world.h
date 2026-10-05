#ifndef SYMOCRAFT_WORLD_H
#define SYMOCRAFT_WORLD_H

#include <symocraft/foundation/types.h>
#include <symocraft/foundation/math.h>

namespace SymoCraft
{
    namespace World{
        inline constexpr uint16 chunk_radius = 10;
        inline constexpr uint16 max_vertices_per_chunk = UINT16_MAX;
        glm::ivec2 ToChunkCoords(const glm::vec3& worldCoordinates);
    }
}

#endif //SYMOCRAFT_WORLD_H
