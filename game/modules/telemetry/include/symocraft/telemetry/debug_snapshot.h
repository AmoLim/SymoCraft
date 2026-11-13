#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

namespace SymoCraft::Telemetry {
    using DebugClock = std::chrono::steady_clock;
    enum class DebugState { Unavailable, Active, Paused };
    enum class DebugBackend { Unknown, OpenGL, D3D12, Vulkan };
    struct DebugSnapshot {
        DebugState state = DebugState::Unavailable;
        DebugBackend backend = DebugBackend::Unknown;
        std::optional<double> fps, frame_interval_ms, physical_tps;
        std::optional<std::array<double, 3>> position;
    };

    // Value-only, single-threaded observer. A one-second window starts with a
    // successful present baseline; no per-frame history or business references.
    class DebugObserver final {
    public:
        void Observe(DebugClock::time_point now, bool presented, unsigned physical_steps,
                     std::optional<std::array<double, 3>> position, DebugBackend backend) noexcept;
        void Pause() noexcept;
        DebugSnapshot Snapshot() const noexcept { return snapshot_; }
    private:
        void ResetRates() noexcept;
        DebugSnapshot snapshot_;
        std::optional<DebugClock::time_point> window_start_, previous_present_;
        std::uint64_t frames_ = 0, steps_ = 0;
    };
}
