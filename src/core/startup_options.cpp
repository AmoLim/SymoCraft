#include "core/startup_options.h"
#include <array>
#include <charconv>
#include <stdexcept>

namespace SymoCraft {
    StartupOptions ParseStartupOptions(std::span<const std::string_view> arguments)
    {
        StartupOptions result;
        constexpr std::array names{"--seed", "--scene", "--checkpoint", "--test-edits",
                                   "--world-summary", "--smoke-frames", "--check-assets", "--benchmark", "--output",
                                   "--width", "--height", "--vsync", "--warmup-seconds", "--sample-seconds", "--focus-policy"};
        std::array<bool, names.size()> seen{};
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            std::size_t option = 0;
            while (option < names.size() && arguments[i] != names[option]) ++option;
            if (option == names.size() || seen[option])
                throw std::invalid_argument("Unknown or repeated option");
            seen[option] = true;
            if (option == 3) { result.test_edits = true; continue; }
            if (option == 4) { result.world_summary = true; continue; }
            if (option == 6) { result.check_assets = true; continue; }
            if (++i == arguments.size()) throw std::invalid_argument("Missing option value");
            const auto value = arguments[i];
            if (option == 0 || option == 5 || (option >= 9 && option <= 13)) {
                std::uint32_t number = 0;
                const auto parsed = std::from_chars(value.data(), value.data() + value.size(), number);
                if (value.empty() || parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
                    throw std::invalid_argument("Expected an unsigned decimal integer");
                if (option == 0) result.seed = number;
                else if (option == 5) {
                    if (number == 0 || number > 10000) throw std::invalid_argument("Invalid frame limit");
                    result.frame_limit = number;
                } else if (option == 9 || option == 10) {
                    if (number < 320 || number > 8192) throw std::invalid_argument("Framebuffer dimensions must be 320..8192");
                    (option == 9 ? result.width : result.height) = number;
                } else if (option == 11) {
                    if (number > 1) throw std::invalid_argument("VSync must be 0 or 1");
                    result.vsync = number != 0;
                } else {
                    if (number > 600 || (option == 13 && number == 0)) throw std::invalid_argument("Invalid benchmark duration");
                    (option == 12 ? result.warmup_seconds : result.sample_seconds) = number;
                }
            } else if (option == 14) {
                if (value != "strict" && value != "allow-unfocused") throw std::invalid_argument("Unknown focus policy");
                result.focus_policy = value;
            } else if (option == 1) {
                if (value != "terrain" && value != "regression") throw std::invalid_argument("Unknown scene");
                result.regression_scene = value == "regression";
            } else if (option == 7) {
                if (value != "static" && value != "walk" && value != "edit") throw std::invalid_argument("Unknown benchmark");
                result.benchmark = value;
            } else if (option == 8) {
                if (value.empty()) throw std::invalid_argument("Empty output directory");
                result.output_directory = value;
            } else {
                constexpr std::array checkpoints{"spawn", "positive-x", "negative-x", "four-chunk",
                                                  "wall-corner", "low-ceiling", "single-block"};
                bool found = false;
                for (const auto name : checkpoints) found |= value == name;
                if (!found) throw std::invalid_argument("Unknown checkpoint");
                result.checkpoint = value;
            }
        }
        if (result.check_assets && arguments.size() != 1)
            throw std::invalid_argument("Asset preflight cannot be combined with world options");
        if (result.world_summary && result.frame_limit != 0)
            throw std::invalid_argument("World summary does not render frames");
        if (!result.benchmark.empty()) {
            if (seen[1] || seen[2] || seen[3] || seen[4] || seen[5] || seen[6] || !seen[8])
                throw std::invalid_argument("Benchmark owns scene, checkpoint and duration; --output is required");
            result.regression_scene = true;
            result.checkpoint = result.benchmark == "walk" ? "spawn" : "four-chunk";
        } else {
            for (std::size_t i = 8; i < seen.size(); ++i)
                if (seen[i]) throw std::invalid_argument("Sampling options require --benchmark");
        }
        if (!result.regression_scene && (seen[2] || result.test_edits))
            throw std::invalid_argument("Checkpoints and test edits require the regression scene");
        return result;
    }
}
