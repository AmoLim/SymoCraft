#include "cpu_baseline_fixture.h"
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/character_system.h>
#include <symocraft/simulation/player.h>
#include <symocraft/simulation/playercontroller.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/world/constants.h>
#include <array>
#include <iostream>

namespace {
    using namespace SymoCraft;
    using CpuBaseline::Near;
    using CpuBaseline::Require;
    using CpuBaseline::Step;

    struct PlayerScene {
        explicit PlayerScene(const char* configuration)
            : fixture(configuration), camera(fixture.registry, 1920.0f, 1080.0f),
              player(Simulation::CreatePlayer(fixture.registry, camera))
        {
            Pose({0.5f, 200.9f, 0.5f});
        }

        void Pose(glm::vec3 position, float yaw = 0.0f, float pitch = 0.0f)
        {
            auto& transform = fixture.registry.GetComponent<Transform>(player);
            transform.position = position;
            transform.yaw = yaw;
            transform.pitch = pitch;
            TransformSystem::Update(fixture.registry);
        }

        CpuBaseline::Fixture fixture;
        Camera camera;
        ECS::EntityId player;
    };

    void ZeroStepAcceptedJump(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        scene.fixture.SetBlock({0, 199, 0});
        auto& body = registry.GetComponent<Physics::RigidBody>(scene.player);
        auto& character = registry.GetComponent<Character::CharacterComponent>(scene.player);
        body.on_ground = true;
        body.velocity.y = -3.0f;
        body.acceleration.y = -8.0f;
        Simulation::PlayerInputState state;
        Simulation::PlayerIntent input;
        input.jump = true;
        const auto start = scene.fixture.Capture(scene.player);

        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, Step * 0.25f);
        Require(character.apply_jump_force, "jump.input.pending");
        Require(body.on_ground && !character.is_jumping, "jump.input.discrete_before_character_rule");
        Near(body.velocity.y, -3.0f, "jump.input.velocity_before_character_rule");
        Character::Player::Update(registry);
        Near(body.velocity, {0.0f, 7.6f, 0.0f}, "jump.zero_step.velocity");
        Near(body.acceleration.y, 0.0f, "jump.zero_step.acceleration.y");
        Require(!body.on_ground, "jump.zero_step.on_ground");
        Require(character.is_jumping, "jump.zero_step.is_jumping");
        Require(!character.apply_jump_force, "jump.zero_step.pending_consumed");

        Physics::Update(registry, *scene.fixture.world, Step * 0.25f);
        const auto zero_step = scene.fixture.Capture(scene.player);
        Near(zero_step.transform.position, start.transform.position, "jump.zero_step.position");
        Near(zero_step.body.velocity, {0.0f, 7.6f, 0.0f}, "jump.zero_step.physics_velocity");
        Require(!zero_step.body.on_ground && character.is_jumping && !character.apply_jump_force,
                "jump.zero_step.physics_discrete");

