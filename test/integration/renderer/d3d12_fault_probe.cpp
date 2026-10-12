#include "renderer_impl.h"
#include "backend/d3d12/d3d12_backend.h"
#include "backend/d3d12/diagnostic_access.h"
#include <symocraft/platform/window.h>
#include <symocraft/telemetry/document_io.h>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

const char* Category(RenderErrorCategory value) {
    switch (value) {
    case RenderErrorCategory::AllocationFailure: return "AllocationFailure";
    case RenderErrorCategory::DeviceLost: return "DeviceLost";
    case RenderErrorCategory::SubmissionFailure: return "SubmissionFailure";
    case RenderErrorCategory::Timeout: return "Timeout";
    case RenderErrorCategory::InvalidState: return "InvalidState";
    default: return "Other";
    }
}

template<class F> void Expect(RenderErrorCategory category, F&& action, Data::Value& report) {
    const auto started = Clock::now();
    try { action(); }
    catch (const RenderError& error) {
        report["category"] = Category(error.Category());
        report["backend"] = error.BackendName();
        report["operation"] = error.Operation();
        report["native_code"] = error.NativeCode();
        report["diagnostic"] = error.Diagnostic();
        report["elapsed_ms"] = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
        Require(error.Category() == category && error.BackendName() == "D3D12", "Unexpected fault category/backend");
        return;
    }
    throw std::runtime_error("Expected fault was not reported");
}

struct QueueGate final {
    ComPtr<ID3D12Fence> fence;
    explicit QueueGate(const D3D12DiagnosticAccess& access) {
        Require(SUCCEEDED(access.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence))), "Cannot create diagnostic gate");
        Require(SUCCEEDED(access.queue->Wait(fence.Get(), 1)), "Cannot queue diagnostic wait");
    }
    ~QueueGate() { if (fence) fence->Signal(1); }
    void Release() { Require(SUCCEEDED(fence->Signal(1)), "Cannot release owned queue gate"); }
};

void Await(const D3D12DiagnosticAccess& access) {
    const HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(event != nullptr, "Cannot create drain event");
    const HRESULT result = access.completion->SetEventOnCompletion(access.last_signal, event);
    const DWORD waited = SUCCEEDED(result) ? WaitForSingleObject(event, 3000) : WAIT_FAILED;
    CloseHandle(event);
    Require(SUCCEEDED(result) && waited == WAIT_OBJECT_0
            && access.completion->GetCompletedValue() >= access.last_signal, "Diagnostic work did not drain within bound");
}

