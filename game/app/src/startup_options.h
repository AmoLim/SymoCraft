#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace SymoCraft {
    struct StartupOptions {
        std::optional<std::uint32_t> seed;
        unsigned int frame_limit = 0;
        bool check_assets = false;
        bool world_summary = false;
        bool regression_scene = false;
        bool test_edits = false;
        std::string_view checkpoint = "spawn";
        std::string_view profile_scenario;
        unsigned scenario_seconds = 60;
        unsigned width = 1920, height = 1080;
        bool vsync = false;
        std::string_view renderer = "d3d12";
        std::uint32_t requested_samples = 4;
        bool enable_validation = true;
    };

    // Throws invalid_argument; views refer to the caller's argument storage.
    StartupOptions ParseStartupOptions(std::span<const std::string_view> arguments);
    inline constexpr std::string_view StartupUsage =
        "Usage: SymoCraft [--check-assets [--renderer opengl|d3d12|vulkan] | "
        "[--renderer opengl|d3d12|vulkan] [--msaa 1|2|4|8|16|32] [--validation 0|1] "
        "[--seed 0..4294967295] "
        "[--scene terrain|regression] [--checkpoint NAME] [--test-edits] "
        "[--world-summary | --smoke-frames 1..10000]] or "
        "SymoCraft --profile-scenario static|walk|edit [--scenario-seconds 1..1200] [--seed N] "
        "[--renderer opengl|d3d12|vulkan] [--msaa 1|2|4|8|16|32] [--validation 0|1] "
        "[--width 1920 --height 1080 --vsync 0|1]";
}
