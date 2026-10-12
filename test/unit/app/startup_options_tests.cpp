#include "startup_options.h"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
    void Require(bool value) { if (!value) throw std::runtime_error("Startup option contract failed"); }
    SymoCraft::StartupOptions Parse(std::initializer_list<std::string_view> args)
    {
        const std::vector<std::string_view> values(args);
        return SymoCraft::ParseStartupOptions(values);
    }
    void Reject(std::initializer_list<std::string_view> args)
    {
        try { Parse(args); } catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid startup options accepted");
    }
}
int main()
{
    try {
        const auto defaults = Parse({});
        Require(!defaults.seed.has_value());
        Require(defaults.renderer == "d3d12" && defaults.requested_samples == 4 && defaults.enable_validation);
        Require(Parse({"--renderer", "opengl"}).renderer == "opengl");
        for (const auto renderer : {"d3d12", "vulkan"}) {
            const auto modern = Parse({"--renderer", renderer});
            Require(modern.renderer == renderer && modern.requested_samples == 4 && modern.enable_validation);
            std::uint32_t expected_samples = 1;
            for (const auto samples : {"1", "2", "4", "8", "16", "32"}) {
                const auto configured = Parse({"--msaa", samples, "--validation", "0", "--renderer", renderer});
                Require(configured.renderer == renderer && !configured.enable_validation);
                Require(configured.requested_samples == expected_samples);
                expected_samples *= 2;
            }
            Require(Parse({"--renderer", renderer, "--validation", "1"}).enable_validation);
            const auto smoke = Parse({"--renderer", renderer, "--msaa", "1", "--validation", "0", "--smoke-frames", "120"});
            Require(smoke.renderer == renderer && smoke.frame_limit == 120 && smoke.requested_samples == 1 && !smoke.enable_validation);
            const auto modern_benchmark = Parse({"--benchmark", "static", "--output", "new-run", "--renderer", renderer,
                                                 "--msaa", "8", "--validation", "1", "--width", "640", "--height", "480", "--vsync", "1"});
            Require(modern_benchmark.renderer == renderer && modern_benchmark.requested_samples == 8 && modern_benchmark.enable_validation);
            Require(modern_benchmark.width == 640 && modern_benchmark.height == 480 && modern_benchmark.vsync);
            Require(Parse({"--check-assets", "--renderer", renderer}).check_assets);
            Require(Parse({"--renderer", renderer, "--check-assets"}).renderer == renderer);
            Reject({"--renderer", renderer, "--world-summary"});
            Reject({"--renderer", renderer, "--msaa", "4", "--world-summary"});
            Reject({"--renderer", renderer, "--validation", "1", "--world-summary"});
            Reject({"--check-assets", "--renderer", renderer, "--msaa", "4"});
            Reject({"--check-assets", "--renderer", renderer, "--validation", "1"});
            Reject({"--renderer", renderer, "--width", "640", "--smoke-frames", "1"});
            Reject({"--renderer", renderer, "--height", "480"});
            Reject({"--renderer", renderer, "--vsync", "1"});
        }
        Require(Parse({"--check-assets", "--renderer", "opengl"}).renderer == "opengl");
        Reject({"--renderer"}); Reject({"--renderer", ""});
        Reject({"--renderer", "D3D12"}); Reject({"--renderer", "metal"});
        Reject({"--renderer", "d3d12", "--renderer", "vulkan"});
        Require(Parse({"--msaa", "4"}).renderer == "d3d12");
        Require(Parse({"--validation", "1"}).renderer == "d3d12");
        Reject({"--renderer", "opengl", "--msaa", "4"});
        Reject({"--renderer", "opengl", "--validation", "1"});
        Require(Parse({"--benchmark", "static", "--output", "new-run", "--msaa", "4"}).renderer == "d3d12");
        Reject({"--benchmark", "static", "--output", "new-run", "--renderer", "opengl", "--validation", "0"});
        Reject({"--renderer", "opengl", "--world-summary"});
        Reject({"--check-assets", "--msaa", "4"});
        Reject({"--check-assets", "--validation", "0"});
        Reject({"--check-assets", "--renderer", "d3d12", "--scene", "terrain"});
        Reject({"--check-assets", "--renderer", "d3d12", "--benchmark", "static", "--output", "new-run"});
        Reject({"--renderer", "d3d12", "--msaa"});
        Reject({"--renderer", "d3d12", "--validation"});
        Reject({"--renderer", "d3d12", "--msaa", "4", "--msaa", "4"});
        Reject({"--renderer", "d3d12", "--validation", "1", "--validation", "0"});
        for (const auto samples : {"", "0", "3", "6", "33", "64", "-1", "+4", "4x", "4.0", "4294967296"})
            Reject({"--renderer", "d3d12", "--msaa", samples});
        for (const auto validation : {"", "2", "-1", "+1", "true", "false", "1x", "4294967296"})
            Reject({"--renderer", "d3d12", "--validation", validation});
        Reject({"--unknown-option"});
        Require(Parse({"--seed", "0"}).seed == 0u);
        Require(Parse({"--seed", "4294967295"}).seed == 4294967295u);
        const auto options = Parse({"--checkpoint", "four-chunk", "--seed", "42", "--scene", "regression", "--world-summary", "--test-edits"});
        Require(options.regression_scene && options.world_summary && options.test_edits && options.checkpoint == "four-chunk");
        Require(Parse({"--seed", "42", "--smoke-frames", "120"}).frame_limit == 120);
        Require(Parse({"--check-assets"}).check_assets && Parse({"--check-assets"}).renderer == "d3d12");
        const auto benchmark = Parse({"--benchmark", "walk", "--output", "new-run", "--warmup-seconds", "0", "--sample-seconds", "1", "--vsync", "1"});
        Require(benchmark.regression_scene && benchmark.checkpoint == "spawn" && benchmark.vsync && benchmark.sample_seconds == 1 && benchmark.warmup_seconds == 0);
        Require(Parse({"--benchmark", "static", "--output", "new-run"}).checkpoint == "four-chunk");
        Reject({"--benchmark", "edit"});
        Require(Parse({"--benchmark", "static", "--output", "new-run"}).focus_policy == "strict");
        Require(Parse({"--benchmark", "static", "--output", "new-run", "--focus-policy", "allow-unfocused"}).focus_policy == "allow-unfocused");
        Reject({"--focus-policy", "allow-unfocused"});
        Reject({"--benchmark", "static", "--output", "new-run", "--focus-policy", "anything"});
        Reject({"--benchmark", "bad", "--output", "new-run"});
        Reject({"--output", "new-run"}); Reject({"--width", "1920"});
        Reject({"--benchmark", "walk", "--output", "new-run", "--scene", "regression"});
        Reject({"--benchmark", "edit", "--output", "new-run", "--test-edits"});
        Reject({"--benchmark", "static", "--output", "new-run", "--sample-seconds", "0"});
        Reject({"--benchmark", "static", "--output", "new-run", "--width", "0"});
        Reject({"--benchmark", "static", "--output", "new-run", "--vsync", "2"});
        Reject({"--seed"}); Reject({"--seed", "-1"}); Reject({"--seed", "+1"});
        Reject({"--seed", "4294967296"}); Reject({"--seed", "1x"}); Reject({"--seed", ""});
        Reject({"--seed", "1", "--seed", "2"}); Reject({"--scene", "unknown"});
        Reject({"--checkpoint", "spawn"}); Reject({"--test-edits"});
        Reject({"--scene", "regression", "--checkpoint", "unknown"});
        Reject({"--world-summary", "--smoke-frames", "1"});
        Reject({"--smoke-frames", "0"}); Reject({"--smoke-frames", "10001"});
        Reject({"--check-assets", "--seed", "1"}); Reject({"--world-summary", "--world-summary"});
        std::cout << "Startup seed, scene, checkpoint, renderer and mode contracts passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
