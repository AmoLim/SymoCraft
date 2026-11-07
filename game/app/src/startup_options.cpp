#include "startup_options.h"
#include <array>
#include <charconv>
#include <stdexcept>
#include <string>

namespace SymoCraft {
    StartupOptions ParseStartupOptions(std::span<const std::string_view> arguments) {
        StartupOptions result;
        enum Option : std::size_t { Seed, Scene, Checkpoint, TestEdits, Summary, Frames, Assets,
            Width, Height, Vsync, Renderer, Msaa, Validation, Profile, Seconds, Count };
        constexpr std::array names{"--seed", "--scene", "--checkpoint", "--test-edits", "--world-summary",
            "--smoke-frames", "--check-assets", "--width", "--height", "--vsync", "--renderer", "--msaa",
            "--validation", "--profile-scenario", "--scenario-seconds"};
        constexpr std::array retired{"--benchmark", "--output", "--warmup-seconds", "--sample-seconds",
            "--focus-policy", "--foreground-ready-timeout-seconds"};
        std::array<bool, Count> seen{};
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            const auto key = arguments[i];
            for (const auto old : retired)
                if (key == old) throw std::invalid_argument(std::string(key) +
                    " is retired; use --profile-scenario static|walk|edit [--scenario-seconds 1..1200]");
            std::size_t option = 0;
            while (option < names.size() && key != names[option]) ++option;
            if (option == names.size() || seen[option])
                throw std::invalid_argument("Unknown or repeated option: " + std::string(key));
            seen[option] = true;
            if (option == TestEdits) { result.test_edits = true; continue; }
            if (option == Summary) { result.world_summary = true; continue; }
            if (option == Assets) { result.check_assets = true; continue; }
            if (++i == arguments.size()) throw std::invalid_argument("Missing value for " + std::string(key));
            const auto value = arguments[i];
            if (option == Seed || option == Frames || option == Width || option == Height ||
                option == Vsync || option == Msaa || option == Validation || option == Seconds) {
                std::uint32_t number = 0;
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
                if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
                    throw std::invalid_argument(std::string(key) + " expects an unsigned decimal integer");
                if (option == Seed) result.seed = number;
                else if (option == Frames) {
                    if (number == 0 || number > 10000) throw std::invalid_argument("--smoke-frames must be 1..10000");
                    result.frame_limit = number;
                } else if (option == Width || option == Height) {
                    if (number < 320 || number > 8192) throw std::invalid_argument(std::string(key) + " must be 320..8192");
                    (option == Width ? result.width : result.height) = number;
                } else if (option == Vsync) {
                    if (number > 1) throw std::invalid_argument("--vsync must be 0 or 1");
                    result.vsync = number != 0;
                } else if (option == Msaa) {
                    if (number == 0 || number > 32 || (number & (number - 1)) != 0)
                        throw std::invalid_argument("--msaa must be a power of two in 1..32");
                    result.requested_samples = number;
                } else if (option == Validation) {
                    if (number > 1) throw std::invalid_argument("--validation must be 0 or 1");
                    result.enable_validation = number != 0;
                } else {
                    if (number == 0 || number > 1200) throw std::invalid_argument("--scenario-seconds must be 1..1200");
                    result.scenario_seconds = number;
                }
            } else if (option == Renderer) {
                if (value != "opengl" && value != "d3d12" && value != "vulkan") throw std::invalid_argument("Unknown --renderer");
                result.renderer = value;
            } else if (option == Scene) {
                if (value != "terrain" && value != "regression") throw std::invalid_argument("Unknown --scene");
                result.regression_scene = value == "regression";
            } else if (option == Profile) {
                if (value != "static" && value != "walk" && value != "edit") throw std::invalid_argument("Unknown --profile-scenario");
                result.profile_scenario = value;
            } else {
                constexpr std::array checkpoints{"spawn", "positive-x", "negative-x", "four-chunk", "wall-corner", "low-ceiling", "single-block"};
                bool found = false;
                for (const auto name : checkpoints) found |= value == name;
                if (!found) throw std::invalid_argument("Unknown --checkpoint");
                result.checkpoint = value;
            }
        }
        if (result.check_assets) {
            for (std::size_t i = 0; i < seen.size(); ++i)
                if (seen[i] && i != Assets && i != Renderer)
                    throw std::invalid_argument("--check-assets only accepts --renderer");
        }
        if (result.world_summary && result.frame_limit) throw std::invalid_argument("--world-summary conflicts with --smoke-frames");
        if (result.world_summary && (seen[Renderer] || seen[Msaa] || seen[Validation]))
            throw std::invalid_argument("--world-summary does not configure a renderer");
        if (result.renderer == "opengl" && (seen[Msaa] || seen[Validation]))
            throw std::invalid_argument("--msaa and --validation overrides require a modern renderer");
        if (!result.profile_scenario.empty()) {
            if (seen[Scene] || seen[Checkpoint] || seen[TestEdits] || seen[Summary] || seen[Frames] || seen[Assets])
                throw std::invalid_argument("--profile-scenario owns scene, checkpoint and duration; conflicting mode");
            result.regression_scene = true;
            result.checkpoint = result.profile_scenario == "walk" ? "spawn" : "four-chunk";
        } else if (seen[Width] || seen[Height] || seen[Vsync] || seen[Seconds]) {
            throw std::invalid_argument("--width, --height, --vsync and --scenario-seconds require --profile-scenario");
        }
        if (!result.regression_scene && (seen[Checkpoint] || result.test_edits))
            throw std::invalid_argument("--checkpoint and --test-edits require the regression scene");
        return result;
    }
}
