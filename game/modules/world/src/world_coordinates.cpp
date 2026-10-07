#include <symocraft/world/world.h>
#include <cmath>
#include <limits>

namespace SymoCraft::World {
    std::optional<BlockCoord> TryToBlockCoord(const glm::vec3& position) noexcept
    {
        BlockCoord result;
        for (int axis = 0; axis < 3; ++axis) {
            const double value = std::floor(static_cast<double>(position[axis]));
            if (!std::isfinite(value) || value < std::numeric_limits<int32>::min() || value > std::numeric_limits<int32>::max())
                return std::nullopt;
            result[axis] = static_cast<int32>(value);
        }
        return result;
    }
}
