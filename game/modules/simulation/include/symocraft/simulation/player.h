#pragma once

#include <symocraft/ecs/registry.h>
#include <symocraft/foundation/math.h>

namespace SymoCraft {
    class Camera;
    namespace ECS { class Registry; }
    namespace Simulation {
        struct LensState;
        struct PlayerIntent {
            bool run{}, sensor{}, descend{}, forward{}, backward{}, right{}, left{}, jump{};
            bool next_block{}, previous_block{};
            // Mouse deltas: right positive and up positive, independent of platform coordinates.
            double mouse_dx{}, mouse_dy{}, scroll_y{};
        };
        struct PlayerInputState {
            int selected_block{};
            float block_change_debounce{};
        };
        ECS::EntityId CreatePlayer(ECS::Registry& registry, Camera& camera);
        ECS::EntityId CreatePlayer(ECS::Registry& registry, glm::vec3 position, float yaw, float pitch);
        void ApplyPointerInput(ECS::Registry& registry, ECS::EntityId player, LensState& lens,
                               double mouse_dx, double mouse_dy, double scroll_y);
        void ApplyPointerInput(ECS::Registry& registry, ECS::EntityId player, Camera& camera,
                               double mouse_dx, double mouse_dy, double scroll_y);
        void ApplyInput(ECS::Registry& registry, ECS::EntityId player, Camera& camera,
                        const PlayerIntent& input, PlayerInputState& state, float delta);
        void ApplyControlInput(ECS::Registry& registry, ECS::EntityId player,
                               const PlayerIntent& input, PlayerInputState& state, float delta);
    }
}
