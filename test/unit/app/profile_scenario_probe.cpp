#include "application.h"
#include "startup_options.h"
#include <symocraft/world/test_scene.h>
#include <SDL3/SDL.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
    void Require(bool value, const char* text) { if (!value) throw std::runtime_error(text); }
}
int main(int argc, char** argv) {
    using namespace SymoCraft;
    try {
        Require(argc == 4, "Expected renderer, scenario and event mode");
        const std::string mode = argv[3];
        const std::string_view args[]{"--renderer", argv[1], "--profile-scenario", argv[2], "--scenario-seconds", "1"};
        const auto options = ParseStartupOptions(args);
        Application::Application app(options, std::chrono::steady_clock::now());
        app.Initialize();
        const auto initial = app.DebugValues();
        const auto expected = TestScene::Checkpoint(options.checkpoint);
        Require(initial.position.has_value() && !initial.fps && !initial.frame_interval_ms && !initial.physical_tps,
                "Initial owned snapshot fabricated rates");
        for (unsigned i = 0; i < 3; ++i)
            Require(std::abs((*initial.position)[i] - expected.position[i]) < .0001, "Initial player pose differs from checkpoint");
        Require(initial.backend == (options.renderer == "d3d12" ? Telemetry::DebugBackend::D3D12 : Telemetry::DebugBackend::Vulkan),
                "Snapshot backend differs from actual renderer");
        int count = 0;
        auto** windows = SDL_GetWindows(&count);
        Require(windows && count == 1, "Expected exactly one owned SDL window");
        auto* native = windows[0];
        const auto id = SDL_GetWindowID(native); SDL_free(windows);
        const auto push = [](SDL_Event event) { Require(SDL_PushEvent(&event), "Cannot queue owned-window event"); };
        const auto focus = [&](bool gain) {
            SDL_Event event{}; event.type = gain ? SDL_EVENT_WINDOW_FOCUS_GAINED : SDL_EVENT_WINDOW_FOCUS_LOST;
            event.window.windowID = id; push(event);
        };
        const auto key = [&](SDL_Scancode scancode) {
            SDL_Event event{}; event.type = SDL_EVENT_KEY_DOWN; event.key.windowID = id; event.key.scancode = scancode;
            event.key.down = true; push(event);
        };
        if (mode == "escape" || mode == "focus-escape") {
            focus(true); key(SDL_SCANCODE_ESCAPE);
            if (mode == "focus-escape") focus(false);
        } else if (mode == "input") {
            focus(true); key(SDL_SCANCODE_W); key(SDL_SCANCODE_SPACE); key(SDL_SCANCODE_E);
            SDL_Event event{}; event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = id; event.motion.xrel = 200; event.motion.yrel = 100; push(event);
            event = {}; event.type = SDL_EVENT_MOUSE_BUTTON_DOWN; event.button.windowID = id; event.button.button = SDL_BUTTON_LEFT; event.button.down = true; push(event);
        } else if (mode == "focus-loss") focus(false);
        else if (mode == "resize") Require(SDL_SetWindowSize(native, 1000, 700), "Cannot resize owned SDL window");
        else if (mode == "minimize") Require(SDL_MinimizeWindow(native), "Cannot minimize owned SDL window");
        else Require(mode == "initial", "Unknown event mode");

        const auto start = std::chrono::steady_clock::now();
        bool expected_failure = false;
        if (mode != "initial") {
            try { app.Run(); }
            catch (const std::runtime_error& error) {
                expected_failure = (mode == "resize" || mode == "minimize") &&
                    std::string(error.what()).find("Profiling scenario framebuffer changed or was minimized") != std::string::npos;
                if (!expected_failure) throw;
            }
        }
        Require(expected_failure == (mode == "resize" || mode == "minimize"), "Window failure classification changed");
        const auto final = app.DebugValues();
        if (mode == "input")
            Require(final.position && (*final.position)[0] == (*initial.position)[0] && (*final.position)[2] == (*initial.position)[2],
                    "Player input moved the static scenario");
        if (mode == "escape" || mode == "focus-escape")
            Require(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(800), "Queued Escape did not cancel promptly");
        Require(app.Shutdown().Succeeded() && app.Shutdown().Succeeded(), "Scenario shutdown did not preserve cleanup facts");
        Require(!app.DebugValues().fps && app.DebugValues().state == Telemetry::DebugState::Paused,
                "Shutdown retained active rates");
        Require(initial.position && !initial.fps && initial.state == Telemetry::DebugState::Unavailable,
                "Later updates changed the caller-owned initial snapshot");
        std::cout << "[profile-probe] passed; mode=" << mode << "; synthetic_window_events=true\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
