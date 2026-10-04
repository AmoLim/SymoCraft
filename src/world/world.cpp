#include "world/world.h"
#include "core/application.h"
#include "core/ECS/registry.h"
#include "core/ECS/component.h"
#include "world/block.h"
#include "world/chunk_manager.h"
#include "world/chunk.h"
#include "camera/camera.h"

namespace SymoCraft::World{

    void Init()
    {
        CreatePlayer();
    }

    void CreatePlayer()
    {
        ECS::Registry &registry = Application::GetRegistry();
        Camera *camera = Application::GetCamera();
        player = registry.CreateEntity();
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
        auto& camera_transform = registry.GetComponent<Transform>(camera->entity_id);
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
    }

    ECS::EntityId GetPlayer()
    {
        return player;
    }
}
