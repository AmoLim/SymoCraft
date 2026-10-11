#include "symocraft/telemetry/performance.h"

#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
    void Require(bool value, const char* message)
    {
        if (!value) throw std::runtime_error(message);
    }

    template<class Exception, class Action>
    void Reject(Action action, const char* message)
    {
        try { action(); }
        catch (const Exception&) { return; }
        throw std::runtime_error(message);
    }

    std::vector<std::string> Lines(const std::filesystem::path& path)
    {
        std::ifstream file(path);
        Require(file.good(), "Missing frame CSV");
        std::vector<std::string> lines;
        std::string line;
        while (std::getline(file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            lines.push_back(line);
        }
        return lines;
    }

    std::string NumericField(const std::string& line, unsigned index)
    {
        std::size_t begin = 0;
        for (unsigned i = 0; i < index; ++i) {
            const auto separator = line.find(',', begin);
            Require(separator != std::string::npos, "Truncated numeric CSV prefix");
            begin = separator + 1;
        }
        return line.substr(begin, line.find(',', begin) - begin);
    }

    SymoCraft::Performance::SessionConfig Options(const std::filesystem::path& parent, const char* suffix)
    {
        SymoCraft::Performance::SessionConfig options;
        options.output_directory = (parent / suffix).string();
        return options;
    }

    SymoCraft::Performance::Frame ModernFrame(std::uint64_t identity)
    {
        SymoCraft::Performance::Frame frame;
        frame.frame_id = identity;
        frame.frame_ms = 10;
        frame.measured = frame.focused = true;
        frame.cpu_prepare_ms = 1.5;
        frame.cpu_submit_ms = 2.25;
        frame.present_ms = .75;
        return frame;
    }
}

