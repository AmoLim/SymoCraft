#include "benchmark/runner.h"
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace {
    void Require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    unsigned long Number(const wchar_t* text) {
        const std::wstring value(text);
        std::size_t consumed = 0;
        const auto number = std::stoul(value, &consumed);
        Require(consumed == value.size() && !value.empty() && value.front() != L'-', "Invalid numeric argument");
        return number;
    }
    std::string CsvText(const std::string& value) {
        std::string out = "\"";
        for (const auto c : value) { if (c == '"') out += '"'; out += c; }
        return out + '"';
    }
    double Metric(const YAML::Node& node) {
        const auto value = node.as<double>();
        Require(std::isfinite(value) && value >= 0, "Missing or invalid capture metric");
        return value;
    }
}

int wmain(int argc, wchar_t** argv) {
    using namespace Benchmark;
    try {
        Require(argc == 6, "Usage: verifier <capture> <scene> <repeat> <exit-code> <metrics.csv>");
        const auto capture = fs::absolute(argv[1]);
        const auto scene = Utf8(argv[2]);
        const auto repeat = Number(argv[3]);
        Require((scene == "static" || scene == "walk" || scene == "edit") && repeat >= 1 && repeat <= 3,
                "Unknown Q06 presample round");
        const auto summary = ValidateCapture(capture, Round{scene, static_cast<unsigned>(repeat), 10, 30},
                                             "allow-unfocused", Number(argv[4]));
        const auto metadata = summary["metadata"];
        Require(metadata["gl_renderer"].as<std::string>().find("RTX 5070 Ti") != std::string::npos
                && metadata["gl_vendor"].as<std::string>().find("NVIDIA") != std::string::npos,
                "Capture did not use the required RTX 5070 Ti GL device");
        Require(metadata["build_configuration"].as<std::string>() == "Release", "Capture is not Release");
        Require(metadata["msaa_samples"].as<unsigned>() == 4, "Capture did not retain 4x MSAA");
        const auto frame = summary["frame_ms"];
        const auto count = frame["count"].as<std::size_t>();
        Require(count != 0, "Capture has no measured frames");
        const auto p50 = Metric(frame["p50"]), p95 = Metric(frame["p95"]), p99 = Metric(frame["p99"]);
        const auto mean = Metric(frame["mean"]), throughput = Metric(summary["throughput_fps"]);
        const auto ready = Metric(summary["startup_ms"]["ready_from_main_entry"]);
        const auto first_frame = Metric(summary["startup_ms"]["first_frame_swap_return_from_main_entry"]);
        Require(ready > 0 && first_frame >= ready, "Missing or inconsistent startup timing");
        const auto gpu = summary["gpu_draw_ms"];
        const auto gpu_count = gpu["count"].as<std::size_t>();
        const auto gpu_missing = summary["gpu_missing_sample_frames"].as<std::size_t>();
        Require(gpu_count <= count && gpu_missing == count - gpu_count, "GPU availability counts are inconsistent");
        const auto cpu = summary["cpu_submit_excluding_upload_ms"], present = summary["present_wait_cpu_ms"];
        const auto mesh = summary["mesh_ms"], upload = summary["upload_cpu_ms"];
        const auto cpu_p95 = Metric(cpu["p95"]), cpu_p99 = Metric(cpu["p99"]);
        const auto present_p95 = Metric(present["p95"]), present_p99 = Metric(present["p99"]);
        const auto mesh_p95 = Metric(mesh["p95"]), mesh_p99 = Metric(mesh["p99"]);
        const auto upload_p95 = Metric(upload["p95"]), upload_p99 = Metric(upload["p99"]);
        YAML::Emitter normalized;
        normalized << summary;
        Require(normalized.good(), "Cannot emit normalized summary YAML");
        const auto metrics_path = fs::absolute(argv[5]);
        Require(!fs::exists(metrics_path), "Metrics output already exists");
        std::ofstream metrics(metrics_path, std::ios::binary);
        metrics.exceptions(std::ios::failbit | std::ios::badbit);
        metrics.imbue(std::locale::classic());
        metrics << "valid,scene,repeat,warmup_seconds,sample_seconds,focus_policy,gl_renderer,gl_version,configuration,msaa_samples,frame_count,frame_p50_ms,frame_p95_ms,frame_p99_ms,frame_mean_ms,throughput_fps,ready_from_main_entry_ms,first_frame_swap_return_from_main_entry_ms,gpu_timer_supported,gpu_available,gpu_count,gpu_missing_sample_frames,gpu_p95_ms,gpu_p99_ms,cpu_submit_p95_ms,cpu_submit_p99_ms,present_p95_ms,present_p99_ms,mesh_p95_ms,mesh_p99_ms,upload_p95_ms,upload_p99_ms\n";
        metrics << "true," << CsvText(scene) << ',' << repeat << ",10,30,allow-unfocused,"
                << CsvText(metadata["gl_renderer"].as<std::string>()) << ','
                << CsvText(metadata["gl_version"].as<std::string>()) << ",Release,4," << count
                << std::setprecision(17) << ',' << p50 << ',' << p95 << ',' << p99 << ',' << mean << ',' << throughput
                << ',' << ready << ',' << first_frame << ',' << (metadata["gpu_timer_supported"].as<bool>() ? "true" : "false")
                << ',' << (gpu_count ? "true" : "false") << ',' << gpu_count << ',' << gpu_missing << ',';
        if (gpu_count) metrics << Metric(gpu["p95"]);
        metrics << ',';
        if (gpu_count) metrics << Metric(gpu["p99"]);
        metrics << ',' << cpu_p95 << ',' << cpu_p99 << ',' << present_p95 << ',' << present_p99
                << ',' << mesh_p95 << ',' << mesh_p99 << ',' << upload_p95 << ',' << upload_p99 << '\n';
        metrics.close();
        std::cout << normalized.c_str() << '\n';
        Require(static_cast<bool>(std::cout), "Cannot write normalized summary YAML");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Q06 presample validation failed: " << error.what() << '\n';
        return 1;
    }
}
