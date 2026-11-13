#include "startup_options.h"
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

namespace {
    void Require(bool value) { if (!value) throw std::runtime_error("Startup option contract failed"); }
    SymoCraft::StartupOptions Parse(std::initializer_list<std::string_view> args) {
        const std::vector<std::string_view> values(args);
        return SymoCraft::ParseStartupOptions(values);
    }
    void Reject(std::initializer_list<std::string_view> args) {
        try { (void)Parse(args); } catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid startup options accepted");
    }
}
int main() {
    try {
        const auto defaults = Parse({});
        Require(!defaults.seed && defaults.profile_scenario.empty());
        Require(defaults.renderer == "d3d12" && defaults.requested_samples == 4 && defaults.enable_validation);
        Require(Parse({"--renderer", "opengl"}).renderer == "opengl");
        for (const auto name : {"static", "walk", "edit"}) {
            const auto scenario = Parse({"--profile-scenario", name});
            Require(scenario.profile_scenario == name && scenario.scenario_seconds == 60);
            Require(scenario.regression_scene && scenario.checkpoint == (std::string_view(name) == "walk" ? "spawn" : "four-chunk"));
            Require(scenario.width == 1920 && scenario.height == 1080 && !scenario.vsync);
            Require(Parse({"--profile-scenario",name,"--scenario-seconds","1"}).scenario_seconds == 1);
            Require(Parse({"--profile-scenario",name,"--scenario-seconds","1200"}).scenario_seconds == 1200);
            for (const auto bad : {"", "0", "1201", "-1", "+1", "1.5", "1x", "4294967296"})
                Reject({"--profile-scenario",name,"--scenario-seconds",bad});
            Reject({"--profile-scenario",name,"--scene","regression"});
            Reject({"--profile-scenario",name,"--scene","terrain"});
            Reject({"--profile-scenario",name,"--checkpoint","spawn"});
            Reject({"--profile-scenario",name,"--test-edits"});
            Reject({"--profile-scenario",name,"--world-summary"});
            Reject({"--profile-scenario",name,"--check-assets"});
            Reject({"--profile-scenario",name,"--smoke-frames","1"});
            Reject({"--profile-scenario",name,"--profile-scenario",name});
        }
        for (const auto renderer : {"d3d12", "vulkan"}) {
            Require(Parse({"--renderer",renderer}).renderer == renderer);
            unsigned expected = 1;
            for (const auto samples : {"1", "2", "4", "8", "16", "32"}) {
                const auto options = Parse({"--renderer",renderer,"--msaa",samples,"--validation","0"});
                Require(options.requested_samples == expected && !options.enable_validation); expected *= 2;
            }
            const auto configured = Parse({"--profile-scenario","static","--renderer",renderer,"--msaa","8",
                "--validation","1","--width","640","--height","480","--vsync","1","--seed","0"});
            Require(configured.width == 640 && configured.height == 480 && configured.vsync && configured.seed == 0u);
            Require(configured.requested_samples == 8 && configured.enable_validation);
            Require(Parse({"--check-assets","--renderer",renderer}).check_assets);
            Require(Parse({"--renderer",renderer,"--smoke-frames","120"}).frame_limit == 120);
            Reject({"--renderer",renderer,"--world-summary"});
            Reject({"--check-assets","--renderer",renderer,"--validation","1"});
            Reject({"--renderer",renderer,"--width","640"});
        }
        for (const auto old : {"--benchmark", "--output", "--warmup-seconds", "--sample-seconds", "--focus-policy", "--foreground-ready-timeout-seconds"}) {
            try { (void)Parse({old,"anything"}); throw std::runtime_error("Retired option accepted"); }
            catch (const std::invalid_argument& error) {
                Require(std::string(error.what()).find(old) != std::string::npos);
                Require(std::string(error.what()).find("--profile-scenario") != std::string::npos);
            }
        }
        for (const auto bad : {"", "bad", "STATIC"}) Reject({"--profile-scenario",bad});
        Reject({"--profile-scenario"}); Reject({"--scenario-seconds","1"});
        Reject({"--width","640"}); Reject({"--height","480"}); Reject({"--vsync","1"});
        Reject({"--profile-scenario","static","--width","0"});
        Reject({"--profile-scenario","static","--height","8193"});
        Reject({"--profile-scenario","static","--vsync","2"});
        Require(Parse({"--seed","4294967295"}).seed == 4294967295u);
        const auto summary = Parse({"--checkpoint","four-chunk","--seed","42","--scene","regression","--world-summary","--test-edits"});
        Require(summary.regression_scene && summary.world_summary && summary.test_edits);
        Require(Parse({"--check-assets"}).check_assets);
        Reject({"--renderer"}); Reject({"--renderer","metal"});
        Reject({"--renderer","d3d12","--renderer","vulkan"});
        Reject({"--renderer","opengl","--msaa","4"}); Reject({"--renderer","opengl","--validation","0"});
        for (const auto bad : {"", "0", "3", "33", "-1", "+4", "4x", "4.0", "4294967296"}) Reject({"--msaa",bad});
        for (const auto bad : {"", "2", "-1", "true", "1x"}) Reject({"--validation",bad});
        Reject({"--seed"}); Reject({"--seed","-1"}); Reject({"--seed","+1"});
        Reject({"--seed","4294967296"}); Reject({"--seed","1x"}); Reject({"--seed",""});
        Reject({"--seed","1","--seed","2"}); Reject({"--scene","unknown"});
        Reject({"--checkpoint","spawn"}); Reject({"--test-edits"});
        Reject({"--scene","regression","--checkpoint","unknown"});
        Reject({"--world-summary","--smoke-frames","1"});
        Reject({"--smoke-frames","0"}); Reject({"--smoke-frames","10001"});
        Reject({"--check-assets","--seed","1"}); Reject({"--world-summary","--world-summary"});
        Reject({"--unknown"});
        std::cout << "Independent scenarios, retired options and preserved ordinary startup contracts passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
