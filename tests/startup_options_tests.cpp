#include "core/startup_options.h"
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
        Require(!Parse({}).seed.has_value());
        Require(Parse({"--seed", "0"}).seed == 0);
        Require(Parse({"--seed", "4294967295"}).seed == 4294967295u);
        const auto options = Parse({"--checkpoint", "four-chunk", "--seed", "42", "--scene", "regression", "--world-summary", "--test-edits"});
        Require(options.regression_scene && options.world_summary && options.test_edits && options.checkpoint == "four-chunk");
        Require(Parse({"--seed", "42", "--smoke-frames", "120"}).frame_limit == 120);
        Require(Parse({"--check-assets"}).check_assets);
        Reject({"--seed"}); Reject({"--seed", "-1"}); Reject({"--seed", "+1"});
        Reject({"--seed", "4294967296"}); Reject({"--seed", "1x"}); Reject({"--seed", ""});
        Reject({"--seed", "1", "--seed", "2"}); Reject({"--scene", "unknown"});
        Reject({"--checkpoint", "spawn"}); Reject({"--test-edits"});
        Reject({"--scene", "regression", "--checkpoint", "unknown"});
        Reject({"--world-summary", "--smoke-frames", "1"});
        Reject({"--smoke-frames", "0"}); Reject({"--smoke-frames", "10001"});
        Reject({"--check-assets", "--seed", "1"}); Reject({"--world-summary", "--world-summary"});
        std::cout << "Startup seed, scene, checkpoint and mode contracts passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
