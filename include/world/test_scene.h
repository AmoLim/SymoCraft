#pragma once

#include <span>
#include <string_view>
#include <glm/glm.hpp>
#include <yaml-cpp/yaml.h>

namespace SymoCraft::TestScene {
    inline constexpr unsigned Version = 1;
    inline constexpr unsigned DefaultSeed = 424242;
    struct Pose {
        std::string_view name;
        glm::vec3 position;
        float yaw;
        float pitch;
    };
    struct Edit {
        glm::ivec3 position;
        unsigned short before;
        unsigned short after;
    };
    std::span<const Pose> Checkpoints();
    const Pose& Checkpoint(std::string_view name);
    std::span<const Edit> Edits();
    void Install();
    void ApplyEdits();
    YAML::Node Describe();
}
