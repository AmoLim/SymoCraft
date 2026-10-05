#include <symocraft/simulation/player.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/playercontroller.h>

namespace SymoCraft::Simulation{

    ECS::EntityId CreatePlayer(ECS::Registry& registry, Camera& camera)
    {
        const auto player = registry.CreateEntity();
        registry.AddComponent<Transform>(player);
        registry.AddComponent<Physics::HitBox>(player);
        registry.AddComponent<Physics::RigidBody>(player);
        registry.AddComponent<Character::CharacterComponent>(player);
        registry.AddComponent<Character::PlayerComponent>(player);

        // Hit box init
        auto& boxCollider = registry.GetComponent<Physics::HitBox>(player);
        boxCollider = {};
        boxCollider.size.x = 0.55f;
        boxCollider.size.y = 1.8f;
        boxCollider.size.z = 0.55f;

        // transform init
        auto& transform = registry.GetComponent<Transform>(player);
        transform = {};
        transform.scale = glm::vec3(1.0f);
        auto& camera_transform = registry.GetComponent<Transform>(camera.entity_id);
        transform.position.x = camera_transform.position.x;
        transform.position.y = camera_transform.position.y - 0.65f;
        transform.position.z = camera_transform.position.z;
        transform.yaw = camera_transform.yaw;
        transform.pitch = camera_transform.pitch;

        //  character component init
        auto &controller = registry.GetComponent<Character::CharacterComponent>(player);
        controller = {};
        controller.base_speed = 4.4f;
        controller.run_speed = 6.2f;
        controller.is_running = false;
        controller.movement_axis = glm::vec3();
        controller.apply_jump_force = false;
        controller.jump_force = 7.6f;
        controller.down_jump_force = -25.0f;

        // rigid body init
        auto &rigid_body = registry.GetComponent<Physics::RigidBody>(player);
        rigid_body = {};
        rigid_body.use_gravity = true;

        auto &player_com = registry.GetComponent<Character::PlayerComponent>(player);
        player_com = {};
        player_com.camera_offset = glm::vec3(0.0f, 0.65f, 0.0f);
        player_com.movement_sensitivity = 0.25f;
        return player;
    }

    void ApplyPointerInput(ECS::Registry& registry, ECS::EntityId player, Camera& camera,
                           double mouse_dx, double mouse_dy, double scroll_y)
    {
        auto& transform = registry.GetComponent<Transform>(player);
        transform.pitch = glm::clamp(transform.pitch + static_cast<float>(mouse_dy) * 0.05f, -89.0f, 89.0f);
        transform.yaw += static_cast<float>(mouse_dx) * 0.05f;
        camera.Scroll(scroll_y);
    }

    void ApplyInput(ECS::Registry& registry, ECS::EntityId player, Camera& camera,
                    const PlayerIntent& input, PlayerInputState& state, float delta)
    {
        if (input.mouse_dx != 0 || input.mouse_dy != 0 || input.scroll_y != 0)
            ApplyPointerInput(registry, player, camera, input.mouse_dx, input.mouse_dy, input.scroll_y);
        auto& character = registry.GetComponent<Character::CharacterComponent>(player);
        auto& body = registry.GetComponent<Physics::RigidBody>(player);
        character.is_running = input.run;
        character.movement_axis.y = 0.0f;
        body.is_sensor = input.sensor;
        body.use_gravity = !input.sensor;
        if (input.sensor)
            character.movement_axis.y = input.descend ? -1.0f : 0.0f;
        character.movement_axis.x = input.forward ? 1.0f : input.backward ? -1.0f : 0.0f;
        character.movement_axis.z = input.right ? 1.0f : input.left ? -1.0f : 0.0f;
        if (input.jump && !character.is_jumping && body.on_ground)
            character.apply_jump_force = true;

        state.block_change_debounce -= delta;
        constexpr int last_block = static_cast<int>(PlayerController::kBlockInventor.size()) - 1;
        if (input.next_block && state.block_change_debounce <= 0.0f) {
            if (++state.selected_block > last_block) state.selected_block = 0;
            state.block_change_debounce = 0.2f;
            PlayerController::DisplayCurrentBlockName(state.selected_block);
        }
        if (input.previous_block && state.block_change_debounce <= 0.0f) {
            if (--state.selected_block < 0) state.selected_block = last_block;
            state.block_change_debounce = 0.2f;
            PlayerController::DisplayCurrentBlockName(state.selected_block);
        }
    }
}