        Physics::Update(registry, *scene.fixture.world, Step * 0.75f);
        const auto one_step = scene.fixture.Capture(scene.player);
        Near(one_step.transform.position, start.transform.position + glm::vec3(0.0f, 7.6f * Step, 0.0f),
             "jump.following_step.position");
        Near(one_step.body.velocity.y, 7.6f - 20.0f * Step, "jump.following_step.velocity.y");
        Require(!one_step.body.on_ground && character.is_jumping && !character.apply_jump_force,
                "jump.following_step.discrete");
    }

    void OneInputAndCharacterRuleForManySteps(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        scene.fixture.SetBlock({0, 199, 0});
        auto& body = registry.GetComponent<Physics::RigidBody>(scene.player);
        auto& character = registry.GetComponent<Character::CharacterComponent>(scene.player);
        body.on_ground = true;
        const auto start = scene.fixture.Capture(scene.player);
        Simulation::PlayerInputState state;
        Simulation::PlayerIntent input;
        input.forward = input.jump = input.next_block = true;
        input.mouse_dy = 20.0;
        input.scroll_y = 10.0;

        // This is the existing app's once-per-frame order, not a new session implementation.
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.1f);
        TransformSystem::Update(registry);
        Character::Player::Update(registry);
        Physics::Update(registry, *scene.fixture.world, 0.1f);

        const auto result = scene.fixture.Capture(scene.player);
        Near(result.transform.position,
             start.transform.position + glm::vec3(4.4f * Step * 8.0f,
                 7.6f * Step * 8.0f - 20.0f * Step * Step * 28.0f, 0.0f),
             "many_steps.position");
        Near(result.body.velocity, {4.4f, 7.6f - 20.0f * Step * 8.0f, 0.0f}, "many_steps.velocity");
        Require(!body.on_ground && character.is_jumping && !character.apply_jump_force,
                "many_steps.jump_not_reapplied");
        Near(result.transform.pitch, 1.0f, "many_steps.pointer_pitch_once");
        Near(result.transform.yaw, 0.0f, "many_steps.pointer_yaw");
        Near(scene.camera.GetFov(), 35.0f, "many_steps.pointer_scroll_once");
        Require(state.selected_block == 1, "many_steps.selected_block_once");
        Near(state.block_change_debounce, 0.2f, "many_steps.material_cooldown_once", 1e-6);
        Character::Player::SyncCamera(registry, scene.camera.entity_id);
        Near(scene.camera.GetCameraPos(), result.transform.position + glm::vec3(0.0f, 0.65f, 0.0f),
             "many_steps.camera_after_physics");
        Near(scene.camera.GetPitch(), result.transform.pitch, "many_steps.camera_pitch");
    }

    void OrderedPointerClipping(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        scene.Pose({0.5f, 200.9f, 0.5f}, 0.0f, 89.0f);
        Simulation::ApplyPointerInput(registry, scene.player, scene.camera, 40.0, 200.0, -10.0);
        const auto first = scene.fixture.Capture(scene.player);
        Near(first.transform.pitch, 89.0f, "pointer.first.pitch_clipped");
        Near(first.transform.yaw, 2.0f, "pointer.first.yaw");
        Near(scene.camera.GetFov(), 45.0f, "pointer.first.fov_clipped");

        Simulation::ApplyPointerInput(registry, scene.player, scene.camera, -20.0, -200.0, 10.0);
        const auto second = scene.fixture.Capture(scene.player);
        Near(second.transform.pitch, 79.0f, "pointer.second.pitch_after_prior_clip");
        Near(second.transform.yaw, 1.0f, "pointer.second.yaw");
        Near(scene.camera.GetFov(), 35.0f, "pointer.second.fov_after_prior_clip");

        Simulation::PlayerInputState state;
        Simulation::ApplyInput(registry, scene.player, scene.camera, {}, state, 0.0f);
        Character::Player::Update(registry);
        Physics::Update(registry, *scene.fixture.world, 0.0f);
        Near(registry.GetComponent<Transform>(scene.player).pitch, 79.0f, "pointer.not_replayed_by_rules");
        Near(scene.camera.GetFov(), 35.0f, "pointer.scroll_not_replayed_by_rules");
    }

    void HeldInputAndMaterialCooldown(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        auto& character = registry.GetComponent<Character::CharacterComponent>(scene.player);
        Simulation::PlayerInputState state;
        state.selected_block = 7;
        Simulation::PlayerIntent input;
        input.forward = input.backward = input.right = input.left = true;
        input.next_block = input.previous_block = true;
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.0f);
        Require(character.movement_axis == glm::vec3(1.0f, 0.0f, 1.0f), "held_input.opposed_key_priority");
        Require(state.selected_block == 0, "material.next_priority_and_wrap");
        Near(state.block_change_debounce, 0.2f, "material.initial_cooldown", 1e-6);

        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.09f);
        Require(state.selected_block == 0, "material.early_repeat_rejected");
        Near(state.block_change_debounce, 0.11f, "material.elapsed_cooldown", 1e-6);
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.11f);
        Require(state.selected_block == 1, "material.due_repeat_accepted");
        Near(state.block_change_debounce, 0.2f, "material.due_repeat_cooldown", 1e-6);

        input = {};
        input.previous_block = true;
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.2f);
        Require(state.selected_block == 0, "material.previous_selection");
        Require(character.movement_axis == glm::vec3(0.0f), "held_input.release_clears_movement");
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.2f);
        Require(state.selected_block == 7, "material.previous_wrap");
    }

    void SensorCharacterRules(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        auto& body = registry.GetComponent<Physics::RigidBody>(scene.player);
        auto& character = registry.GetComponent<Character::CharacterComponent>(scene.player);
        body.on_ground = true;
        body.acceleration.y = -25.0f;
        character.is_jumping = true;
        Simulation::PlayerInputState state;
        Simulation::PlayerIntent input;
        input.run = input.sensor = input.descend = input.forward = input.right = input.jump = true;
        Simulation::ApplyInput(registry, scene.player, scene.camera, input, state, 0.0f);
        Require(body.is_sensor && !body.use_gravity && character.is_running, "sensor.input_modes");
        Require(character.movement_axis == glm::vec3(1.0f, -1.0f, 1.0f), "sensor.input_axes");
        Character::Player::Update(registry);
        const float speed = 6.2f / std::sqrt(3.0f);
        Near(body.velocity, {speed, -speed, speed}, "sensor.normalized_run_velocity");
        Near(body.acceleration.y, 0.0f, "sensor.gravity_acceleration_cleared");
        Require(!body.on_ground && !character.is_jumping && !character.apply_jump_force,
                "sensor.jump_and_ground_cleared");

        Simulation::ApplyInput(registry, scene.player, scene.camera, {}, state, 0.0f);
        Character::Player::Update(registry);
        Require(!body.is_sensor && body.use_gravity && !character.is_running, "sensor.release_modes");
        Near(body.velocity, {0.0f, -speed, 0.0f}, "sensor.release_preserves_vertical_velocity");
    }

    void DeferredPlacementAndCooldown(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        scene.Pose({0.5f, 200.85f, 0.5f});
        scene.fixture.SetBlock({2, 201, 0});
        scene.fixture.RemoveBlock({1, 201, 0});
        const auto start = scene.fixture.Capture(scene.player);
        Simulation::PlayerInputState state;
        Simulation::ApplyInput(registry, scene.player, scene.camera, {}, state, Step);
        Character::Player::Update(registry);
        Physics::Update(registry, *scene.fixture.world, Step);
        Near(scene.fixture.Capture(scene.player).transform.position, start.transform.position,
             "ordinary_edit.physics_before_interaction.position");
        Near(registry.GetComponent<Physics::RigidBody>(scene.player).velocity.y, -20.0f * Step,
             "ordinary_edit.physics_before_interaction.velocity.y");

        float cooldown = 0.0f;
        PlayerController::InteractionInput input{true, true, false, 0};
        const auto requested = PlayerController::DoRayCast(registry, *scene.fixture.world, scene.player, input, cooldown);
        Require(requested.selection.has_value(), "placement.selection_present");
        Near(*requested.selection, {2.5f, 201.5f, 0.5f}, "placement.selection");
        Require(requested.edit.has_value(), "placement.request_present");
        Require(requested.edit->operation == World::EditOperation::Set, "placement.operation");
        Require(requested.edit->position == glm::ivec3(1, 201, 0), "placement.position");
        Require(requested.edit->id == 2, "placement.inventory_block_id");
        Near(cooldown, 0.2f, "placement.request_cooldown", 1e-6);
        Require(scene.fixture.world->QueryBlock({1, 201, 0}).block == BlockConstants::AIR_BLOCK,
                "placement.world_unchanged_before_commit");

        const auto committed = scene.fixture.world->TryEdit(*requested.edit);
        Require(committed.status == World::EditStatus::Changed, "placement.commit_status");
        Require(committed.rejection == World::EditRejection::None, "placement.commit_rejection");
        Require(scene.fixture.world->QueryBlock({1, 201, 0}).block.block_id == 2, "placement.committed_block");
        cooldown -= 0.05f;
        const auto early = PlayerController::DoRayCast(registry, *scene.fixture.world, scene.player, input, cooldown);
        Require(early.selection.has_value(), "placement.cooldown_selection_present");
        Near(*early.selection, {1.5f, 201.5f, 0.5f}, "placement.selection_after_commit");
        Require(!early.edit, "placement.cooldown_repeat_rejected");
        Near(cooldown, 0.15f, "placement.cooldown_not_restarted", 1e-6);

        cooldown -= 0.15f;
        input.place = false;
        input.remove = true;
        const auto removal = PlayerController::DoRayCast(registry, *scene.fixture.world, scene.player, input, cooldown);
        Require(removal.edit.has_value(), "placement.following_removal_request_present");
        Require(removal.edit->operation == World::EditOperation::Remove &&
                removal.edit->position == glm::ivec3(1, 201, 0), "placement.following_removal_order");
        Require(scene.fixture.world->QueryBlock({1, 201, 0}).block.block_id == 2,
                "placement.removal_deferred_until_commit");
        const auto removed = scene.fixture.world->TryEdit(*removal.edit);
        Require(removed.status == World::EditStatus::Changed && removed.rejection == World::EditRejection::None,
                "placement.removal_commit_result");
        Require(scene.fixture.world->QueryBlock({1, 201, 0}).block == BlockConstants::AIR_BLOCK,
                "placement.removal_committed_block");
        Near(cooldown, 0.2f, "placement.removal_cooldown", 1e-6);
    }

    void RejectedPlacementConsumesCooldown(const char* configuration)
    {
        PlayerScene scene(configuration);
        scene.Pose({0.5f, 200.85f, 0.5f});
        scene.fixture.SetBlock({1, 201, 0});
        scene.fixture.RemoveBlock({0, 201, 0});
        float cooldown = 0.0f;
        PlayerController::InteractionInput input{true, true, true, 0};
        auto result = PlayerController::DoRayCast(scene.fixture.registry, *scene.fixture.world,
                                                  scene.player, input, cooldown);
        Require(result.selection.has_value() && !result.edit, "rejected_placement.overlap_no_remove_fallback");
        Near(*result.selection, {1.5f, 201.5f, 0.5f}, "rejected_placement.selection");
        Near(cooldown, 0.2f, "rejected_placement.overlap_cooldown", 1e-6);
        Require(scene.fixture.world->QueryBlock({1, 201, 0}).block.block_id == 5,
                "rejected_placement.target_unchanged");
        Require(scene.fixture.world->QueryBlock({0, 201, 0}).block == BlockConstants::AIR_BLOCK,
                "rejected_placement.destination_unchanged");

        input.remove = false;
        for (const int invalid_selection : {-1, 8}) {
            cooldown = 0.0f;
            input.selected_block = invalid_selection;
            result = PlayerController::DoRayCast(scene.fixture.registry, *scene.fixture.world,
                                                 scene.player, input, cooldown);
            Require(result.selection.has_value() && !result.edit, "rejected_placement.invalid_inventory_request");
            Near(cooldown, 0.2f, "rejected_placement.invalid_inventory_cooldown", 1e-6);
        }
    }

    void SelectionWithoutAllowedInputAndMiss(const char* configuration)
    {
        PlayerScene scene(configuration);
        scene.Pose({0.5f, 200.85f, 0.5f});
        scene.fixture.SetBlock({2, 201, 0});
        float cooldown = -0.1f;
        PlayerController::InteractionInput input{false, true, true, 0};
        const auto disabled = PlayerController::DoRayCast(scene.fixture.registry, *scene.fixture.world,
                                                         scene.player, input, cooldown);
        Require(disabled.selection.has_value() && !disabled.edit, "disabled_input.selection_without_edit");
        Near(*disabled.selection, {2.5f, 201.5f, 0.5f}, "disabled_input.selection");
        Near(cooldown, -0.1f, "disabled_input.cooldown_unchanged", 1e-6);

        scene.Pose({0.5f, 200.85f, 0.5f}, 180.0f);
        cooldown = 0.0f;
        input.allow_input = true;
        const auto miss = PlayerController::DoRayCast(scene.fixture.registry, *scene.fixture.world,
                                                     scene.player, input, cooldown);
        Require(!miss.selection && !miss.edit, "ray_miss.no_selection_or_edit");
        Near(cooldown, 0.0f, "ray_miss.cooldown_unchanged", 1e-6);
    }

    void OrdinaryRemovalAfterPhysics(const char* configuration)
    {
        PlayerScene scene(configuration);
        auto& registry = scene.fixture.registry;
        scene.Pose({0.5f, 200.9f, 0.5f}, 0.0f, -89.0f);
        scene.fixture.SetBlock({0, 199, 0});
        auto& body = registry.GetComponent<Physics::RigidBody>(scene.player);
        body.on_ground = true;
        Simulation::PlayerInputState state;
        const auto start = scene.fixture.Capture(scene.player);
        Simulation::ApplyInput(registry, scene.player, scene.camera, {}, state, Step);
        Character::Player::Update(registry);
        Physics::Update(registry, *scene.fixture.world, Step);
        Require(body.on_ground, "ordinary_removal.physics_observes_support_before_commit");
        Near(body.velocity.y, 0.0f, "ordinary_removal.supported_velocity.y");
        Near(scene.fixture.Capture(scene.player).transform.position, start.transform.position,
             "ordinary_removal.supported_position");

        float cooldown = 0.0f;
        const PlayerController::InteractionInput input{true, false, true, 0};
        const auto requested = PlayerController::DoRayCast(registry, *scene.fixture.world, scene.player, input, cooldown);
        Require(requested.selection.has_value() && requested.edit.has_value(), "ordinary_removal.request_present");
        Near(*requested.selection, {0.5f, 199.5f, 0.5f}, "ordinary_removal.selection");
        Require(requested.edit->operation == World::EditOperation::Remove &&
                requested.edit->position == glm::ivec3(0, 199, 0), "ordinary_removal.request_fields");
        Require(scene.fixture.world->QueryBlock({0, 199, 0}).block.block_id == 5,
                "ordinary_removal.world_unchanged_before_commit");
        const auto committed = scene.fixture.world->TryEdit(*requested.edit);
        Require(committed.status == World::EditStatus::Changed && committed.rejection == World::EditRejection::None,
                "ordinary_removal.commit_result");
        Require(body.on_ground, "ordinary_removal.commit_does_not_run_physics");
        Near(cooldown, 0.2f, "ordinary_removal.cooldown", 1e-6);

        Simulation::ApplyInput(registry, scene.player, scene.camera, {}, state, Step);
        Character::Player::Update(registry);
        Physics::Update(registry, *scene.fixture.world, Step);
        Require(!body.on_ground, "ordinary_removal.following_step_support_lost");
        Near(body.velocity.y, -20.0f * Step, "ordinary_removal.following_step.velocity.y");
        Near(scene.fixture.Capture(scene.player).transform.position, start.transform.position,
             "ordinary_removal.following_step.position_before_gravity");
        Physics::Update(registry, *scene.fixture.world, Step);
        Near(scene.fixture.Capture(scene.player).transform.position,
             start.transform.position + glm::vec3(0.0f, -20.0f * Step * Step, 0.0f),
             "ordinary_removal.next_step.position_falls");
    }
}

int main(int argc, char** argv)
{
    const char* active_case = "arguments";
    try {
        Require(argc == 2, "Expected block configuration path");
        struct Case { const char* name; void (*run)(const char*); };
        const std::array cases{
            Case{"zero_step_accepted_jump", ZeroStepAcceptedJump},
            Case{"one_input_character_rule_many_steps", OneInputAndCharacterRuleForManySteps},
            Case{"ordered_pointer_per_event_clipping", OrderedPointerClipping},
            Case{"held_input_material_cooldown", HeldInputAndMaterialCooldown},
            Case{"sensor_character_rules", SensorCharacterRules},
            Case{"deferred_placement_commit_and_cooldown", DeferredPlacementAndCooldown},
            Case{"rejected_placement_consumes_cooldown", RejectedPlacementConsumesCooldown},
            Case{"disabled_input_selection_and_ray_miss", SelectionWithoutAllowedInputAndMiss},
            Case{"ordinary_support_removal_after_physics", OrdinaryRemovalAfterPhysics}
        };
        for (const auto& test : cases) {
            active_case = test.name;
            test.run(argv[1]);
            std::cout << "[PASS] " << test.name << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << active_case << ": " << error.what() << '\n';
        return 1;
    }
}
