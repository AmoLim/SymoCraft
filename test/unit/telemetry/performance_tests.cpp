#include "symocraft/telemetry/performance.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
int main(int argc, char** argv)
{
    using namespace SymoCraft;
    try {
        Require(argc == 2, "Missing test output parent");
        std::vector<double> values;
        for (int i = 100; i > 0; --i) values.push_back(i);
        const auto stats = Performance::Statistics(values);
        Require(stats["p95"].as<double>() == 95 && stats["p99"].as<double>() == 99 && stats["mean"].as<double>() == 50.5, "Percentile definition changed");
        Require(Performance::Statistics({})["count"].as<int>() == 0, "Missing samples were fabricated");
        bool rejected = false;
        try { Performance::Statistics({std::numeric_limits<double>::infinity()}); }
        catch (const std::invalid_argument&) { rejected = true; }
        Require(rejected, "Invalid metric accepted");
        const auto parent = std::filesystem::absolute(argv[1]);
        std::filesystem::create_directories(parent);
        const auto path = parent / std::to_string(Performance::Clock::now().time_since_epoch().count());
        const auto path_text = path.string();
        Performance::SessionConfig options;
        options.benchmark = "edit"; options.output_directory = path_text;
        Performance::Session session(options, Performance::Clock::now());
        Performance::Frame warmup;
        warmup.frame_ms = 1000; warmup.focused = true;
        session.Add(warmup);
        Performance::Frame measured;
        measured.measured = true; measured.focused = true; measured.frame_ms = 10;
        session.Add(measured);
        measured.frame_ms = 200; measured.edits = 1; measured.rebuilt_chunks = 2;
        session.Add(measured);
        session.SetGpu(1, 2.5);
        session.completed = true;
        session.Export(true);
        const auto report = YAML::LoadFile((path / "summary.yaml").string());
        Require(report["valid_run"].as<bool>() && report["frame_ms"]["count"].as<int>() == 2, "Warmup leaked into summary");
        Require(report["frame_ms"]["p95"].as<double>() == 200 && report["sample_edits"].as<int>() == 1, "Edit slow frame was discarded");
        Require(report["gpu_missing_sample_frames"].as<int>() == 1, "Unavailable GPU result was fabricated");
        rejected = false;
        try { Performance::Session duplicate(options, Performance::Clock::now()); }
        catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Existing result directory was overwritten");
        session.Invalidate("test-interference");
        session.completed = false;
        session.Export(false);
        Require(!YAML::LoadFile((path / "summary.yaml").string())["valid_run"].as<bool>(), "Interrupted capture marked valid");
        std::ifstream csv(path / "frames.csv");
        std::string line; int lines = 0;
        while (std::getline(csv, line)) ++lines;
        Require(lines == 4, "Raw frames missing");
        for (const auto policy : {"strict", "allow-unfocused"}) {
            const auto focus_path = path.string() + policy;
            options.output_directory = focus_path; options.focus_policy = policy;
            Performance::Session focus_session(options, Performance::Clock::now());
            measured.focused = false; measured.frame_ms = 100;
            focus_session.Add(measured);
            measured.focused = true; focus_session.Add(measured);
            focus_session.completed = true;
            Require(focus_session.Export(true) == (options.focus_policy == "allow-unfocused"), "Wrong focus validity policy");
            const auto focus_report = YAML::LoadFile((std::filesystem::path(focus_path) / "summary.yaml").string());
            Require(focus_report["focus"]["transitions"].as<int>() == 1 && focus_report["focus"]["unfocused_frames"].as<int>() == 1, "Focus accounting failed");
            focus_session.Invalidate("framebuffer-changed-or-minimized");
            Require(!focus_session.Export(true), "Allow-unfocused accepted minimized capture");
        }
        std::cout << "Percentiles, warmup, slow frames, missing GPU values and partial exports passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
