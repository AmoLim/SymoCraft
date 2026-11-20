#include <symocraft/simulation/session.h>
#include <symocraft/profiling/profiling.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/character_system.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/simulation/playercontroller.h>
#include <symocraft/simulation/physics_system.h>
#include <symocraft/simulation/player_math.h>
#include "physics_adapter.h"
#include "npc_runtime.h"
#include "diagnostics/session_diagnostics.h"
#include "diagnostics/npc_diagnostics.h"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <limits>

namespace SymoCraft::Simulation {
    namespace {
        std::uint64_t NextSessionIdentity() {
            static std::atomic<std::uint64_t> next{1};
            auto value = next.load(std::memory_order_relaxed);
            do {
                if (value == (std::numeric_limits<std::uint64_t>::max)()) throw Rejected("Session identity exhausted");
            } while (!next.compare_exchange_weak(value, value + 1, std::memory_order_relaxed));
            return value;
        }
        void CheckPointer(double value) {
            if (!std::isfinite(value) || std::abs(value) > (std::numeric_limits<float>::max)())
                throw Rejected("Advance: pointer value is not finite/float-representable");
        }
        void CheckPointer(const PointerInput& value) {
            CheckPointer(value.mouse_dx); CheckPointer(value.mouse_dy); CheckPointer(value.scroll_y);
        }
        PlayerPose ReadPose(const ECS::Registry& registry, ECS::EntityId player) {
            const auto& transform = registry.GetComponent<Transform>(player);
            if (!PlayerMath::IsFinite(transform.position) || !PlayerMath::IsFinite(transform.front) ||
                !PlayerMath::IsFinite(transform.up) || !std::isfinite(transform.yaw) || !std::isfinite(transform.pitch))
                throw std::runtime_error("Session player pose is not finite");
            return {transform.position, transform.yaw, transform.pitch, transform.front, transform.up};
        }
    }

    struct SimulationSession::Impl {
        ECS::Registry& registry;
        const World::VoxelWorld& world;
        ECS::EntityId player;
        PlayerMath::FixedStepBudget budget;
        PlayerInputState controls;
        LensState lens;
        float edit_cooldown = 0;
        SessionState state = SessionState::Ready;
        bool resume_pending = false;
        std::unique_ptr<Detail::NpcRuntime> npcs;
        std::uint64_t identity = NextSessionIdentity();
        std::uint64_t frame_id{};
        bool pending_frame = false;
        std::optional<World::EditRequest> pending_edit;

        Impl(ECS::Registry& storage, const World::VoxelWorld& terrain, ECS::EntityId entity)
            : registry(storage), world(terrain), player(entity) {}
        void RequirePlayer() const {
            if (!registry.IsEntityValid(player) || !registry.HasComponent<Transform>(player) ||
                !registry.HasComponent<Physics::RigidBody>(player) || !registry.HasComponent<Physics::HitBox>(player) ||
                !registry.HasComponent<Character::CharacterComponent>(player) ||
                !registry.HasComponent<Character::PlayerComponent>(player))
                throw std::runtime_error("Session player storage is missing");
        }
        LensState CurrentLens() const {
            auto result = lens;
            result.eye_offset = registry.GetComponent<Character::PlayerComponent>(player).camera_offset;
            return result;
        }
    };

