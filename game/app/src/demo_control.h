#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace SymoCraft::Application::Detail {
    enum class DemoState { Ready, Playing, Paused, Completed };
    inline const char* DemoStateName(DemoState state) noexcept {
        switch (state) {
        case DemoState::Ready: return "READY";
        case DemoState::Playing: return "PLAYING";
        case DemoState::Paused: return "PAUSED";
        case DemoState::Completed: return "COMPLETED";
        }
        return "UNKNOWN";
    }

    // Only application policy: authoritative simulation remains in the session.
    class DemoControl final {
    public:
        DemoState State() const noexcept { return state_; }
        bool Playing() const noexcept { return state_ == DemoState::Playing; }
        bool Contest() const noexcept { return contest_; }
        std::uint64_t Round() const noexcept { return round_; }
        void Toggle() noexcept {
            if (state_ == DemoState::Ready || state_ == DemoState::Paused) state_ = DemoState::Playing;
            else if (state_ == DemoState::Playing) state_ = DemoState::Paused;
        }
        void Complete() noexcept { if (Playing()) state_ = DemoState::Completed; }
        void Restart(bool change_scene = false) {
            if (round_ == (std::numeric_limits<std::uint64_t>::max)())
                throw std::overflow_error("Demo round identity exhausted");
            ++round_;
            if (change_scene) contest_ = !contest_;
            state_ = DemoState::Ready;
        }
    private:
        DemoState state_ = DemoState::Ready;
        bool contest_ = false;
        std::uint64_t round_ = 1;
    };
}
