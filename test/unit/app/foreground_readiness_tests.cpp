#include "foreground_readiness.h"
#include <iostream>
#include <stdexcept>

int main() {
    using Gate = SymoCraft::Application::Detail::ForegroundReadiness;
    using namespace std::chrono_literals;
    const Gate::Clock::time_point start{};
    const auto require = [](bool value) { if (!value) throw std::runtime_error("Foreground readiness contract failed"); };
    try {
        Gate stable(start, 60s);
        require(stable.Observe(start, false) == Gate::State::Waiting);
        require(stable.Observe(start + 3s, true) == Gate::State::Waiting);
        require(stable.Observe(start + 3500ms, true) == Gate::State::Waiting);
        require(stable.Observe(start + 4s, true) == Gate::State::Ready);
        Gate interrupted(start, 60s);
        require(interrupted.Observe(start, true) == Gate::State::Waiting);
        require(interrupted.Observe(start + 500ms, false) == Gate::State::Waiting);
        require(interrupted.Observe(start + 1s, true) == Gate::State::Waiting);
        require(interrupted.Observe(start + 2s, true) == Gate::State::Ready);
        Gate timeout(start, 2s);
        require(timeout.Observe(start + 1s, false) == Gate::State::Waiting);
        require(timeout.Observe(start + 2s, false) == Gate::State::TimedOut);
        std::cout << "Foreground readiness stable, restart and bounded timeout basic cases passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