void Run(const std::string& mode, Window& window, const std::filesystem::path& shaders, Data::Value& report) {
    RendererConfig config;
    config.vsync = false;
    config.enable_validation = true;
    config.shader_asset_root = shaders;
    config.wait_timeout = std::chrono::milliseconds(500);
    auto access = std::make_shared<D3D12DiagnosticAccess>();
    RendererState renderer(config, CreateD3D12DiagnosticBackend(window, config, access));
    const auto info = renderer.GetInfo();
    Require(info.device_name == "NVIDIA GeForce RTX 5070 Ti" && info.validation_enabled, "Actual hardware/validation differs from local acceptance scope");
    report["device_name"] = info.device_name;
    report["device_identity"] = info.device_identity;
    report["driver_version"] = info.driver_version;
    report["api_version"] = info.api_version;
    report["validation_enabled"] = info.validation_enabled;
    report["samples"] = info.actual_samples;
    report["wait_timeout_ms"] = config.wait_timeout.count();
    TextureArrayData texture;
    texture.width = texture.height = texture.layers = 1;
    texture.pixels = {230, 20, 40, 255};
    MeshData mesh;
    mesh.vertices = {{{-1, -1, -2}, {0, 0, 0}, 1.0f},
                     {{1, -1, -2}, {1, 0, 0}, 1.0f}, {{0, 1, -2}, {0.5f, 1, 0}, 1.0f}};
    if (mode == "in-flight-timeout") {
        // Real copy commands and their retirement signal wait behind this owned
        // GPU queue gate. No busy shader, global TDR or desktop setting changes.
        QueueGate gate(*access);
        Expect(RenderErrorCategory::Timeout, [&] { renderer.CreateTextureArray(texture); }, report["primary"]);
        const auto pending = access->last_signal;
        const auto completed = access->completion->GetCompletedValue();
        report["queued_fence"] = pending;
        report["completed_fence_at_timeout"] = completed;
        Require(pending > completed && completed != UINT64_MAX, "Timeout did not have actual pending native work");
        report["actual_in_flight_timeout"] = true;
        Expect(RenderErrorCategory::InvalidState, [&] { renderer.CreateMesh(mesh); }, report["continuation_rejected"]);
        Expect(RenderErrorCategory::Timeout, [&] { renderer.Shutdown(); }, report["cleanup"]);
        Require(access->quarantined, "In-flight timeout falsely reported normal reclamation");
        report["terminal_quarantine"] = true;
        gate.Release();
        Await(*access);
        report["owned_gate_released_and_work_completed"] = true;
        Expect(RenderErrorCategory::SubmissionFailure, [&] { renderer.Shutdown(); }, report["repeat_cleanup"]);
        report["normal_shutdown"] = false;
        return;
    }
    const auto texture_handle = renderer.CreateTextureArray(texture);
    const auto mesh_handle = renderer.CreateMesh(mesh);
    const std::array draws{MeshDraw{mesh_handle, texture_handle}};
    FrameInput frame;
    frame.draws = draws;
    frame.request_screenshot = true;
    const auto before = renderer.Render(frame);
    Require(before.screenshot && before.status == FrameStatus::Presented, "Native baseline was not presented/read back");
    if (mode == "allocation") {
        access->next_mesh_allocation_result = E_OUTOFMEMORY;
        auto replacement = mesh;
        for (auto& vertex : replacement.vertices) vertex.pos_coord.x += 20;
        Expect(RenderErrorCategory::AllocationFailure, [&] { renderer.UpdateMesh(mesh_handle, replacement); }, report["primary"]);
        Require(report["primary"]["native_code"].as<std::string>() == "0x8007000E", "Original allocation HRESULT was lost");
        frame.frame_id = 1;
        const auto retained = renderer.Render(frame);
        Require(retained.screenshot && retained.screenshot->pixels == before.screenshot->pixels, "Recoverable update changed accepted native content");
        renderer.UpdateMesh(mesh_handle, {});
        frame.frame_id = 2;
        const auto cleared = renderer.Render(frame);
        Require(cleared.screenshot && cleared.screenshot->pixels != before.screenshot->pixels, "Baseline triangle was blank or successful empty update retained geometry");
        report["injected_allocation_failure"] = true;
        report["old_content_retained_and_empty_retry_cleared"] = true;
    } else if (mode == "submission") {
        access->next_frame_signal_result = E_FAIL;
        frame.frame_id = 1;
        Expect(RenderErrorCategory::SubmissionFailure, [&] { renderer.Render(frame); }, report["primary"]);
        Require(report["primary"]["native_code"].as<std::string>() == "0x80004005", "Original submission HRESULT was lost");
        report["injected_signal_failure_after_actual_execute"] = true;
        Expect(RenderErrorCategory::InvalidState, [&] { renderer.Render(frame); }, report["continuation_rejected"]);
    } else {
        ComPtr<ID3D12Device5> device;
        Require(SUCCEEDED(access->device.As(&device)), "RemoveDevice interface is unavailable");
        device->RemoveDevice();
        Require(FAILED(access->device->GetDeviceRemovedReason()), "Runtime device was not removed");
        report["controlled_native_device_removal"] = true;
        report["spontaneous_hardware_device_loss"] = false;
        Expect(RenderErrorCategory::DeviceLost, [&] { renderer.PollGpuSamples(); }, report["primary"]);
        Expect(RenderErrorCategory::InvalidState, [&] { renderer.CreateMesh(mesh); }, report["continuation_rejected"]);
        try { renderer.Shutdown(); report["cleanup_reported_error"] = false; }
        catch (const RenderError& error) {
            report["cleanup_reported_error"] = true;
            report["cleanup_category"] = Category(error.Category());
            Require(error.Category() == RenderErrorCategory::DeviceLost, "Cleanup replaced device-loss category");
        }
        report["normal_shutdown"] = false;
        return;
    }
    renderer.Shutdown();
    renderer.Shutdown();
    report["normal_shutdown"] = true;
}
}

int main(int argc, char** argv) {
    if (argc != 4) return 64;
    const std::string mode = argv[1];
    if (mode != "allocation" && mode != "submission" && mode != "device-removal" && mode != "in-flight-timeout") return 64;
    const std::filesystem::path output = argv[3];
    if (std::filesystem::exists(output)) return 64;
    Data::Value report;
    report["schema"] = "symocraft.d3d12-controlled-faults.v1";
    report["mode"] = mode;
    report["real_facade_state_and_backend"] = true;
    report["public_wrapper_or_gameplay_acceptance"] = false;
    report["success"] = false;
    bool initialized = false;
    int exit_code = 1;
    const auto started = Clock::now();
    try {
        Window::Init(); initialized = true;
        std::unique_ptr<Window> window(Window::Create("Symocraft controlled native fault", 640, 360, false, true, WindowMode::Native));
        Run(mode, *window, std::filesystem::path(argv[2]), report);
        window->Destroy(); window.reset();
        Window::Free(); initialized = false;
        report["success"] = true;
        exit_code = 0;
    } catch (const std::exception& error) {
        report["harness_error"] = error.what();
        std::cerr << error.what() << '\n';
    }
    if (initialized) { try { Window::Free(); } catch (...) {} }
    report["elapsed_ms"] = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
    std::ofstream stream(output, std::ios::binary);
    stream << Data::DumpYaml(report);
    if (!stream) return 1;
    std::cout << mode << " success=" << (exit_code == 0) << '\n';
    return exit_code;
}
