#include "application.h"
#include "game_entry.h"
#include "diagnostic_access.h"
#include <symocraft/platform/window.h>
#include <symocraft/renderer/error.h>
#include <symocraft/telemetry/document_io.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

void CompleteOwnedWork(D3D12DiagnosticAccess& access, ID3D12Fence& gate) {
    Require(SUCCEEDED(gate.Signal(1)), "Cannot release owned gate");
    const HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(event != nullptr, "Cannot create completion event");
    const HRESULT signaled = access.completion->SetEventOnCompletion(access.last_signal, event);
    const DWORD waited = SUCCEEDED(signaled) ? WaitForSingleObject(event, 3000) : WAIT_FAILED;
    CloseHandle(event);
    Require(SUCCEEDED(signaled) && waited == WAIT_OBJECT_0
            && access.completion->GetCompletedValue() >= access.last_signal, "Owned work failed to complete");
}

void Run(const std::string& mode, Data::Value& report) {
    auto access = std::make_shared<D3D12DiagnosticAccess>();
    ComPtr<ID3D12Fence> gate;
    struct ReleaseGate {
        ComPtr<ID3D12Fence>& value;
        ~ReleaseGate() { if (value) value->Signal(1); }
    } release{gate};
    if (mode == "allocation") access->next_mesh_allocation_result = E_OUTOFMEMORY;
    if (mode == "submission") access->next_frame_signal_result = E_FAIL;
    if (mode == "device-removal") {
        access->before_frame_signal = [](D3D12DiagnosticAccess& control) {
            ComPtr<ID3D12Device5> device;
            Require(SUCCEEDED(control.device.As(&device)), "RemoveDevice is unavailable");
            device->RemoveDevice();
            Require(FAILED(device->GetDeviceRemovedReason()), "Device was not actually removed");
        };
    }
    if (mode == "in-flight-timeout") {
        access->on_initialized = [&](D3D12DiagnosticAccess& control) {
            Require(SUCCEEDED(control.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate))),
                    "Cannot create owned gate");
            Require(SUCCEEDED(control.queue->Wait(gate.Get(), 1)), "Cannot enqueue owned gate");
        };
    }
    ScopedD3D12Diagnostics binding(access);
    std::vector<std::string> arguments{"SymoCraft", "--renderer", "d3d12", "--msaa", "1",
        "--seed", "424242", "--scene", "regression", "--smoke-frames", "2"};
    std::vector<char*> argv;
    for (auto& argument : arguments) argv.push_back(argument.data());
    const int game_exit = Application::RunGame(static_cast<int>(argv.size()), argv.data());
    Require(game_exit == (mode == "normal" ? 0 : 3), "Production game entry changed failure exit classification");
    report["actual_game_entry_exit_code"] = game_exit;
    if (gate) {
        const auto completed = access->completion->GetCompletedValue();
        Require(access->last_signal > completed && completed != UINT64_MAX, "No actual pending texture work");
        report["queued_fence"] = access->last_signal;
        report["completed_fence_at_timeout"] = completed;
    }
    // The production entry has completed instance cleanup; reacquire platform ownership.
    report["entry_cleanup_returned"] = true;
    if (gate) {
        Require(access->quarantined, "Incomplete native work was falsely reclaimed");
        CompleteOwnedWork(*access, *gate.Get());
        report["actual_pending_texture_work"] = true;
        report["terminal_quarantine"] = true;
        report["owned_work_completed_before_process_exit"] = true;
    }
    Platform::Runtime runtime;
    auto window = Window::Create(runtime,"Post-cleanup ownership check", 320, 240, false, true, WindowMode::Native);
    window->Destroy();
    window.reset();
    runtime.Shutdown();
    report["platform_reinitialization_completed"] = true;
}
}

int main(int argc, char** argv) {
    if (argc != 3) return 64;
    const std::string mode = argv[1];
    if (mode != "normal" && mode != "allocation" && mode != "submission"
        && mode != "device-removal" && mode != "in-flight-timeout") return 64;
    const std::filesystem::path path = argv[2];
    if (std::filesystem::exists(path)) return 64;
    Data::Value report;
    report["schema"] = "symocraft.d3d12-game-controlled-faults.v1";
    report["mode"] = mode;
    report["production_application_game_adapter_and_public_facade"] = true;
    report["production_game_entry_and_exit_handling"] = true;
    report["ordinary_game_main_executable"] = false;
    report["spontaneous_device_loss_or_real_oom"] = false;
    report["success"] = false;
    const auto started = Clock::now();
    int result = 1;
    try { Run(mode, report); report["success"] = true; result = 0; }
    catch (const std::exception& error) {
        report["harness_error"] = error.what();
        std::cerr << "[probe] " << error.what() << '\n';
    }
    report["elapsed_ms"] = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
    std::ofstream output(path, std::ios::binary);
    output << Data::DumpYaml(report);
    if (!output) return 2;
    return result;
}