    AdvanceFailed::AdvanceFailed(AdvanceStage stage, unsigned steps, std::uint64_t commits)
        : std::runtime_error("Advance failed; session cannot continue"), stage_(stage), steps_(steps), commits_(commits) {}
    SimulationSession::SimulationSession(std::unique_ptr<Impl> implementation) : impl_(std::move(implementation)) {}
    SimulationSession::~SimulationSession() = default;
    std::unique_ptr<SimulationSession> SimulationSession::Create(ECS::Registry& registry,
        const World::VoxelWorld& world, PlayerPose initial) {
        if (!PlayerMath::IsFinite(initial.position) || !std::isfinite(initial.yaw) ||
            !std::isfinite(initial.pitch) || std::abs(initial.pitch) > 89.0f)
            throw Rejected("CreateSession: initial pose is invalid");
        if (!registry.IsComponentRegistered<Transform>() || !registry.IsComponentRegistered<Physics::RigidBody>() ||
            !registry.IsComponentRegistered<Physics::HitBox>() ||
            !registry.IsComponentRegistered<PhysicsStepOrder>() ||
            !registry.IsComponentRegistered<Character::CharacterComponent>() ||
            !registry.IsComponentRegistered<Character::PlayerComponent>())
            throw Rejected("CreateSession: caller must register all player components");
        const auto player = CreatePlayer(registry, initial.position, initial.yaw, initial.pitch);
        auto implementation = std::make_unique<Impl>(registry, world, player);
        TransformSystem::Update(registry);
        static_cast<void>(ReadPose(registry, player));
        return std::unique_ptr<SimulationSession>(new SimulationSession(std::move(implementation)));
    }
    SessionState SimulationSession::State() const noexcept { return impl_->state; }
    ECS::EntityId SimulationSession::AddNpc(const NpcConfig& config) {
        if (impl_->state != SessionState::Ready || impl_->pending_frame) throw Rejected("AddNpc: session is not at an editable boundary");
        if (!impl_->npcs) impl_->npcs = std::make_unique<Detail::NpcRuntime>(impl_->registry, impl_->world);
        return impl_->npcs->Add(config);
    }
    void SimulationSession::RemoveNpc(ECS::EntityId entity) {
        if (impl_->state == SessionState::Failed || !impl_->npcs) throw Rejected("RemoveNpc: no active NPC runtime");
        impl_->npcs->Remove(entity);
    }
    ActionId SimulationSession::StartHarvest(ECS::EntityId entity, World::BlockCoord target) {
        if (impl_->state != SessionState::Ready || impl_->pending_frame || !impl_->npcs)
            throw Rejected("StartHarvest: session is not at an editable boundary");
        return impl_->npcs->Start(entity, target);
    }
    void SimulationSession::CancelAction(ECS::EntityId entity, ActionId action) {
        if (impl_->state == SessionState::Failed || !impl_->npcs) throw Rejected("CancelAction: no active NPC runtime");
        impl_->npcs->Cancel(entity, action);
    }
    NpcSnapshot SimulationSession::InspectNpc(ECS::EntityId entity) const {
        if (impl_->state == SessionState::Failed || !impl_->npcs) throw Rejected("InspectNpc: no readable NPC runtime");
        return impl_->npcs->Inspect(entity);
    }
    std::vector<NpcDisplay> SimulationSession::NpcDisplays() const {
        if (impl_->state == SessionState::Failed) throw Rejected("NpcDisplays: session has failed");
        auto values = impl_->npcs ? impl_->npcs->Displays() : std::vector<NpcDisplay>{};
        for (auto& value : values) {
            value.session_id = impl_->identity;
            value.advance_id = impl_->frame_id;
        }
        return values;
    }
    void SimulationSession::CommitFrame(World::VoxelWorld& world, const AdvanceResult& result) {
        if (impl_->state != SessionState::Ready || !impl_->pending_frame || &world != &impl_->world ||
            result.frame_owner != impl_->identity || result.frame_id != impl_->frame_id)
            throw Rejected("CommitFrame: stale, foreign or consumed frame batch");
        impl_->pending_frame = false;
        try {
            if (impl_->pending_edit) world.TryEdit(*impl_->pending_edit);
            impl_->pending_edit.reset();
            impl_->npcs->Commit(world);
        } catch (...) {
            impl_->state = SessionState::Failed;
            throw;
        }
    }
    void Detail::NpcDiagnosticsAdapter::SetResource(SimulationSession& session, ECS::EntityId entity,
                                                   std::string resource, std::uint64_t quantity) {
        if (session.impl_->state != SessionState::Ready || session.impl_->pending_frame || !session.impl_->npcs)
            throw Rejected("NPC test setup requires an editable session");
        auto& values = session.impl_->npcs->Require(entity).resources;
        const auto entry = std::lower_bound(values.begin(), values.end(), resource,
            [](const auto& value, const auto& key) { return value.resource_id < key; });
        if (entry != values.end() && entry->resource_id == resource) entry->quantity = quantity;
        else values.insert(entry, {std::move(resource), quantity});
    }
    void Detail::NpcDiagnosticsAdapter::FailAfterRemoveOnce(SimulationSession& session) {
        if (session.impl_->state != SessionState::Ready || !session.impl_->npcs)
            throw Rejected("NPC test setup requires a ready session");
        session.impl_->npcs->fail_after_remove_ = true;
    }
    void SimulationSession::Pause() {
        if (impl_->state == SessionState::Failed) throw Rejected("Pause: session has failed");
        if (impl_->state == SessionState::Paused) return;
        try {
            impl_->RequirePlayer();
            auto& character = impl_->registry.GetComponent<Character::CharacterComponent>(impl_->player);
            auto& body = impl_->registry.GetComponent<Physics::RigidBody>(impl_->player);
            character.movement_axis = glm::vec3(0.0f);
            character.apply_jump_force = false;
            body.velocity.x = body.velocity.z = 0.0f;
            impl_->budget.Reset();
            if (impl_->npcs) impl_->npcs->Pause();
            impl_->pending_frame = false;
            impl_->pending_edit.reset();
            impl_->state = SessionState::Paused;
        } catch (...) { impl_->state = SessionState::Failed; throw; }
    }
    void SimulationSession::Resume() {
        if (impl_->state != SessionState::Paused) throw Rejected("Resume: session must be paused");
        impl_->budget.Reset();
        impl_->resume_pending = true;
        impl_->state = SessionState::Ready;
    }

