#include "demo_control.h"
#include <iostream>
#include <stdexcept>

using namespace SymoCraft::Application::Detail;
namespace { void Require(bool condition) { if (!condition) throw std::runtime_error("Demo control contract failed"); } }
int main() {
    try {
        DemoControl control;
        Require(control.State() == DemoState::Ready && !control.Playing() && !control.Contest() && control.Round() == 1);
        control.Complete();
        Require(control.State() == DemoState::Ready);
        control.Toggle(); Require(control.Playing());
        control.Toggle(); Require(control.State() == DemoState::Paused);
        control.Complete(); Require(control.State() == DemoState::Paused);
        // Focus restoration does not mutate application-owned manual pause policy.
        Require(!control.Playing());
        control.Toggle(); Require(control.Playing());
        control.Complete(); Require(control.State() == DemoState::Completed);
        control.Toggle(); Require(control.State() == DemoState::Completed);
        control.Restart(); Require(control.State() == DemoState::Ready && control.Round() == 2 && !control.Contest());
        control.Toggle(); control.Restart(true);
        Require(control.State() == DemoState::Ready && control.Contest() && control.Round() == 3);
        control.Toggle(); control.Toggle(); control.Restart(true);
        Require(control.State() == DemoState::Ready && !control.Contest() && control.Round() == 4);
        std::cout << "Demo start, manual pause, terminal state and fresh-round policy passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
