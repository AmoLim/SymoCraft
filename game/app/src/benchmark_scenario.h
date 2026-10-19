#pragma once
#include <symocraft/simulation/session.h>
#include <string>
#include <string_view>

namespace SymoCraft::Application::Detail {
    struct ScenarioFrame {
        Simulation::AdvanceInput input;
        std::optional<Simulation::PlayerPose> pose_override;
        std::uint64_t edits = 0;
        double lateness_ms = 0;
    };
    class BenchmarkScenario final {
    public:
        explicit BenchmarkScenario(std::string_view workload);
        ScenarioFrame BeforeAdvance(double elapsed, Simulation::PlayerPose pose, World::VoxelWorld& world);
        std::uint64_t AppliedEdits() const noexcept { return applied_edits_; }
    private:
        std::string workload_;
        std::uint64_t applied_edits_ = 0;
        bool walk_forward_ = true;
    };
}