    AdvanceResult SimulationSession::Advance(double seconds, const AdvanceInput& input,
                                              std::span<const PointerInput> pointer) {
        SYMOCRAFT_PROFILE_ZONE("simulation.advance");
        if (impl_->state != SessionState::Ready) throw Rejected("Advance: session is not Ready");
        if (impl_->pending_frame) throw Rejected("Advance: previous NPC frame requires commit or pause");
        if (impl_->frame_id == (std::numeric_limits<std::uint64_t>::max)()) throw Rejected("Frame identity exhausted");
        if (!std::isfinite(seconds) || seconds < 0) throw Rejected("Advance: seconds must be finite/nonnegative");
        const PointerInput aggregate{input.player.mouse_dx, input.player.mouse_dy, input.player.scroll_y};
        CheckPointer(aggregate);
        for (const auto& event : pointer) CheckPointer(event);
        if (!pointer.empty() && (aggregate.mouse_dx != 0 || aggregate.mouse_dy != 0 || aggregate.scroll_y != 0))
            throw Rejected("Advance: ordered pointer events and aggregate cannot both be applied");
        const float delta = impl_->resume_pending ? 0.0f : static_cast<float>(std::min(seconds, 0.1));
        auto stage = AdvanceStage::Input;
        AdvanceStatistics statistics{delta};
        try {
            impl_->RequirePlayer();
            impl_->edit_cooldown -= delta;
            for (const auto& event : pointer)
                ApplyPointerInput(impl_->registry, impl_->player, impl_->lens, event.mouse_dx, event.mouse_dy, event.scroll_y);
            if (pointer.empty()) ApplyPointerInput(impl_->registry, impl_->player, impl_->lens,
                aggregate.mouse_dx, aggregate.mouse_dy, aggregate.scroll_y);
            ApplyControlInput(impl_->registry, impl_->player, input.player, impl_->controls, delta);
            stage = AdvanceStage::Character;
            if (impl_->npcs) impl_->npcs->BeforeAdvance(delta);
            TransformSystem::Update(impl_->registry);
            Character::Player::Update(impl_->registry);
            stage = AdvanceStage::Physics;
            const auto steps = impl_->budget.Consume(delta);
            for (unsigned step = 0; step < steps; ++step) {
                if (impl_->npcs) impl_->npcs->BeforePhysics();
                Detail::StepBodies(impl_->registry, impl_->world, statistics.body_commits);
                if (impl_->npcs) impl_->npcs->AfterPhysics();
                ++statistics.physical_steps;
            }
            stage = AdvanceStage::Recovery;
            auto& transform = impl_->registry.GetComponent<Transform>(impl_->player);
            if (transform.position.y < -8.0f) {
                transform.position = impl_->world.FindSpawn();
                auto& body = impl_->registry.GetComponent<Physics::RigidBody>(impl_->player);
                body.zero_forces(); body.on_ground = false;
                auto& character = impl_->registry.GetComponent<Character::CharacterComponent>(impl_->player);
                character.is_jumping = false; character.apply_jump_force = false;
                impl_->budget.Reset();
                statistics.recovered = true;
            }
            stage = AdvanceStage::Interaction;
            const auto lens = impl_->CurrentLens();
            const auto hit = Physics::RayCast(transform.position + lens.eye_offset, transform.front, 3.0f,
                                              Detail::CollisionQuery(impl_->world));
            RaycastStaticResult legacy_hit{};
            legacy_hit.hit = hit.hit;
            legacy_hit.block_center = glm::vec3(hit.cell) + glm::vec3(0.5f);
            legacy_hit.hit_normal = hit.normal;
            const auto interaction = PlayerController::EvaluateHit(impl_->registry, impl_->world, impl_->player,
                legacy_hit, {input.allow_interaction, input.place, input.remove, impl_->controls.selected_block},
                impl_->edit_cooldown);
            stage = AdvanceStage::Result;
            // Caller owns this small result; later advances cannot mutate it. Revisit an
            // output buffer only after measured, persistent result-transfer cost (D09).
            AdvanceResult result{ReadPose(impl_->registry, impl_->player), lens, {}, interaction.edit, statistics};
            if (hit.hit) result.selection = SelectionHit{hit.cell, hit.normal, hit.distance};
            result.frame_owner = impl_->identity;
            result.frame_id = ++impl_->frame_id;
            if (impl_->npcs && !impl_->npcs->Empty()) {
                impl_->pending_edit = interaction.edit;
                impl_->pending_frame = true;
            }
            impl_->resume_pending = false;
            return result;
        } catch (...) {
            impl_->state = SessionState::Failed;
            std::throw_with_nested(AdvanceFailed(stage, statistics.physical_steps, statistics.body_commits));
        }
    }
    PlayerView DeriveView(const PlayerPose& pose, const LensState& lens, double aspect_ratio) {
        if (!std::isfinite(aspect_ratio) || aspect_ratio <= 0 || aspect_ratio > (std::numeric_limits<float>::max)() ||
            !PlayerMath::IsFinite(pose.position) || !PlayerMath::IsFinite(pose.forward) || !PlayerMath::IsFinite(pose.up) ||
            !PlayerMath::IsFinite(lens.eye_offset) || !std::isfinite(lens.fov) || lens.fov <= 0 || lens.fov >= 180 ||
            !std::isfinite(lens.near_plane) || !std::isfinite(lens.far_plane) ||
            lens.near_plane <= 0 || lens.far_plane <= lens.near_plane)
            throw Rejected("DeriveView: invalid pose, lens or aspect ratio");
        const auto eye = pose.position + lens.eye_offset;
        if (!PlayerMath::IsFinite(eye)) throw Rejected("DeriveView: eye position overflow");
        return {eye, pose.forward, pose.up, lens.fov, lens.near_plane, lens.far_plane};
    }
    PlayerView SimulationSession::View() const {
        impl_->RequirePlayer();
        return DeriveView(ReadPose(impl_->registry, impl_->player), impl_->CurrentLens(), 1.0);
    }
    Detail::SessionDiagnostics Detail::SessionDiagnosticsAdapter::Capture(const SimulationSession& session) {
        const auto& state = *session.impl_;
        SessionDiagnostics result{state.state, state.budget.Remainder(), state.edit_cooldown,
            state.controls.block_change_debounce, state.controls.selected_block, state.resume_pending, {}};
        if (state.registry.GetStatus().health == ECS::RegistryHealth::Failed) return result;
        try {
            state.RequirePlayer();
            const auto& body = state.registry.GetComponent<Physics::RigidBody>(state.player);
            const auto& character = state.registry.GetComponent<Character::CharacterComponent>(state.player);
            if (PlayerMath::IsFinite(body.velocity) && PlayerMath::IsFinite(body.acceleration))
                result.motion = MotionDiagnostics{ReadPose(state.registry, state.player), body, character, state.CurrentLens()};
        } catch (const ECS::RegistryError&) { /* No unsafe ECS access escapes into diagnostics. */ }
        catch (const std::runtime_error&) { /* Missing/unsafe storage is explicitly unavailable. */ }
        return result;
    }
}
