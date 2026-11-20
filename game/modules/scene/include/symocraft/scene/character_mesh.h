#pragma once

#include <symocraft/foundation/math.h>
#include <string>
#include <vector>

namespace SymoCraft {
    struct CharacterVertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec3 color;
    };
    // Geometry remains immutable; a pose changes only the per-part transform.
    // parent indexes the containing model's parts, or -1 for a root part.
    struct CharacterPart {
        std::string name;
        int parent{-1};
        glm::mat4 local_transform{1};
        glm::vec3 pivot{};
        glm::vec3 rotation_axis{1, 0, 0};
        std::vector<CharacterVertex> vertices;
    };
    struct CharacterMeshData {
        // Complete bind-pose triangles, retained for static consumers.
        std::vector<CharacterVertex> vertices;
        // Empty for legacy whole-body static inputs.
        std::vector<CharacterPart> parts;
    };
    static_assert(sizeof(CharacterVertex) == 36);
}
