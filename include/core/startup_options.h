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
    };

    // Throws invalid_argument; views refer to the caller's argument storage.
    StartupOptions ParseStartupOptions(std::span<const std::string_view> arguments);
    inline constexpr std::string_view StartupUsage =
        "Usage: SymoCraft [--check-assets | [--seed 0..4294967295] "
        "[--scene terrain|regression] [--checkpoint NAME] [--test-edits] "
        "[--world-summary | --smoke-frames 1..10000]]";
}
