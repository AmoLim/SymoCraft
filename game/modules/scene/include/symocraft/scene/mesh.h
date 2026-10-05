#pragma once
#include "symocraft/foundation/math.h"
#include <span>
#include <type_traits>

namespace SymoCraft {
    struct BlockVertex3D {
        glm::ivec3 pos_coord;
        glm::vec3 tex_coord;
        float normal;
    };
    struct LineVertex3D { glm::vec3 pos_coord; };
    using MeshView = std::span<const BlockVertex3D>;
    static_assert(std::is_standard_layout_v<BlockVertex3D>);
    static_assert(sizeof(BlockVertex3D) == 28);
    static_assert(sizeof(LineVertex3D) == 12);
}
