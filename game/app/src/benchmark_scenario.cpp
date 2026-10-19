#include "benchmark_scenario.h"
#include <symocraft/world/benchmark_workload.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace SymoCraft::Application::Detail {
    BenchmarkScenario::BenchmarkScenario(std::string_view workload) : workload_(workload) {
        if (workload_ != "static" && workload_ != "walk" && workload_ != "edit")
            throw std::invalid_argument("BenchmarkScenario: unknown workload");
    }
    ScenarioFrame BenchmarkScenario::BeforeAdvance(double elapsed, Simulation::PlayerPose pose, World::VoxelWorld& world) {
        if (!std::isfinite(elapsed) || elapsed < 0) throw std::invalid_argument("BenchmarkScenario: invalid elapsed time");
        ScenarioFrame frame;
        frame.input.allow_interaction = false;
        if (workload_ == "walk") {
            if (walk_forward_ && pose.position.x >= 20.5f) { pose.position.x = 20.5f; walk_forward_ = false; }
            if (!walk_forward_ && pose.position.x <= -20.5f) { pose.position.x = -20.5f; walk_forward_ = true; }
            pose.yaw = walk_forward_ ? 0.0f : 180.0f;
            frame.pose_override = pose;
            frame.input.player.forward = true;
        }
        if (workload_ == "edit") {
            const auto due = Benchmark::DueEdits(elapsed);
            while (applied_edits_ < due) {
                const auto edit = Benchmark::CycleEdit(applied_edits_);
                const auto before = world.QueryBlock(edit.position);
                if (before.status != World::BlockQueryStatus::Found || before.block.block_id != edit.before ||
                    !world.TryEdit({World::EditOperation::Set, edit.position, edit.after}).Accepted())
                    throw std::runtime_error("Benchmark edit precondition failed");
                frame.lateness_ms = std::max(frame.lateness_ms,
                    (elapsed - (applied_edits_ + 1) * Benchmark::EditInterval) * 1000.0);
                ++applied_edits_; ++frame.edits;
            }
        }
        return frame;
    }
}
