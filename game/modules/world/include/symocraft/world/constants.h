#ifndef MAIN_CPP_CONSTANTS_H
#define MAIN_CPP_CONSTANTS_H
#include <symocraft/world/block.h>
#include <array>

namespace SymoCraft
{
    namespace INormals2
    {
        inline constexpr glm::ivec2 Front = glm::ivec2(1, 0);
        inline constexpr glm::ivec2 Back = glm::ivec2(-1, 0);
        inline constexpr glm::ivec2 Left = glm::ivec2(0, -1);
        inline constexpr glm::ivec2 Right = glm::ivec2(0, 1);
        inline constexpr std::array<glm::ivec2, 4> CardinalDirections = { Front, Back, Left, Right };
    }

    namespace BlockConstants
    {
        // The 8 vertices will look like this:
        //   v4 ----------- v7     v0 is at (0, 0, 0)
        //   /|            /|
        //  / |           / |      Axis orientation
        // v5 --------- v6  |            y
        // |  |         |   |            |
        // |  v0 -------|-- v3           +--- x
        // | /          |  /            /
        // |/           | /            z
        // v1 --------- v2
        //
        // Where v4, v5, v6, v7 is the top face


        inline constexpr std::array<glm::ivec3, 8> pos_coords{
                glm::ivec3(0, 0, 0),  // v0
                glm::ivec3(0, 0, 1),  // v1
                glm::ivec3(1, 0, 1),  // v2
                glm::ivec3(1, 0, 0),  // v3
                glm::ivec3(0, 1, 0),  // v4
                glm::ivec3(0, 1, 1),  // v5
                glm::ivec3(1, 1, 1),  // v6
                glm::ivec3(1, 1, 0),  // v7
        };

        // Tex-coords always loop with the triangle going:
        // Counterclockwise order(right-hand rule), starting at top right
        inline constexpr std::array<glm::vec2, 4> tex_coords{
                glm::vec2(1.0f, 1.0f), // top-right
                glm::vec2(0.0f, 1.0f), // top-left
                glm::vec2(0.0f, 0.0f), // bottom-left
                glm::vec2(1.0f, 0.0f), // bottom-right
        };

        inline constexpr std::array<uint16, 24> vertex_indices = {
                // Each set of 6 indices represents one quad
                7, 6, 2, 3, // Front face
                6, 5, 1, 2, // Right face
                5, 4, 0, 1, // Back face
                4, 7, 3, 0, // Left face
                7, 4, 5, 6, // Top face
                2, 1, 0, 3  // Bottom face
        };

        inline constexpr std::array<uint16, 24> frame_indices = {
                7, 6,  6, 2,  2, 3,  3, 7,
                7, 4,  6, 5,  2, 1,  3, 0,
                5, 4,  4, 0,  0, 1,  1, 5
        };

        inline constexpr Block NULL_BLOCK{0, 255, 255, 255};
        inline constexpr Block AIR_BLOCK{1, 255, 255, 255};
    }
}

#endif //MAIN_CPP_CONSTANTS_H
