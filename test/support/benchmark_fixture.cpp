#include "benchmark/runner.h"
#include <Windows.h>
#include <cstdint>
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
    const bool vulkan = args.contains(L"--renderer") && args.at(L"--renderer") == L"vulkan";
    const bool modern = vulkan || (args.contains(L"--renderer") && args.at(L"--renderer") == L"d3d12");
    YAML::Node status; status["protocol_version"] = 2; status["phase"] = "warmup"; status["elapsed_seconds"] = 0;
    if (mode != L"missing-status") WriteYaml(output / "status.yaml", status);
    std::ofstream frames(output / "frames.csv");
    frames << "frame,phase,elapsed_s,frame_ms,simulation_delta_ms,event_ms,simulation_ms,mesh_ms,pack_ms,render_cpu_ms,upload_cpu_ms,present_ms,instrumentation_ms,gpu_draw_ms,rebuilt_chunks,vertices,upload_bytes,draw_calls,edits,edit_lateness_ms,x,y,z,yaw,focused";
    if (modern) frames << ",renderer_frame_id,cpu_prepare_ms,cpu_submit_ms,gpu_missing_reason,resource_upload_bytes,render_upload_bytes,upload_calls";
    frames << '\n';
    const auto duration = mode == L"short" ? warmup + 1 : warmup + sample;
    unsigned missing = 0;
    for (unsigned i = 0; i < duration; ++i) {
        const bool absent_gpu = i % 2 != 0;
        if (modern && absent_gpu && i >= warmup) ++missing;
        frames << i << ',' << (i >= warmup ? "sample" : "warmup") << ',' << i << ",1000,100,0,0,0,0,"
               << (modern ? 3 : 0) << ",0,0,0,";
        if (modern && !absent_gpu) frames << (mode == L"modern-bad-gpu" ? -1.5 : 1.5);
        frames << ",0,0,0,0,0,0,0,0,0,0," << focused;
        if (modern) {
            const std::uint64_t id = 9007199254740993ull + (mode == L"modern-duplicate-id" && i == 1 ? 0 : i);
            frames << ',' << id << ",1,2,\"" << (absent_gpu && mode != L"modern-missing-reason" ? "not-returned" : "")
                   << "\"," << (mode == L"modern-bad-upload" ? 1 : 0) << ",0,0";
        }
        frames << '\n';
    }
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
    if (modern) {
        meta["renderer_telemetry_version"] = mode == L"modern-old-version" ? 99 : 1;
        meta["renderer_backend"] = vulkan ? "Vulkan" : "D3D12";
        auto renderer = meta["renderer"];
        renderer["backend"] = mode == L"modern-wrong-backend" ? (vulkan ? "D3D12" : "Vulkan") : (vulkan ? "Vulkan" : "D3D12");
        renderer["device_name"] = vulkan ? "Vulkan CPU fixture (not a GPU result)" : "D3D12 CPU fixture (not a GPU result)";
        renderer["device_identity"] = "fixture-only-identity"; renderer["driver_version"] = "fixture";
        renderer["api_version"] = "fixture FL12_0 SM6_0"; renderer["framebuffer_format"] = "fixture UNORM";
        renderer["framebuffer_width"] = 1920; renderer["framebuffer_height"] = 1080;
        renderer["actual_samples"] = 4; renderer["actual_present_mode"] = mode == L"modern-vsync" ? "Fifo" : "Immediate";
        renderer["validation_enabled"] = !args.contains(L"--validation") || args.at(L"--validation") == L"1";
        for (const auto key : {"gpu_time_scope", "cpu_prepare_scope", "cpu_submit_scope", "upload_bytes_scope", "memory_scope"})
            meta[key] = "CPU fixture scope only";
        if (mode == L"modern-gl-identity") meta["gl_renderer"] = "mislabeled native identity";
        {
            meta["shader_generation"] = ReadYaml(ExecutablePath().parent_path()/"assets/shaders"/(vulkan ? "vulkan" : "d3d12")/"build.json");
            if (mode == L"modern-bad-generation") meta["shader_generation"]["target"] = "wrong-target";
            if (mode == L"modern-shader-identity") meta["shader_generation"]["binary_sha256"][vulkan ? "blocks.vs.spv" : "blocks.vs.dxil"] = std::string(64,'b');
            if (mode == L"modern-missing-generation") meta.remove("shader_generation");
        }
        summary["sample_resource_upload_bytes"] = 0; summary["sample_render_upload_bytes"] = 0;
        summary["sample_upload_bytes"] = 0; summary["sample_upload_calls"] = 0;
        summary["gpu_draw_ms"]["count"] = duration - warmup - missing;
        summary["gpu_missing_sample_frames"] = missing;
        summary["gpu_missing_sample_reasons"]["not-returned"] = missing;
        for (const auto key : {"cpu_prepare_ms", "cpu_submit_ms"}) {
            summary[key]["count"] = duration - warmup;
            for (const auto stat : {"p50", "p95", "p99", "mean"})
                summary[key][stat] = std::string(key) == "cpu_prepare_ms" ? 1 : 2;
        }
    } else {
        meta["gl_renderer"] = mode == L"intel" ? "Intel (fixture)" : "AMD Radeon (fixture)";
        meta["gl_version"] = "4.6 fixture";
    }
    meta["build_configuration"] = "Release";
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
    status["phase"] = mode == L"exporting-status" ? "exporting" : "finished";
    if (mode != L"missing-status") WriteYaml(output / "status.yaml", status);
    std::cout << "fixture package=" << Utf8(ExecutablePath().parent_path().wstring()) << '\n';
    if (mode == L"finished-nonzero") return 3;
    return invalid ? 4 : 0;
}
