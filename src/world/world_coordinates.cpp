#include "world/world.h"

namespace SymoCraft::World {
    glm::ivec2 ToChunkCoords(const glm::vec3& worldCoordinates)
    {
        return {glm::floor(worldCoordinates.x / 16.0f), glm::floor(worldCoordinates.z / 16.0f)};
    }
}
