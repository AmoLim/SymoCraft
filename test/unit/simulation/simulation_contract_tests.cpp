#include <symocraft/ecs/registry.h>
#include <symocraft/scene/mesh.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/character_system.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/player.h>
#include <symocraft/simulation/playercontroller.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/world/block.h>
#include <symocraft/world/world.h>
#include <symocraft/world/generation.h>
#include <symocraft/world/test_scene.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    bool Near(float first, float second) { return std::abs(first - second) < 0.0001f; }
}

int main(int argc, char** argv)
{
    using namespace SymoCraft;
    try {
        Require(argc == 2, "Expected block configuration path");
        static_assert(sizeof(BlockVertex3D) == 28, "T0 must preserve existing vertex stride");
        std::ifstream config(argv[1], std::ios::binary);
        Require(static_cast<bool>(config), "Cannot read block configuration");
        const std::string text{std::istreambuf_iterator<char>(config), std::istreambuf_iterator<char>()};
        auto world = World::VoxelWorld::Create(World::BlockDefinition::FromConfig(text), {TestScene::DefaultSeed, 3, true});
        ECS::Registry registry;
        registry.RegisterComponent<Transform>("Transform");
        registry.RegisterComponent<Physics::RigidBody>("RigidBody");
        registry.RegisterComponent<Physics::HitBox>("HitBox");
        registry.RegisterComponent<Character::CharacterComponent>("Character");
        registry.RegisterComponent<Character::PlayerComponent>("Player");
        Camera camera(registry, 1920, 1080);
        const auto player = Simulation::CreatePlayer(registry, camera);
        auto& transform = registry.GetComponent<Transform>(player);
        auto& body = registry.GetComponent<Physics::RigidBody>(player);
        auto& character = registry.GetComponent<Character::CharacterComponent>(player);
        const auto& hit_box = registry.GetComponent<Physics::HitBox>(player);
        Require(Near(hit_box.size.x, 0.55f) && Near(hit_box.size.y, 1.8f), "Player dimensions changed");
        Require(body.use_gravity && Near(character.base_speed, 4.4f), "Player defaults changed");

        Simulation::PlayerInputState input_state;
        Simulation::PlayerIntent input;
        input.run = input.sensor = input.descend = input.forward = input.right = input.next_block = true;
        input.mouse_dx = 10;
        input.mouse_dy = 10000;
        input.scroll_y = 2;
        Simulation::ApplyInput(registry, player, camera, input, input_state, 0.01f);
        Require(body.is_sensor && !body.use_gravity && character.is_running, "Input mode mapping changed");
        Require(character.movement_axis == glm::vec3(1, -1, 1), "Movement axes changed");
        Require(Near(transform.yaw, -89.5f) && Near(transform.pitch, 89.0f), "Mouse orientation contract changed");
        Require(Near(camera.GetFov(), 43.0f) && input_state.selected_block == 1, "Scroll or material input changed");
        input.mouse_dx = input.mouse_dy = input.scroll_y = 0;
        Simulation::ApplyInput(registry, player, camera, input, input_state, 0.1f);
        Require(input_state.selected_block == 1, "Material debounce no longer rejects early repeat");
        Simulation::ApplyInput(registry, player, camera, input, input_state, 0.11f);
        Require(input_state.selected_block == 2, "Material debounce no longer allows due repeat");
        input_state.selected_block = 7;
        input_state.block_change_debounce = 0;
        Simulation::ApplyInput(registry, player, camera, input, input_state, 0);
        Require(input_state.selected_block == 0, "Material forward wrap changed");
        input = {};
        input.previous_block = true;
        input_state.block_change_debounce = 0;
        body.on_ground = true;
        input.jump = true;
        Simulation::ApplyInput(registry, player, camera, input, input_state, 0);
        Require(input_state.selected_block == 7 && character.apply_jump_force, "Reverse wrap or jump changed");
        Require(!body.is_sensor && body.use_gravity && character.movement_axis == glm::vec3(0), "Released keys remain active");
        camera.Scroll(1000);
        Require(Near(camera.GetFov(), 1), "Minimum FOV changed");
        camera.Scroll(-1000);
        Require(Near(camera.GetFov(), 45), "Maximum FOV changed");
        transform.pitch = 89;
        Simulation::ApplyPointerInput(registry, player, camera, 0, 200, -10);
        Simulation::ApplyPointerInput(registry, player, camera, 0, -200, 10);
        Require(Near(transform.pitch, 79) && Near(camera.GetFov(), 35),
                "Ordered pointer events must retain per-event pitch/FOV clipping");

        transform.position = {14.5f, 160.85f, 0.5f};
        transform.yaw = 0;
        transform.pitch = 0;
        TransformSystem::Update(registry);
        Character::Player::SyncCamera(registry, camera.entity_id);
        Require(glm::length(camera.GetCameraPos() - glm::vec3(14.5f, 161.5f, 0.5f)) < 0.0001f,
                "Camera no longer follows explicit player state");
        const auto view = camera.GetCameraViewMat();
        const auto projection = camera.GetCameraProjMat(1920.0f / 1080.0f);
        Require(std::isfinite(view[0][0]) && std::isfinite(projection[0][0]), "Camera matrices are invalid");

        TestScene::Install(*world);
        const glm::vec3 target(15.5f, 161.5f, 0.5f);
        const auto before = world->QueryBlock({15, 161, 0}).block.block_id;
        Require(before == 5, "Regression fixture changed");
        float debounce = 0;
        PlayerController::InteractionInput interaction;
        interaction.allow_input = false;
        interaction.remove = true;
        auto result = PlayerController::DoRayCast(registry, *world, player, interaction, debounce);
        Require(result.selection && *result.selection == target && !result.edit && debounce == 0,
                "Disabled input must still select without editing/debouncing");
        interaction.allow_input = true;
        result = PlayerController::DoRayCast(registry, *world, player, interaction, debounce);
        Require(result.edit && result.edit->operation == World::EditOperation::Remove && result.edit->position == glm::ivec3(15, 161, 0) && Near(debounce, 0.2f),
                "Removal request or cooldown changed");
        Require(world->QueryBlock({15, 161, 0}).block.block_id == before, "Raycast changed world before app committed request");
        result = PlayerController::DoRayCast(registry, *world, player, interaction, debounce);
        Require(!result.edit, "Placement debounce no longer rejects repeat");
        debounce = 0;
        interaction.remove = false;
        interaction.place = true;
        result = PlayerController::DoRayCast(registry, *world, player, interaction, debounce);
        Require(!result.edit && Near(debounce, 0.2f), "Placement inside player must be rejected with old cooldown");
        Require(world->QueryBlock({15, 161, 0}).block.block_id == before, "Rejected placement modified world");
        std::cout << "Simulation explicit-input, camera and deferred-interaction contracts passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
