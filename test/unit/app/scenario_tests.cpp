#include "benchmark_scenario.h"
#include "shutdown_report.h"
#include <symocraft/world/test_scene.h>
#include <symocraft/world/benchmark_workload.h>
#include <fstream>
#include <iostream>
#include <system_error>

namespace {
    using namespace SymoCraft;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void Reports() {
        using namespace Application;
        ShutdownReport report;
        int attempts = 0;
        report.Attempt(ShutdownStep::Renderer, true, [&] { ++attempts; throw std::system_error(32, std::system_category(), "first native error"); });
        report.Attempt(ShutdownStep::Window, true, [&] { ++attempts; throw 7; });
        report.Attempt(ShutdownStep::Runtime, true, [&] { ++attempts; });
        report.Attempt(ShutdownStep::World, false, [&] { ++attempts; });
        Require(attempts == 3 && !report.Succeeded(), "Report skipped cleanup or claimed failure as success");
        ShutdownReport owned;
        try { throw ShutdownError(report); } catch (const ShutdownError& error) { owned = error.Report(); }
        report = {};
        Require(owned.items[static_cast<std::size_t>(ShutdownStep::World)].fact == ReleaseFact::NotAcquired &&
            owned.items[static_cast<std::size_t>(ShutdownStep::Runtime)].fact == ReleaseFact::Released,
            "Not-acquired and released facts were conflated");
        try { std::rethrow_exception(owned.items[static_cast<std::size_t>(ShutdownStep::Renderer)].error); }
        catch (const std::system_error& error) { Require(error.code().value() == 32, "Owned report lost native code"); }
        try { std::rethrow_exception(owned.items[static_cast<std::size_t>(ShutdownStep::Window)].error); }
        catch (int value) { Require(value == 7, "Owned report lost secondary failure"); }
    }
}
int main(int argc, char** argv) {
    using namespace SymoCraft;
    try {
        Require(argc == 2, "Expected production block configuration"); Reports();
        std::ifstream file(argv[1], std::ios::binary);
        const auto definition = World::BlockDefinition::FromConfig(std::string(std::istreambuf_iterator<char>(file), {}));
        auto world = World::VoxelWorld::Create(definition, {424242, 3, false});
        TestScene::Install(*world);
        const auto original = world->Digest();
        Application::Detail::BenchmarkScenario stationary("static"), walk("walk"), edit("edit");
        Simulation::PlayerPose pose{{20.6f, 161, -6.5f}, 0, 0};
        const auto still = stationary.BeforeAdvance(20, pose, *world);
        Require(!still.input.allow_interaction && !still.input.player.forward && !still.pose_override && still.edits == 0 &&
            world->Digest() == original, "Static scenario changed gameplay state");
        const auto forward = walk.BeforeAdvance(0, pose, *world);
        Require(forward.input.player.forward && forward.pose_override->position.x == 20.5f && forward.pose_override->yaw == 180,
            "Walk endpoint reversal/clamp changed");
        pose.position.x = -20.6f;
        const auto back = walk.BeforeAdvance(1, pose, *world);
        Require(back.pose_override->position.x == -20.5f && back.pose_override->yaw == 0, "Reverse endpoint changed");
        const auto changed = edit.BeforeAdvance(0.51, pose, *world);
        Require(changed.edits == 2 && changed.lateness_ms >= 250 && edit.AppliedEdits() == 2, "Real-time edit schedule changed");
        const auto expected = Benchmark::CycleEdit(1);
        Require(world->QueryBlock(expected.position).block.block_id == expected.after, "Scenario did not commit before simulation");
        Require(edit.BeforeAdvance(0.51, pose, *world).edits == 0, "Scenario repeated an applied edit");
        std::cout << "Owned shutdown reports and independent static/walk/edit scenarios passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
