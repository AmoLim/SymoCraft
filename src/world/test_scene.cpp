#include "world/test_scene.h"
#include "world/chunk.h"
#include <array>
#include <stdexcept>

namespace SymoCraft::TestScene {
    namespace {
        const std::array<Pose, 7> checkpoints{{
            {"spawn", {-20.5f, 161.9f, -6.5f}, 0.0f, -10.0f},
            {"positive-x", {14.5f, 161.9f, 2.5f}, -63.435f, -25.16f},
            {"negative-x", {-17.5f, 161.9f, 2.5f}, -63.435f, -25.16f},
            {"four-chunk", {-2.5f, 161.9f, -2.5f}, 45.0f, -20.4f},
            {"wall-corner", {6.5f, 161.9f, 6.5f}, 45.0f, 0.0f},
            {"low-ceiling", {-6.5f, 161.9f, 6.5f}, 0.0f, 0.0f},
            {"single-block", {20.5f, 168.0f, 20.5f}, -90.0f, -60.0f}
        }};
        const std::array<glm::ivec3, 8> edit_targets{{
            {15, 161, 0}, {16, 161, 0}, {-17, 161, 0}, {-16, 161, 0},
            {-1, 161, -1}, {-1, 161, 0}, {0, 161, -1}, {0, 161, 0}
        }};
        const std::array<Edit, 16> edits = [] {
            std::array<Edit, 16> result{};
            for (std::size_t i = 0; i < edit_targets.size(); ++i) {
                result[i * 2] = {edit_targets[i], 5, 1};
                result[i * 2 + 1] = {edit_targets[i], 1, static_cast<unsigned short>(i % 2 ? 11 : 8)};
            }
            return result;
        }();

        void Write(const glm::ivec3& position, unsigned short block)
        {
            const glm::vec3 world(position);
            auto* chunk = ChunkManager::GetChunk(world);
            if (!chunk || chunk->m_is_fringe_chunk || !chunk->SetWorldBlock(world, block))
                throw std::runtime_error("Regression fixture is outside the playable world");
        }

        YAML::Node Vector(const glm::vec3& value)
        {
            YAML::Node node;
            node.push_back(value.x); node.push_back(value.y); node.push_back(value.z);
            node.SetStyle(YAML::EmitterStyle::Flow);
            return node;
        }
    }

    std::span<const Pose> Checkpoints() { return checkpoints; }
    const Pose& Checkpoint(std::string_view name)
    {
        for (const auto& pose : checkpoints) if (pose.name == name) return pose;
        throw std::invalid_argument("Unknown regression checkpoint");
    }
    std::span<const Edit> Edits() { return edits; }

    void Install()
    {
        // Validate coverage before changing anything; this pad is an explicit test overlay, not terrain.
        for (int x = -24; x <= 24; ++x)
            for (int z = -24; z <= 24; ++z) {
                auto* chunk = ChunkManager::GetChunk(glm::vec3{x, 160, z});
                if (!chunk || chunk->m_is_fringe_chunk)
                    throw std::invalid_argument("Regression scene requires at least radius 3");
            }
        for (int x = -24; x <= 24; ++x)
            for (int z = -24; z <= 24; ++z) {
                for (int y = 158; y <= 174; ++y) Write({x, y, z}, 1);
                Write({x, 160, z}, 2);
            }
        for (int y = 161; y <= 163; ++y)
            for (int t = 4; t <= 8; ++t) {
                Write({8, y, t}, 11);
                Write({t, y, 8}, 11);
            }
        for (int x = -8; x <= -4; ++x)
            for (int z = 4; z <= 8; ++z) Write({x, 163, z}, 8);
        Write({20, 164, 20}, 5);
        for (const auto target : edit_targets) Write(target, 5);
    }

    void ApplyEdits()
    {
        for (const auto& edit : edits) {
            if (ChunkManager::GetBlock(glm::vec3(edit.position)).block_id != edit.before)
                throw std::runtime_error("Regression edit precondition failed; regenerate the scene first");
            Write(edit.position, edit.after);
        }
    }

    YAML::Node Describe()
    {
        YAML::Node node;
        node["name"] = "regression";
        node["version"] = Version;
        node["floor_y"] = 160;
        node["pad_min_xz"] = -24;
        node["pad_max_xz"] = 24;
        node["fov_degrees"] = 45;
        for (const auto& pose : checkpoints) {
            YAML::Node entry;
            entry["name"] = std::string(pose.name);
            entry["position"] = Vector(pose.position);
            entry["yaw"] = pose.yaw;
            entry["pitch"] = pose.pitch;
            node["checkpoints"].push_back(entry);
        }
        node["route"]["name"] = "walk-positive-x-v1";
        node["route"]["mode"] = "manual-waypoints-not-input-replay";
        node["route"]["walk_speed"] = 4.4f;
        node["route"]["yaw"] = 0;
        node["route"]["pitch"] = -10;
        for (const float x : {-20.5f, -16.5f, -0.5f, 15.5f, 20.5f})
            node["route"]["waypoints"].push_back(Vector({x, 161.9f, -6.5f}));
        node["edit_plan"]["mode"] = "ordered-world-writes-not-player-input";
        node["edit_plan"]["manual_min_interval_seconds"] = 0.25f;
        for (const auto& edit : edits) {
            YAML::Node entry;
            entry["position"] = Vector(glm::vec3(edit.position));
            entry["before"] = edit.before;
            entry["after"] = edit.after;
            node["edit_plan"]["operations"].push_back(entry);
        }
        return node;
    }
}
