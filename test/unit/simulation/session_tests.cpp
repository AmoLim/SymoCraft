#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <symocraft/foundation/memory.h>
#include "session_diagnostics.h"
#include "registry_test_access.h"
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <cstdlib>
#include <new>
#include <iomanip>
#include <type_traits>

namespace {
    using namespace SymoCraft;
    using namespace Simulation;
    using Simulation::Detail::SessionDiagnostics;
    using Simulation::Detail::SessionDiagnosticsAdapter;
    const char* sequence = "setup";
    unsigned sequence_step = 0;
    thread_local int fail_new_after = -1;
    void Require(bool value, const char* message) {
        if (!value) {
            std::cerr << "Sequence=" << sequence << "; advance/commit=" << sequence_step << "; MSVC=" << _MSC_FULL_VER << '\n';
            throw std::runtime_error(message);
        }
    }
    void Near(double a, double b, double tolerance, const char* field) {
        if (!std::isfinite(a) || !std::isfinite(b) || std::abs(a - b) > tolerance) {
            std::cerr << "First difference: " << field << ": " << a << " / " << b << ", tolerance=" << tolerance << '\n';
            Require(false, "Session field comparison failed");
        }
    }
    void Near(glm::vec3 a, glm::vec3 b, const char* field) {
        for (int axis = 0; axis < 3; ++axis) Near(a[axis], b[axis], 1e-4, field);
    }
    template<class Value> void Exact(Value a, Value b, const char* field) {
        if (a == b) return;
        std::cerr << "First difference: " << field << ": ";
        if constexpr (std::is_enum_v<Value>) std::cerr << static_cast<int>(a) << " / " << static_cast<int>(b);
        else std::cerr << a << " / " << b;
        std::cerr << ", tolerance=0\n";
        Require(false, "Exact session comparison failed");
    }
    template<class Error, class Call> Error Throws(Call&& call) {
        try { call(); } catch (const Error& error) { return error; }
        throw std::runtime_error("Expected session failure not reported");
    }
    void Register(ECS::Registry& registry) {
        registry.RegisterComponent<Transform>("Transform");
        registry.RegisterComponent<PhysicsStepOrder>("PhysicsStepOrder");
        registry.RegisterComponent<Physics::RigidBody>("RigidBody");
        registry.RegisterComponent<Physics::HitBox>("HitBox");
        registry.RegisterComponent<Character::CharacterComponent>("Character");
        registry.RegisterComponent<Character::PlayerComponent>("Player");
    }
    struct Fixture {
        ECS::Registry registry;
        std::unique_ptr<World::VoxelWorld> world;
        std::unique_ptr<SimulationSession> session;
        ECS::EntityId player = ECS::null_entity;
        explicit Fixture(const World::BlockDefinition& definition, glm::vec3 position = {0.5f, 200.9f, 0.5f}) {
            Register(registry);
            world = World::VoxelWorld::Create(definition, {424242, 2, false});
            session = SimulationSession::Create(registry, *world, {position, 0, 0});
            std::size_t count = 0;
            registry.Each<const Character::PlayerComponent>([&](ECS::EntityId entity, const Character::PlayerComponent&) { player = entity; ++count; });
            Require(count == 1, "Factory did not produce exactly one player");
            count = 0;
            registry.Each<const Transform>([&](ECS::EntityId, const Transform&) { ++count; });
            Require(count == 1, "Factory retained a duplicate camera entity");
        }
        SessionDiagnostics Capture() const { return SessionDiagnosticsAdapter::Capture(*session); }
        void Block(glm::ivec3 cell, uint16 id = 5) {
            Require(world->TryEdit({World::EditOperation::Set, cell, id}).Accepted(), "Fixture edit rejected");
        }
    };
    void Compare(const SessionDiagnostics& a, const SessionDiagnostics& b) {
        Exact(a.state, b.state, "state"); Exact(a.selected_block, b.selected_block, "selected material");
        Exact(a.resume_pending, b.resume_pending, "resume pending"); Exact(a.motion.has_value(), b.motion.has_value(), "motion available");
        Near(a.budget_seconds, b.budget_seconds, 1e-9, "budget");
        Near(a.edit_cooldown, b.edit_cooldown, 1e-6, "edit cooldown");
        Near(a.material_cooldown, b.material_cooldown, 1e-6, "material cooldown");
        if (!a.motion) return;
        const auto& x = *a.motion; const auto& y = *b.motion;
        Near(x.pose.position, y.pose.position, "position");
        Near(x.pose.forward, y.pose.forward, "forward"); Near(x.pose.up, y.pose.up, "up");
        Near(x.pose.yaw, y.pose.yaw, 1e-4, "yaw"); Near(x.pose.pitch, y.pose.pitch, 1e-4, "pitch");
        Near(x.lens.fov, y.lens.fov, 1e-4, "FOV");
        Near(x.lens.eye_offset, y.lens.eye_offset, "eye offset");
        Near(x.lens.near_plane, y.lens.near_plane, 0, "near"); Near(x.lens.far_plane, y.lens.far_plane, 0, "far");
        Near(x.body.velocity, y.body.velocity, "velocity"); Near(x.body.acceleration, y.body.acceleration, "acceleration");
        Exact(x.body.on_ground, y.body.on_ground, "grounded"); Exact(x.body.is_sensor, y.body.is_sensor, "sensor");
        Exact(x.body.use_gravity, y.body.use_gravity, "gravity"); Exact(x.character.is_jumping, y.character.is_jumping, "jumping");
        Exact(x.character.apply_jump_force, y.character.apply_jump_force, "pending jump");
        Exact(x.character.is_running, y.character.is_running, "running");
        Near(x.character.movement_axis, y.character.movement_axis, "movement axis");
    }
    void Compare(const AdvanceResult& a, const AdvanceResult& b) {
        Near(a.pose.position, b.pose.position, "result position");
        Near(a.pose.forward, b.pose.forward, "result forward"); Near(a.pose.up, b.pose.up, "result up");
        Near(a.pose.yaw, b.pose.yaw, 1e-4, "result yaw"); Near(a.pose.pitch, b.pose.pitch, 1e-4, "result pitch");
        Near(a.lens.fov, b.lens.fov, 1e-4, "result FOV");
        Near(a.lens.eye_offset, b.lens.eye_offset, "result eye offset");
        Exact(a.statistics.rule_seconds, b.statistics.rule_seconds, "rule seconds");
        Exact(a.statistics.physical_steps, b.statistics.physical_steps, "physical steps");
        Exact(a.statistics.body_commits, b.statistics.body_commits, "body commits");
        Exact(a.statistics.recovered, b.statistics.recovered, "recovered");
        Exact(a.selection.has_value(), b.selection.has_value(), "hit present");
        Exact(a.edit.has_value(), b.edit.has_value(), "request present");
        if (a.selection) {
            for (int axis = 0; axis < 3; ++axis) {
                Exact(a.selection->cell[axis], b.selection->cell[axis], "hit cell");
                Exact(a.selection->normal[axis], b.selection->normal[axis], "hit normal");
            }
            Near(a.selection->distance, b.selection->distance, 1e-4, "ray distance");
        }
        if (a.edit) {
            Exact(a.edit->operation, b.edit->operation, "edit operation"); Exact(a.edit->id, b.edit->id, "edit block");
            for (int axis = 0; axis < 3; ++axis) Exact(a.edit->position[axis], b.edit->position[axis], "edit coordinate");
        }
    }
    void Commit(Fixture& a, Fixture& b, World::EditRequest edit) {
        const auto x = a.world->TryEdit(edit), y = b.world->TryEdit(edit);
        const auto left = a.world->QueryBlock(edit.position), right = b.world->QueryBlock(edit.position);
        Exact(x.status, y.status, "commit status"); Exact(x.rejection, y.rejection, "commit rejection");
        Exact(left.status, right.status, "query status"); Exact(left.block.block_id, right.block.block_id, "queried block ID");
        Require(left.block == right.block, "World related block metadata differs");
        Exact(a.world->Digest(), b.world->Digest(), "world digest");
        Compare(a.Capture(), b.Capture());
    }
    AdvanceResult AdvancePair(Fixture& a, Fixture& b, double seconds, const AdvanceInput& input,
                              std::span<const PointerInput> pointer = {}) {
        const auto left = a.session->Advance(seconds, input, pointer);
        const auto diagnostic = a.Capture();
        static_cast<void>(DeriveView(left.pose, left.lens, 16.0 / 9.0));
        static_cast<void>(a.session->View()); static_cast<void>(a.session->View());
        Compare(diagnostic, a.Capture());
        const auto right = b.session->Advance(seconds, input, pointer);
        Compare(left, right); Compare(a.Capture(), b.Capture());
        if (left.edit) Commit(a, b, *left.edit);
        return left;
    }
    void FactoryAndTiming(const World::BlockDefinition& definition) {
        ECS::Registry missing;
        Throws<Rejected>([&] { SimulationSession::Create(missing, *World::VoxelWorld::Create(definition, {424242, 2, false})); });
        Fixture a(definition), b(definition);
        const auto initial_b = b.Capture();
        AdvanceInput input; input.player.forward = input.player.jump = input.player.next_block = true;
        auto& body = a.registry.GetComponent<Physics::RigidBody>(a.player);
        body.on_ground = true;
        const auto jump = a.session->Advance(0, input);
        Require(jump.statistics.physical_steps == 0 && a.Capture().motion->character.is_jumping &&
            !a.Capture().motion->body.on_ground, "Zero-step jump was deferred");
        Near(a.Capture().motion->body.velocity.y, 7.6, 1e-4, "zero-step jump");
        a.session->Advance(0.002, input);
        Require(a.Capture().budget_seconds > 0, "Short frame lost remainder");
        Compare(initial_b, b.Capture());
        const auto before_pause = a.Capture();
        a.session->Pause();
        Require(a.session->State() == SessionState::Paused && a.Capture().budget_seconds == 0, "Pause failed");
        Near(a.Capture().material_cooldown, before_pause.material_cooldown, 0, "pause keeps material cooldown");
        Throws<Rejected>([&] { a.session->Advance(0.1, input); });
        a.session->Resume();
        const auto recovered = a.session->Advance(100, input);
        Require(recovered.statistics.rule_seconds == 0 && recovered.statistics.physical_steps == 0, "Resume caught up paused time");
        const auto long_frame = a.session->Advance(100, input);
        Require(long_frame.statistics.rule_seconds == 0.1f && long_frame.statistics.physical_steps == 8, "Two-layer timing changed");
        Compare(initial_b, b.Capture());
        const auto owned = b.session->Advance(0, {});
        b.session.reset();
        Require(b.registry.IsEntityValid(b.player) && b.world->ChunkCount() != 0,
            "Session destructor deleted caller-owned player/storage");
        Near(owned.pose.position, initial_b.motion->pose.position, "owned result after session destruction");
    }
    void FactoryAllocationFailure(const World::BlockDefinition& definition) {
        sequence = "factory-allocation-discard";
        auto world = World::VoxelWorld::Create(definition, {424242, 2, false});
        unsigned failures = 0;
        bool succeeded = false;
        // Six real object pools have more preparation allocations than the old byte pools.
        for (int budget = 0; budget < 64 && !succeeded; ++budget) {
            sequence_step = static_cast<unsigned>(budget);
            ECS::Registry registry;
            Register(registry);
            std::unique_ptr<SimulationSession> candidate;
            fail_new_after = budget;
            try { candidate = SimulationSession::Create(registry, *world); succeeded = true; }
            catch (const std::bad_alloc&) { ++failures; }
            fail_new_after = -1;
            // No shared-storage rollback/retry: discard this dedicated assembly even after a partial player.
            candidate.reset();
        }
        Require(succeeded && failures >= 2, "Factory allocation sweep did not reach owned publication failures");
    }
    void RejectionAndOwnership(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto before = f.Capture();
        AdvanceInput input;
        const std::array<PointerInput, 2> invalid{{{0, 100, 0}, {std::numeric_limits<double>::infinity(), 0, 0}}};
        for (double seconds : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            Throws<Rejected>([&] { f.session->Advance(seconds, input); }); Compare(before, f.Capture());
        }
        Throws<Rejected>([&] { f.session->Advance(0.1, input, invalid); }); Compare(before, f.Capture());
        const std::array<PointerInput, 4> ordered{{{0, 2000, 50}, {0, -2000, -10}, {20, 0, 0}, {-20, 0, 0}}};
        const auto old = f.session->Advance(0, input, ordered);
        Near(old.pose.pitch, -11, 1e-4, "ordered pitch clip"); Near(old.lens.fov, 11, 1e-4, "ordered FOV clip");
        const auto stable = f.Capture();
        for (double ratio : {0.0, -1.0, std::numeric_limits<double>::infinity()}) {
            Throws<Rejected>([&] { DeriveView(old.pose, old.lens, ratio); }); Compare(stable, f.Capture());
        }
        for (int repeat = 0; repeat < 2; ++repeat) { static_cast<void>(f.session->View()); Compare(stable, f.Capture()); }
        input.player.sensor = input.player.forward = true;
        f.session->Advance(0.1, input);
        Near(old.pose.position, before.motion->pose.position, "old owned pose");
        Near(old.lens.fov, 11, 1e-4, "old owned lens");
    }
    void PartialFailure(const World::BlockDefinition& definition) {
        Fixture f(definition);
        const auto extra = f.registry.CreateEntity();
        f.registry.Emplace<Transform>(extra).position = {5, 210, 5};
        f.registry.Emplace<Physics::RigidBody>(extra).velocity = {1, 0, 0};
        f.registry.Emplace<Physics::HitBox>(extra).size = glm::vec3(-1);
        f.registry.Emplace<PhysicsStepOrder>(extra, std::uint64_t{1});
        AdvanceInput input; input.player.sensor = input.player.forward = true;
        const auto failed_body_before = f.registry.GetComponent<Transform>(extra).position;
        const auto failure = Throws<AdvanceFailed>([&] { f.session->Advance(1.0 / 120.0, input); });
        Require(failure.Stage() == AdvanceStage::Physics && failure.CompletedSteps() == 0 && failure.BodyCommits() == 1 &&
            f.session->State() == SessionState::Failed, "Partial commit progress was lost");
        Near(f.registry.GetComponent<Transform>(extra).position, failed_body_before, "failed body not committed");
        Require(f.Capture().motion->pose.position.x > 0.5f, "Prior body commit was rolled back");
        const auto after = f.Capture();
        Throws<Rejected>([&] { f.session->Advance(0, input); }); Compare(after, f.Capture());
        f.registry.Clear();
        Require(!f.Capture().motion, "Failed diagnostics invented missing motion");
    }
    void FailedRegistryDiagnostics(const World::BlockDefinition& definition) {
        sequence = "failed-registry-diagnostics";
        Fixture f(definition);
        ECS::Detail::RegistryTestAccess::SimulatePostClearConsistencyFailure(f.registry);
        Require(!f.Capture().motion, "Failed Registry diagnostics accessed unknown native storage");
        const auto failure = Throws<AdvanceFailed>([&] { f.session->Advance(1.0 / 120.0, {}); });
        Require(failure.BodyCommits() == 0 && f.session->State() == SessionState::Failed && !f.Capture().motion,
                "Failed Registry did not preserve safe Session diagnostics");
    }
    void ExplicitBodyOrder(const World::BlockDefinition& definition) {
        sequence = "explicit-body-order";
        for (int mode = 0; mode < 4; ++mode) {
            Fixture f(definition);
            const auto extra = f.registry.CreateEntity();
            f.registry.Emplace<Transform>(extra).position = {5, 210, 5};
            f.registry.Emplace<Physics::RigidBody>(extra);
            f.registry.Emplace<Physics::HitBox>(extra).size = glm::vec3(-1);
            if (mode != 0) f.registry.Emplace<PhysicsStepOrder>(extra, std::uint64_t{mode == 1 ? 0u : 1u});
            if (mode == 2) f.registry.GetComponent<PhysicsStepOrder>(f.player).value = 2;
            // Deliberately repack one driver pool; logical order must remain assembly-owned.
            const auto player_body = f.registry.GetComponent<Physics::RigidBody>(f.player);
            f.registry.RemoveComponent<Physics::RigidBody>(f.player);
            f.registry.Emplace<Physics::RigidBody>(f.player, player_body);
            const auto before = f.registry.GetComponent<Transform>(f.player).position;
            AdvanceInput input; input.player.sensor = input.player.forward = true;
            const auto failure = Throws<AdvanceFailed>([&] { f.session->Advance(1.0 / 120.0, input); });
            Require(failure.Stage() == AdvanceStage::Physics && failure.CompletedSteps() == 0 &&
                failure.BodyCommits() == (mode == 3 ? 1u : 0u), "Explicit body key did not control failure prefix");
            if (mode != 3) Near(f.registry.GetComponent<Transform>(f.player).position, before, "zero physical commits before key/first-body failure");
            Require(f.session->State() == SessionState::Failed, "Body-order failure did not stop Session");
        }
    }
    void PairSequence(const World::BlockDefinition& definition) {
        sequence = "mixed-input-edits-pause";
        Fixture a(definition), b(definition);
        for (auto* scene : {&a, &b}) {
            scene->Block({0, 199, 0}); scene->Block({1, 200, 0});
            scene->Block({2, 201, 0}); scene->Block({0, 203, 0});
        }
        Require(a.world->Digest() == b.world->Digest(), "Initial world state differs");
        AdvanceInput input;
        std::array<PointerInput, 2> pointer{};
        for (unsigned step = 0; step < 48; ++step) {
            sequence_step = step;
            input.player.forward = step < 18;
            input.player.backward = step >= 18 && step < 24;
            input.player.right = step >= 24 && step < 30;
            input.player.jump = step == 2 || step == 12;
            input.player.sensor = step >= 30 && step < 35;
            input.player.next_block = step == 4 || step == 5 || step == 20;
            input.player.previous_block = step == 21;
            input.place = step >= 8 && step < 14;
            input.remove = step >= 14 && step < 18;
            pointer = {{{step == 36 ? 100.0 : 0, step == 36 ? 2000.0 : 0, step == 36 ? 50.0 : 0},
                        {0, step == 36 ? -2000.0 : 0, step == 36 ? -10.0 : 0}}};
            if (step == 6) {
                const World::EditRequest edit{World::EditOperation::Remove, {0, 199, 0}};
                Commit(a, b, edit);
            }
            if (step == 26) { a.session->Pause(); b.session->Pause(); Compare(a.Capture(), b.Capture()); a.session->Resume(); b.session->Resume(); }
            const double delta = step % 7 == 0 ? 0 : step % 5 == 0 ? 0.3 : step % 3 == 0 ? 0.002 : 1.0 / 60.0;
            AdvancePair(a, b, delta, input, pointer);
        }
        Near(a.Capture().budget_seconds, b.Capture().budget_seconds, 1e-9, "final pair budget");
    }
    void ContactAndFailurePairs(const World::BlockDefinition& definition) {
        struct Case { const char* name; float x; bool wall, corner, ceiling; };
        for (const auto item : {Case{"floor-wall-corner", .5f, true, true, false},
             Case{"low-ceiling-jump", .5f, false, false, true}, Case{"negative-chunk-boundary", -16.5f, false, false, false},
             Case{"positive-chunk-boundary", 15.5f, false, false, false}}) {
            sequence = item.name;
            Fixture a(definition, {item.x, 200.9f, .5f}), b(definition, {item.x, 200.9f, .5f});
            const int origin = static_cast<int>(std::floor(item.x));
            sequence_step = 0;
            for (int x = origin - 2; x <= origin + 5; ++x) for (int z = -2; z <= 3; ++z) {
                Commit(a, b, {World::EditOperation::Set, {x, 199, z}, 5});
                if (item.ceiling) Commit(a, b, {World::EditOperation::Set, {x, 202, z}, 5});
            }
            for (int y = 200; y <= 202; ++y) for (int along = -2; along <= 3; ++along) {
                if (item.wall) Commit(a, b, {World::EditOperation::Set, {origin + 2, y, along}, 5});
                if (item.corner) Commit(a, b, {World::EditOperation::Set, {origin + along, y, 2}, 5});
            }
            AdvanceInput input;
            AdvancePair(a, b, .1, input);
            for (unsigned index = 0; index < 40; ++index) {
                sequence_step = index;
                input.player.forward = true; input.player.right = item.corner;
                input.player.jump = item.ceiling && (index == 1 || index == 25);
                input.player.run = index >= 30;
                AdvancePair(a, b, index % 11 == 0 ? .2 : 1.0 / 60.0, input);
            }
            if (item.wall) Require(a.Capture().motion->pose.position.x < origin + 2, "Wall sequence never constrained motion");
            else if (!item.ceiling) Require(a.Capture().motion->pose.position.x > item.x + 2, "Boundary sequence did not cross chunk seam");
        }
        sequence = "placement-rejection-cooldown-and-world-rejection"; sequence_step = 0;
        Fixture a(definition, {.5f, 200.9f, .5f}), b(definition, {.5f, 200.9f, .5f});
        Commit(a, b, {World::EditOperation::Set, {1, 201, 0}, 5});
        AdvanceInput input; input.place = true;
        auto result = AdvancePair(a, b, 0, input);
        Require(result.selection && !result.edit && a.Capture().edit_cooldown > 0, "Overlap rejection did not consume cooldown");
        input.place = false; input.remove = true;
        Require(!AdvancePair(a, b, .01, input).edit, "Rejected placement cooldown did not block removal");
        AdvancePair(a, b, .1, input);
        Require(AdvancePair(a, b, .1, input).edit.has_value(), "Cooldown sequence never produced a removal");
        Commit(a, b, {World::EditOperation::Set, {0, 200, 0}, 65535});
        Commit(a, b, {World::EditOperation::Remove, {100000, 200, 0}});
        sequence = "preflight-correction-recovery-owned-result";
        const auto before_a = a.Capture(), before_b = b.Capture();
        for (double invalid : {-1.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
            Throws<Rejected>([&] { a.session->Advance(invalid, input); });
            Throws<Rejected>([&] { b.session->Advance(invalid, input); });
            Compare(before_a, a.Capture()); Compare(before_b, b.Capture()); Compare(a.Capture(), b.Capture());
        }
        const std::array<PointerInput, 2> invalid{{{0, 1, 0}, {0, 0, std::numeric_limits<double>::max()}}};
        Throws<Rejected>([&] { a.session->Advance(.1, input, invalid); });
        Throws<Rejected>([&] { b.session->Advance(.1, input, invalid); });
        Compare(before_a, a.Capture()); Compare(before_b, b.Capture());
        const auto old = AdvancePair(a, b, 0, input);
        for (auto* fixture : {&a, &b}) fixture->registry.GetComponent<Transform>(fixture->player).position.y = -9;
        const auto recovery = AdvancePair(a, b, 0, {});
        Require(recovery.statistics.recovered && a.Capture().budget_seconds == 0 &&
            !a.Capture().motion->character.is_jumping, "Recovery sequence did not clear timing/jump");
        Near(old.pose.position, before_a.motion->pose.position, "held result after recovery");
        const auto safe = a.Capture();
        Throws<Rejected>([&] { DeriveView(recovery.pose, recovery.lens, 0); }); Compare(safe, a.Capture());
        sequence = "partial-body-commit-failed";
        for (auto* fixture : {&a, &b}) {
            const auto extra = fixture->registry.CreateEntity();
            fixture->registry.Emplace<Transform>(extra).position = {5, 210, 5};
            fixture->registry.Emplace<Physics::RigidBody>(extra).velocity = {1, 0, 0};
            fixture->registry.Emplace<Physics::HitBox>(extra).size = glm::vec3(-1);
            fixture->registry.Emplace<PhysicsStepOrder>(extra, std::uint64_t{1});
        }
        input = {}; input.player.sensor = input.player.forward = true;
        const auto left = Throws<AdvanceFailed>([&] { a.session->Advance(.1, input); });
        const auto right = Throws<AdvanceFailed>([&] { b.session->Advance(.1, input); });
        Exact(left.Stage(), right.Stage(), "failure stage"); Exact(left.CompletedSteps(), right.CompletedSteps(), "failure steps");
        Exact(left.BodyCommits(), right.BodyCommits(), "failure commits"); Require(left.BodyCommits() == 1, "Expected prior body commit missing");
        Compare(a.Capture(), b.Capture());
        Throws<Rejected>([&] { a.session->Advance(0, input); });
        Throws<Rejected>([&] { b.session->Advance(0, input); }); Compare(a.Capture(), b.Capture());
    }
}
void* operator new(std::size_t bytes) {
    if (fail_new_after == 0) throw std::bad_alloc();
    if (fail_new_after > 0) --fail_new_after;
    auto* result = std::malloc(bytes ? bytes : 1);
    if (!result) throw std::bad_alloc();
    return result;
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
int main(int argc, char** argv) {
    std::set_terminate([] { std::_Exit(86); });
    std::cerr << std::setprecision(17);
    try {
        Require(argc == 2, "Expected production block configuration");
        AmoBase::AmoMemory_Init(true, 1024);
        std::ifstream file(argv[1], std::ios::binary);
        Require(static_cast<bool>(file), "Cannot read block configuration");
        const auto definition = SymoCraft::World::BlockDefinition::FromConfig(
            std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
        FactoryAndTiming(definition);
        FactoryAllocationFailure(definition);
        RejectionAndOwnership(definition);
        PartialFailure(definition);
        ExplicitBodyOrder(definition);
        FailedRegistryDiagnostics(definition);
        PairSequence(definition);
        ContactAndFailurePairs(definition);
        std::cout << "Session: factory/timing, rejection/ownership, partial failure and per-advance pair comparison passed\n";
        return 0;
    } catch (const std::exception& error) { fail_new_after = -1; std::cerr << error.what() << '\n'; return 1; }
}
