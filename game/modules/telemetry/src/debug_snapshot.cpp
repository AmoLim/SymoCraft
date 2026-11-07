#include <symocraft/telemetry/debug_snapshot.h>
#include <cmath>
#include <limits>

namespace SymoCraft::Telemetry {
    void DebugObserver::ResetRates() noexcept {
        snapshot_.fps.reset(); snapshot_.frame_interval_ms.reset(); snapshot_.physical_tps.reset();
        window_start_.reset(); previous_present_.reset(); frames_ = 0; steps_ = 0;
    }
    void DebugObserver::Pause() noexcept {
        ResetRates(); snapshot_.state = DebugState::Paused;
    }
    void DebugObserver::Observe(DebugClock::time_point now, bool presented, unsigned physical_steps,
        std::optional<std::array<double, 3>> position, DebugBackend backend) noexcept {
        snapshot_.backend = backend;
        snapshot_.position = position;
        if (position)
            for (const auto coordinate : *position)
                if (!std::isfinite(coordinate)) { snapshot_.position.reset(); break; }
        if (!presented) {
            ResetRates(); snapshot_.state = DebugState::Unavailable; return;
        }
        if (!previous_present_ || now <= *previous_present_) {
            ResetRates(); window_start_ = now; previous_present_ = now;
            snapshot_.state = DebugState::Active; return;
        }
        snapshot_.state = DebugState::Active;
        snapshot_.frame_interval_ms = std::chrono::duration<double, std::milli>(now - *previous_present_).count();
        previous_present_ = now;
        if (frames_ == (std::numeric_limits<std::uint64_t>::max)() ||
            steps_ > (std::numeric_limits<std::uint64_t>::max)() - physical_steps) {
            ResetRates(); window_start_ = now; previous_present_ = now; return;
        }
        ++frames_; steps_ += physical_steps;
        const auto seconds = std::chrono::duration<double>(now - *window_start_).count();
        if (seconds >= 1.0) {
            snapshot_.fps = static_cast<double>(frames_) / seconds;
            snapshot_.physical_tps = static_cast<double>(steps_) / seconds;
            window_start_ = now; frames_ = 0; steps_ = 0;
        }
    }
}