int main(int argc, char** argv)
{
    using namespace SymoCraft::Performance;
    try {
        Require(argc == 2, "Missing test output parent");
        const auto parent = std::filesystem::absolute(argv[1]) /
            std::to_string(Clock::now().time_since_epoch().count());
        std::filesystem::create_directories(parent);

        Session legacy(Options(parent, "legacy"), Clock::now());
        legacy.metadata["gl_renderer"] = "legacy-fixture";
        Frame old_frame;
        old_frame.frame_ms = 10;
        old_frame.measured = old_frame.focused = true;
        old_frame.pack_ms = 2;
        old_frame.render_cpu_ms = 4;
        old_frame.upload_cpu_ms = 1;
        legacy.Add(old_frame);
        legacy.Add(old_frame);
        legacy.SetGpu(0, 2);
        legacy.SetGpu(0, 3);
        Reject<std::invalid_argument>([&] { legacy.SetGpu(0, -1); }, "Negative legacy GPU sample accepted");
        Reject<std::invalid_argument>([&] { legacy.SetGpu(0, std::numeric_limits<double>::infinity()); }, "Infinite legacy GPU sample accepted");
        Reject<std::logic_error>([&] { legacy.SetGpu(2, 1); }, "Unmatched legacy row accepted");
        legacy.completed = true;
        Require(legacy.Export(true), "Legacy valid capture rejected");
        const auto old_report = YAML::LoadFile((legacy.Directory() / "summary.yaml").string());
        Require(old_report["metadata"]["schema_version"].as<int>() == 2, "Legacy schema version changed");
        Require(old_report["metadata"]["gl_renderer"].as<std::string>() == "legacy-fixture", "Legacy identity changed");
        Require(!old_report["metadata"]["renderer"], "Modern identity fabricated for legacy capture");
        Require(old_report["gpu_draw_ms"]["mean"].as<double>() == 3, "Legacy row-index replacement semantics changed");
        Require(old_report["gpu_missing_sample_frames"].as<int>() == 1, "Legacy missing GPU duration fabricated");
        Require(old_report["cpu_submit_excluding_upload_ms"]["mean"].as<double>() == 5, "Legacy submit metric changed");
        const auto legacy_lines = Lines(legacy.Directory() / "frames.csv");
        Require(legacy_lines.front() == "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused",
                "Legacy CSV shape changed");
        Require(NumericField(legacy_lines.at(2), 13).empty(), "Missing legacy GPU CSV cell is not empty");

        Session session(Options(parent, "modern"), Clock::now());
        RendererInfo renderer;
        renderer.backend = "d3d12";
        renderer.device_name = "actual-device";
        renderer.device_identity = "actual-luid";
        renderer.driver_version = "actual-driver";
        renderer.api_version = "FL12_0 SM6_0";
        renderer.framebuffer_format = "RGBA8_UNORM";
        renderer.framebuffer_width = 803;
        renderer.framebuffer_height = 557;
        renderer.actual_samples = 4;
        renderer.actual_present_mode = "immediate";
        renderer.validation_enabled = renderer.texture_arrays = renderer.selection_boxes = true;
        renderer.screenshots = renderer.gpu_timestamps = true;
        renderer.gpu_time_scope = "mesh/selection timestamps; excludes clear, upload, resolve, screenshot and Present";
        session.SetRendererInfo(renderer);
        session.AddStartupUpload({1.25, 100, 1});
        session.AddStartupUpload({.75, 200, 2});
        Reject<std::invalid_argument>([&] { session.AddStartupUpload({-1, 999, 9}); }, "Negative startup upload duration accepted");

        auto warmup = ModernFrame(2);
        warmup.measured = false;
        warmup.upload_bytes = warmup.resource_upload_bytes = 999;
        session.Add(warmup);
        auto first = ModernFrame(77);
        first.pack_ms = 50;
        first.render_cpu_ms = .25;
        first.upload_cpu_ms = 100;
        first.upload_bytes = 40;
        first.resource_upload_bytes = 28;
        first.render_upload_bytes = 12;
        first.upload_calls = 2;
        session.Add(first);
        session.Add(ModernFrame(4000));
        session.Add(ModernFrame(4001));
        session.Add(ModernFrame(900000000000ULL));
        session.SetGpuSample(4001, 0);
        session.SetGpuSample(2, 99);
        session.SetGpuSample(4000, std::nullopt, "discarded");
        session.SetGpuSample(77, 2.5);
        Reject<std::logic_error>([&] { session.SetGpuSample(77, 9); }, "Duplicate valid GPU result accepted");
        Reject<std::logic_error>([&] { session.SetGpuSample(4000, 9); }, "Missing result overwritten by duplicate GPU sample");
        Reject<std::logic_error>([&] { session.SetGpuSample(4002, 9); }, "Unmatched sparse renderer ID accepted");
        Reject<std::logic_error>([&] { session.Add(ModernFrame(77)); }, "Duplicate CPU renderer identity accepted");
        Reject<std::invalid_argument>([&] { session.SetGpuSample(900000000000ULL, -1); }, "Negative modern GPU sample accepted");
        Reject<std::invalid_argument>([&] { session.SetGpuSample(900000000000ULL, std::numeric_limits<double>::quiet_NaN()); }, "NaN modern GPU sample accepted");
        Reject<std::invalid_argument>([&] { session.SetGpuSample(900000000000ULL, std::numeric_limits<double>::infinity()); }, "Infinite modern GPU sample accepted");
        Reject<std::invalid_argument>([&] { session.SetGpuSample(900000000000ULL, std::nullopt); }, "Unexplained missing GPU sample accepted");
        Reject<std::invalid_argument>([&] { session.SetGpuSample(900000000000ULL, 1, "discarded"); }, "Duration with missing reason accepted");
        auto invalid_frame = ModernFrame(1000000000000ULL);
        invalid_frame.cpu_prepare_ms = -1;
        Reject<std::invalid_argument>([&] { session.Add(invalid_frame); }, "Negative renderer preparation accepted");
        invalid_frame.cpu_prepare_ms = 1;
        invalid_frame.cpu_submit_ms = std::numeric_limits<double>::quiet_NaN();
        Reject<std::invalid_argument>([&] { session.Add(invalid_frame); }, "Nonfinite renderer submission accepted");
        invalid_frame.cpu_submit_ms = 1;
        invalid_frame.gpu_draw_ms = std::numeric_limits<double>::infinity();
        Reject<std::invalid_argument>([&] { session.Add(invalid_frame); }, "Nonfinite prefilled GPU result accepted");
        Require(session.NextFrame() == 5, "Rejected sample mutated the row count");
        session.completed = true;
        Require(session.Export(true), "Modern valid capture rejected");
        const auto report = YAML::LoadFile((session.Directory() / "summary.yaml").string());
        const auto info = report["metadata"]["renderer"];
        Require(info["backend"].as<std::string>() == "d3d12" && info["device_identity"].as<std::string>() == "actual-luid",
                "Actual backend/device identity missing");
        Require(info["actual_samples"].as<int>() == 4 && info["framebuffer_height"].as<int>() == 557
                && info["actual_present_mode"].as<std::string>() == "immediate" && info["validation_enabled"].as<bool>(),
                "Actual framebuffer/presentation configuration missing");
        Require(!report["metadata"]["gl_renderer"], "Modern renderer labeled as GL");
        Require(report["metadata"]["gpu_time_scope"].as<std::string>() == renderer.gpu_time_scope, "GPU scope not preserved");
        Require(report["frame_ms"]["count"].as<int>() == 4 && report["gpu_draw_ms"]["count"].as<int>() == 2,
                "Warmup/missing GPU values leaked into measured statistics");
        Require(report["gpu_draw_ms"]["min"].as<double>() == 0 && report["gpu_draw_ms"]["max"].as<double>() == 2.5,
                "Valid zero timestamp or delayed result changed");
        Require(report["gpu_missing_sample_frames"].as<int>() == 2
                && report["gpu_missing_sample_reasons"]["discarded"].as<int>() == 1
                && report["gpu_missing_sample_reasons"]["not-returned"].as<int>() == 1,
                "Missing GPU reasons not retained");
        Require(report["cpu_prepare_ms"]["mean"].as<double>() == 1.5 && report["cpu_submit_ms"]["mean"].as<double>() == 2.25
                && report["present_wait_cpu_ms"]["mean"].as<double>() == .75,
                "Modern CPU timing scopes conflated");
        Require(report["cpu_submit_excluding_upload_ms"]["count"].as<int>() == 0, "Legacy derived submit fabricated for modern frame");
        Require(report["sample_upload_bytes"].as<int>() == 40 && report["sample_resource_upload_bytes"].as<int>() == 28
                && report["sample_render_upload_bytes"].as<int>() == 12 && report["sample_upload_calls"].as<int>() == 2,
                "Upload attribution/warmup exclusion failed");
        Require(report["startup_ms"]["resource_upload_cpu"].as<double>() == 2
                && report["metadata"]["startup_upload_bytes"].as<int>() == 300
                && report["metadata"]["startup_upload_calls"].as<int>() == 3,
                "Startup upload costs lost or invalid addition mutated counters");
        const auto rows = Lines(session.Directory() / "frames.csv");
        Require(rows.size() == 6 && rows.front().ends_with("renderer_frame_id,cpu_prepare_ms,cpu_submit_ms,gpu_missing_reason,resource_upload_bytes,render_upload_bytes,upload_calls"),
                "Renderer CSV columns missing");
        Require(NumericField(rows.at(2), 13) == "2.5" && rows.at(2).ends_with(",77,1.5,2.25,\"\",28,12,2"),
                "Delayed GPU sample mapped to row index instead of stable renderer ID");
        Require(NumericField(rows.at(3), 13).empty() && rows.at(3).ends_with(",4000,1.5,2.25,\"discarded\",0,0,0"),
                "Discarded GPU value fabricated in CSV");
        Require(NumericField(rows.at(4), 13) == "0", "Valid zero GPU duration omitted");
        Require(NumericField(rows.at(5), 13).empty() && rows.at(5).find("\"not-returned\"") != std::string::npos,
                "Unreturned GPU value fabricated in CSV");

        Session skipped(Options(parent, "skipped"), Clock::now());
        skipped.SetRendererInfo(renderer);
        skipped.Invalidate("renderer-frame-skipped");
        skipped.Add(ModernFrame(10));
        skipped.Add(ModernFrame(11));
        skipped.Add(ModernFrame(12));
        skipped.SetGpuSample(10, std::nullopt, "discarded");
        skipped.SetGpuSample(11, .5);
        Reject<std::logic_error>([&] { skipped.SetGpuSample(11, 1); }, "Skipped frame allowed duplicate GPU result");
        skipped.completed = true;
        Require(!skipped.Export(true), "Skipped renderer frame incorrectly validated the run");
        const auto skipped_report = YAML::LoadFile((skipped.Directory() / "summary.yaml").string());
        Require(skipped_report["gpu_draw_ms"]["count"].as<int>() == 1
                && skipped_report["gpu_missing_sample_reasons"]["discarded"].as<int>() == 1
                && skipped_report["gpu_missing_sample_reasons"]["not-returned"].as<int>() == 1,
                "Skipped presentation prematurely resolved its pending GPU result");

        Session overflow(Options(parent, "overflow"), Clock::now());
        overflow.SetRendererInfo(renderer);
        overflow.AddStartupUpload({0, std::numeric_limits<std::uint64_t>::max(), 1});
        Reject<std::overflow_error>([&] { overflow.AddStartupUpload({0, 1, 1}); }, "Startup bytes wrapped");
        auto missing = ModernFrame(0);
        overflow.Add(missing);
        overflow.SetGpuSample(0, std::nullopt, "device, \"failed\"");
        overflow.completed = true;
        Require(overflow.Export(true), "Explained unavailable GPU capture rejected");
        const auto quoted = Lines(overflow.Directory() / "frames.csv");
        Require(quoted.at(1).find("\"device, \"\"failed\"\"\"") != std::string::npos, "GPU missing reason not escaped as CSV");
        const auto overflow_report = YAML::LoadFile((overflow.Directory() / "summary.yaml").string());
        Require(overflow_report["metadata"]["startup_upload_bytes"].as<std::uint64_t>() == std::numeric_limits<std::uint64_t>::max(),
                "Rejected overflow mutated startup upload bytes");

        std::cout << "Legacy schema and stable sparse renderer telemetry passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
