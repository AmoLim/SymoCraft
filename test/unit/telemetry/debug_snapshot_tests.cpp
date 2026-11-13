#include <symocraft/telemetry/debug_snapshot.h>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
    void Require(bool value) { if (!value) throw std::runtime_error("Debug snapshot contract failed"); }
}
int main() {
    using namespace SymoCraft::Telemetry;
    using namespace std::chrono_literals;
    try {
        DebugObserver observer;
        const DebugClock::time_point zero{};
        const std::array<double, 3> point{1, 2, 3};
        Require(observer.Snapshot().state == DebugState::Unavailable);
        observer.Observe(zero, true, 0, point, DebugBackend::D3D12);
        Require(!observer.Snapshot().fps && !observer.Snapshot().frame_interval_ms);
        observer.Observe(zero + 250ms, true, 0, point, DebugBackend::D3D12);
        Require(observer.Snapshot().frame_interval_ms == 250.0 && !observer.Snapshot().fps);
        observer.Observe(zero + 1s, true, 7, point, DebugBackend::D3D12);
        const auto owned = observer.Snapshot();
        Require(owned.fps == 2.0 && owned.physical_tps == 7.0 && owned.position == point);
        observer.Observe(zero + 2s, true, 0, point, DebugBackend::D3D12);
        Require(observer.Snapshot().physical_tps == 0.0 && owned.physical_tps == 7.0);
        observer.Pause();
        const auto paused = observer.Snapshot();
        Require(paused.state == DebugState::Paused && !paused.fps && !paused.physical_tps && !paused.frame_interval_ms);
        Require(paused.position == point && owned.state == DebugState::Active);
        observer.Observe(zero + 100s, true, 8, point, DebugBackend::Vulkan);
        Require(!observer.Snapshot().frame_interval_ms && !observer.Snapshot().physical_tps);
        observer.Observe(zero + 101s, true, 2, point, DebugBackend::Vulkan);
        Require(observer.Snapshot().physical_tps == 2.0 && observer.Snapshot().backend == DebugBackend::Vulkan);
        observer.Observe(zero + 102s, false, 0, point, DebugBackend::Vulkan);
        Require(observer.Snapshot().state == DebugState::Unavailable && !observer.Snapshot().fps);
        observer.Observe(zero + 103s, true, 0, {}, DebugBackend::Vulkan);
        Require(!observer.Snapshot().position && !observer.Snapshot().frame_interval_ms);
        observer.Observe(zero + 103s, true, 0, point, DebugBackend::Vulkan);
        Require(!observer.Snapshot().fps && !observer.Snapshot().frame_interval_ms);
        const std::array<double, 3> invalid{1, std::numeric_limits<double>::quiet_NaN(), 2};
        observer.Observe(zero + 104s, true, 0, invalid, DebugBackend::Vulkan);
        Require(!observer.Snapshot().position);
        observer.Pause();
        for (unsigned i = 0; i <= 500000; ++i)
            observer.Observe(zero + std::chrono::milliseconds(i), true, 1, point, DebugBackend::OpenGL);
        const auto long_run = observer.Snapshot();
        Require(long_run.fps == 1000.0 && long_run.physical_tps == 1000.0 && long_run.frame_interval_ms == 1.0);
        std::cout << "Owned debug rates, missing values, skipped/paused/recovery and 500000 observations passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
