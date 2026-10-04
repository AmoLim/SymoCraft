#include "benchmark/runner.h"
#include <Windows.h>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>

int wmain(int argc, wchar_t** argv)
{
    using namespace Benchmark;
    wchar_t mode_text[128]{}; GetEnvironmentVariableW(L"SYMO_BENCH_FIXTURE", mode_text, 128);
    const std::wstring mode(mode_text);
    if (argc == 2 && std::wstring(argv[1]) == L"--hang") { Sleep(60000); return 0; }
    if (argc > 2 && std::wstring(argv[1]) == L"--echo") {
        for (int i = 2; i < argc; ++i) std::cout << Utf8(argv[i]) << '\n';
        return 0;
    }
    if (mode == L"instant-failure") return 3;
    if (mode == L"hang") { Sleep(60000); return 0; }
    std::map<std::wstring, std::wstring> args;
    for (int i = 1; i + 1 < argc; i += 2) args[argv[i]] = argv[i + 1];
    const fs::path output(args.at(L"--output")); fs::create_directory(output);
    const auto scene = Utf8(args.at(L"--benchmark"));
    const unsigned warmup = std::stoul(args.at(L"--warmup-seconds")), sample = std::stoul(args.at(L"--sample-seconds"));
    const bool focused = args.at(L"--focus-policy") == L"strict";
    YAML::Node status; status["protocol_version"] = 2; status["phase"] = "warmup"; status["elapsed_seconds"] = 0;
    WriteYaml(output / "status.yaml", status);
    std::ofstream frames(output / "frames.csv");
    frames << "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused\n";
    const auto duration = mode == L"short" ? warmup + 1 : warmup + sample;
    for (unsigned i = 0; i < duration; ++i)
        frames << i << ',' << (i >= warmup ? "sample" : "warmup") << ',' << i << ",1000,100,0,0,0,0,0,0,0,0,,0,0,0,0,0,0,0,0,0,0," << focused << '\n';
    frames.close();
    std::ofstream(output / "memory.csv") << "elapsed_s,working_set_bytes,private_bytes,device_dedicated_kib,device_available_kib\n0,100,200,,\n";
    std::ofstream(output / "focus.csv") << "frame,elapsed_s,focused\n0,0," << focused << '\n';
    YAML::Node summary;
    auto meta = summary["metadata"];
    meta["schema_version"] = 2; meta["protocol_version"] = mode == L"old-protocol" ? 1 : 2; meta["workload_version"] = 2;
    meta["benchmark"] = scene; meta["focus_policy"] = Utf8(args.at(L"--focus-policy"));
    meta["warmup_seconds"] = warmup; meta["sample_seconds"] = sample;
    meta["framebuffer_width"] = 1920; meta["framebuffer_height"] = 1080; meta["requested_vsync"] = false;
    meta["world"]["generation"]["seed"] = 424242; meta["world"]["chunks"] = 441;
    meta["world"]["terrain_digest"] = "fnv1a64:cd80ebb0446c15c6"; meta["world"]["scene_digest"] = "fnv1a64:bddd435ea737fa62";
    meta["gl_renderer"] = mode == L"intel" ? "Intel (fixture)" : "AMD Radeon (fixture)";
    meta["gl_version"] = "4.6 fixture"; meta["build_configuration"] = "Release";
    summary["completed"] = true;
    const bool invalid = mode == L"minimized" || (mode == L"fail-walk" && scene == "walk");
    summary["valid_run"] = !invalid; summary["invalid_reasons"] = YAML::Node(YAML::NodeType::Sequence);
    if (invalid) summary["invalid_reasons"].push_back("framebuffer-changed-or-minimized");
    summary["total_frames"] = duration; summary["frame_ms"]["count"] = duration - warmup;
    for (const auto key : {"p50", "p95", "p99", "mean"}) summary["frame_ms"][key] = 1000;
    if (mode == L"bad-percentile") summary["frame_ms"]["p95"] = 0;
    summary["sample_edits"] = 0; summary["throughput_fps"] = 1;
    summary["focus"]["unfocused_frames"] = focused ? 0 : duration;
    summary["focus"]["sample_unfocused_frames"] = focused ? 0 : duration - warmup;
    summary["focus"]["unfocused_seconds_estimate"] = focused ? 0 : duration;
    summary["focus"]["sample_unfocused_seconds_estimate"] = focused ? 0 : duration - warmup;
    if (mode != L"missing-summary") WriteYaml(output / "summary.yaml", summary);
    status["phase"] = "finished"; WriteYaml(output / "status.yaml", status);
    std::cout << "fixture package=" << Utf8(ExecutablePath().parent_path().wstring()) << '\n';
    return invalid ? 4 : 0;
}
