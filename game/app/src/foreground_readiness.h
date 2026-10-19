#pragma once
#include <chrono>
#include <optional>

namespace SymoCraft::Application::Detail {
    class ForegroundReadiness {
    public:
        using Clock = std::chrono::steady_clock;
        enum class State { Waiting, Ready, TimedOut };
        ForegroundReadiness(Clock::time_point start, std::chrono::seconds timeout)
            : deadline_(start + timeout) {}

        State Observe(Clock::time_point now, bool focused_drawable) {
            if (now >= deadline_) return State::TimedOut;
            if (!focused_drawable) stable_since_.reset();
            else if (!stable_since_) stable_since_ = now;
            else if (now - *stable_since_ >= std::chrono::seconds(1)) return State::Ready;
            return State::Waiting;
        }
    private:
        Clock::time_point deadline_;
        std::optional<Clock::time_point> stable_since_;
    };
}
