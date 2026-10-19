#include "diagnostic_access.h"
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/telemetry/document_io.h>
#include <array>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>
#include <type_traits>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Microsoft::WRL::ComPtr;
using Value = Data::Value;
using Clock = std::chrono::steady_clock;
void Require(bool ok, const char* text) { if (!ok) throw std::runtime_error(text); }
void Check(HRESULT result) { Require(SUCCEEDED(result), "Owned native diagnostic operation failed"); }

void Json(std::ostream& out, const Value& value) {
    std::visit([&](const auto& item) {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, std::monostate>) out << "null";
        else if constexpr (std::is_same_v<T, bool>) out << (item ? "true" : "false");
        else if constexpr (std::is_same_v<T, std::string>) {
            out << '"'; constexpr char hex[] = "0123456789abcdef";
            for (unsigned char c : item) {
                if (c == '"' || c == '\\') out << '\\' << c;
                else if (c < 32) out << "\\u00" << hex[c >> 4] << hex[c & 15]; else out << c;
            }
            out << '"';
        } else if constexpr (std::is_same_v<T, Value::Mapping>) {
            out << '{'; bool first = true;
            for (const auto& [key, child] : item) {
                if (!first) out << ','; first = false; Json(out, Value(key)); out << ':'; Json(out, child);
            }
            out << '}';
        } else if constexpr (std::is_same_v<T, Value::Sequence>) {
            out << '['; bool first = true;
            for (const auto& child : item) { if (!first) out << ','; first = false; Json(out, child); }
            out << ']';
        } else out << item;
    }, value.storage());
}

struct Gate {
    ComPtr<ID3D12Fence> fence;
    std::mutex mutex;
    std::condition_variable condition;
    bool released{}, watchdog_fired{};
    std::thread watchdog;
    explicit Gate(D3D12DiagnosticAccess& d) {
        Check(d.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        Check(d.queue->Wait(fence.Get(), 1));
        // Only host-signals our own fence; never calls Renderer from this thread.
        try {
            watchdog = std::thread([this] {
                std::unique_lock lock(mutex);
                if (!condition.wait_for(lock, std::chrono::milliseconds(1500), [this] { return released; })) {
                    watchdog_fired = true;
                    fence->Signal(1);
                }
            });
        } catch (...) {
            fence->Signal(1);
            throw;
        }
    }
    void Release() {
        { std::lock_guard lock(mutex); Check(fence->Signal(1)); released = true; }
        condition.notify_all();
        if (watchdog.joinable()) watchdog.join();
        Require(!watchdog_fired, "Gate watchdog fired: not valid controlled in-flight evidence");
    }
    ~Gate() {
        { std::lock_guard lock(mutex); fence->Signal(1); released = true; }
        condition.notify_all(); if (watchdog.joinable()) watchdog.join();
    }
};

void Await(D3D12DiagnosticAccess& d) {
    const HANDLE event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    Require(event != nullptr, "Cannot create bounded native completion event");
    const HRESULT result = d.completion->SetEventOnCompletion(d.last_signal, event);
    const auto waited = SUCCEEDED(result) ? WaitForSingleObject(event, 3000) : WAIT_FAILED;
    CloseHandle(event);
    Require(SUCCEEDED(result) && waited == WAIT_OBJECT_0 && d.completion->GetCompletedValue() >= d.last_signal,
            "Owned native work did not complete within bound");
}

void Pending(D3D12DiagnosticAccess& d, Gate& gate) {
    const auto completed = d.completion->GetCompletedValue();
    Require(d.last_signal > completed && completed != UINT64_MAX && gate.fence->GetCompletedValue() == 0,
            "No actual native pending submission behind gate");
}

template<class F> void Reject(RenderErrorCategory category, F&& action) {
    try { action(); }
    catch (const RenderError& error) { Require(error.Category() == category, "Unexpected rejection category"); return; }
    throw std::runtime_error("Expected rejection did not occur");
}

struct Scene {
    std::shared_ptr<D3D12DiagnosticAccess> d = std::make_shared<D3D12DiagnosticAccess>();
    ScopedD3D12Diagnostics binding{d};
    Renderer renderer;
    TextureArrayData texture;
    MeshData mesh;
    TextureArrayHandle tex;
    MeshHandle handle;
    std::uint64_t next_id = 1;
    std::filesystem::path output;
    static RendererConfig Config(const std::filesystem::path& shaders) {
        RendererConfig c; c.vsync = false; c.shader_asset_root = shaders; return c;
    }
    Scene(Window& window, const std::filesystem::path& shaders, const std::filesystem::path& dir, Value& report)
        : renderer(window, Config(shaders)), output(dir) {
        const auto info = renderer.GetInfo();
        Require(info.backend == Backend::D3D12 && info.device_name == "NVIDIA GeForce RTX 5070 Ti" &&
                info.validation_enabled && info.actual_samples == 1 && info.capabilities.gpu_timestamps &&
                info.actual_present_mode == PresentMode::Immediate, "Actual device/configuration differs from local scope");
        report["device"] = info.device_name; report["identity"] = info.device_identity;
        report["driver"] = info.driver_version; report["api"] = info.api_version;
        report["samples"] = info.actual_samples; report["validation_enabled"] = true;
        texture.width = texture.height = texture.layers = 1; texture.pixels = {230, 20, 40, 255};
        mesh.vertices = {{{-1,-1,-2},{0,0,0},1.0f}, {{1,-1,-2},{1,0,0},1.0f}, {{0,1,-2},{0.5f,1,0},1.0f}};
        tex = renderer.CreateTextureArray(texture); handle = renderer.CreateMesh(mesh);
    }
    FrameResult Draw(bool screenshot = false, bool empty = false) {
        const std::array draws{MeshDraw{handle, tex}};
        FrameInput frame; frame.frame_id = next_id++; frame.request_screenshot = screenshot;
        if (!empty) frame.draws = draws;
        auto result = renderer.Render(frame);
        Require(result.frame_id == frame.frame_id && result.status == FrameStatus::Presented, "Draw not actually presented");
        return result;
    }
    std::vector<std::uint8_t> Capture(const std::string& name, bool empty = false) {
        const auto frame = Draw(true, empty); Require(frame.screenshot.has_value(), "Missing native screenshot");
        const auto& image = *frame.screenshot;
        Assets::WritePng(output/(name+".png"), int(image.extent.width), int(image.extent.height), 4, image.pixels);
        return image.pixels;
    }
    void DrainSlots() { for (int i = 0; i < 4; ++i) { Draw(false, true); Await(*d); } }
    void Finish(Value& report) {
        renderer.DestroyMesh(handle); renderer.DestroyTextureArray(tex);
        renderer.Shutdown(); renderer.Shutdown();
        Require(d->live_resource_bytes == 0 && d->mesh_buffers_created == d->mesh_buffers_released &&
                d->textures_created == d->textures_released && d->native_cleared && !d->quarantined,
                "Persistent native resources did not retire normally");
        Require(d->info_queue_checks > 0 && d->validation_warnings == 0 && d->validation_errors == 0 &&
                d->validation_corruptions == 0, "Native Debug Layer diagnostic");
        report["mesh_buffers_created"] = d->mesh_buffers_created; report["mesh_buffers_released"] = d->mesh_buffers_released;
        report["textures_created"] = d->textures_created; report["textures_released"] = d->textures_released;
        report["live_resource_bytes"] = d->live_resource_bytes; report["peak_resource_bytes"] = d->peak_resource_bytes;
        report["resource_bytes_scope"] = "GetResourceAllocationInfo bytes of successful mesh buffers and accepted texture resources; excludes frame uploads, attachments and process/VRAM budgets";
        report["native_cleared"] = d->native_cleared; report["quarantined"] = d->quarantined;
        report["info_queue_checks"] = d->info_queue_checks; report["validation_warnings"] = d->validation_warnings;
        report["validation_errors"] = d->validation_errors; report["validation_corruptions"] = d->validation_corruptions;
        report["sample_drops"] = d->sample_drops;
    }
};

void Run(const std::string& mode, Scene& s, Value& report) {
    const auto baseline = s.Capture("baseline"); s.renderer.PollGpuSamples();
    if (mode == "lifetime") {
        Value::Sequence events;
        for (int step = 0; step < 3; ++step) {
            s.renderer.UpdateMesh(s.handle, s.mesh);
            const auto old_mesh = s.d->last_mesh, old_texture = s.d->last_texture;
            Value event; event["step"] = step;
            {
                Gate gate(*s.d); s.Draw(); Pending(*s.d, gate);
                Require(!old_mesh.expired() && s.d->frame_mesh.lock() == old_mesh.lock(), "Native frame did not retain accepted mesh");
                Require(s.d->frame_texture.lock() == old_texture.lock(), "Native frame did not retain accepted texture");
                const auto live = s.d->live_resource_bytes;
                s.d->next_mesh_allocation_result = E_OUTOFMEMORY;
                Reject(RenderErrorCategory::AllocationFailure, [&] { s.renderer.UpdateMesh(s.handle, s.mesh); });
                s.d->next_mesh_allocation_result = E_OUTOFMEMORY;
                Reject(RenderErrorCategory::AllocationFailure, [&] { s.renderer.CreateMesh(s.mesh); });
                Require(s.d->live_resource_bytes == live && s.d->last_mesh.lock() == old_mesh.lock(), "Failed allocation published/leaked native candidate");
                event["allocation_failure_preserved_old_version"] = true;
                if (step == 0) {
                    auto changed = s.mesh; for (auto& v : changed.vertices) v.pos_coord.x += 20;
                    s.renderer.UpdateMesh(s.handle, changed);
                    Require(s.d->last_mesh.lock() != old_mesh.lock(), "Update overwrote old native version");
                } else if (step == 1) s.renderer.UpdateMesh(s.handle, {});
                else {
                    s.renderer.DestroyMesh(s.handle); s.renderer.DestroyTextureArray(s.tex);
                    Reject(RenderErrorCategory::InvalidHandle, [&] { s.renderer.UpdateMesh(s.handle, s.mesh); });
                }
                std::vector<MeshHandle> capacity;
                for (int i = 0; i < 48; ++i) capacity.push_back(s.renderer.CreateMesh(s.mesh));
                for (auto h : capacity) s.renderer.DestroyMesh(h);
                Pending(*s.d, gate);
                Require(!old_mesh.expired() && !old_texture.expired(), "In-flight native resource was released early");
                event["pending_submit_fence"] = s.d->last_signal;
                event["completed_before_release"] = s.d->completion->GetCompletedValue();
                event["gate_before_release"] = gate.fence->GetCompletedValue();
                event["old_versions_retained"] = true; event["capacity_creates"] = 48;
                gate.Release(); Await(*s.d);
                event["watchdog_fired"] = gate.watchdog_fired;
            }
            s.DrainSlots();
            Require(old_mesh.expired() && (step != 2 || old_texture.expired()), "Completed native version not retired after slot reuse");
            event["old_mesh_retired"] = true; event["deleted_texture_retired"] = step == 2;
            const auto cleared = s.Capture("after-"+std::to_string(step), step == 2);
            Require(cleared != baseline, "Actual update/empty/destroy did not clear old image");
            if (step == 2) { s.tex = s.renderer.CreateTextureArray(s.texture); s.handle = s.renderer.CreateMesh(s.mesh); }
            else s.renderer.UpdateMesh(s.handle, s.mesh);
            Require(s.Capture("restored-"+std::to_string(step)) == baseline, "Restored native content changed");
            events.push_back(std::move(event));
        }
        report["events"] = std::move(events);
    } else if (mode == "async-pending") {
        s.next_id = 9007199254740993ULL;
        const auto pending_id = s.next_id;
        Gate gate(*s.d); s.Draw(); Pending(*s.d, gate);
        const auto waits = s.d->wait_calls, reads = s.d->query_readbacks;
        for (int i = 0; i < 10; ++i) Require(s.renderer.PollGpuSamples().empty(), "Unready native query fabricated sample");
        Pending(*s.d, gate);
        Require(s.d->wait_calls == waits && s.d->query_readbacks == reads, "Unready Poll waited or mapped query readback");
        report["pending_poll_calls"] = 10; report["poll_added_waits"] = s.d->wait_calls-waits;
        report["poll_added_query_reads"] = s.d->query_readbacks-reads;
        report["pending_submit_fence"] = s.d->last_signal;
        report["completed_before_release"] = s.d->completion->GetCompletedValue();
        gate.Release(); Await(*s.d);
        const auto samples = s.renderer.PollGpuSamples();
        Require(samples.size() == 1 && samples[0].frame_id == pending_id && samples[0].valid &&
                samples[0].gpu_draw_time.has_value() && samples[0].missing_reason == GpuSampleMissingReason::None,
                "Completed query lost exact original frame identity");
        Require(s.renderer.PollGpuSamples().empty(), "GPU query returned twice");
        report["resolved_frame_id"] = samples[0].frame_id; report["watchdog_fired"] = gate.watchdog_fired;
    } else {
        Value::Sequence submitted, returned; std::set<std::uint64_t> unique;
        s.next_id = 9007199254740993ULL;
        for (int i = 0; i < 301; ++i) {
            submitted.emplace_back(s.next_id); s.Draw(); s.next_id += 2; Await(*s.d);
        }
        const auto samples = s.renderer.PollGpuSamples();
        Require(samples.size() == 256 && s.d->sample_drops == 45, "Unexpected native result capacity");
        for (const auto& sample : samples) {
            Require(sample.valid && sample.gpu_draw_time.has_value() && sample.missing_reason == GpuSampleMissingReason::None &&
                    unique.insert(sample.frame_id).second, "Invalid/duplicated native GPU sample");
            returned.emplace_back(sample.frame_id);
        }
        Require(s.renderer.PollGpuSamples().empty(), "Native ring returned twice");
        report["submitted_ids"] = std::move(submitted); report["returned_ids"] = std::move(returned);
    }
    s.Finish(report);
}
}

int main(int argc, char** argv) {
    if (argc != 4) return 64;
    const std::string mode = argv[1]; const std::filesystem::path output = argv[3];
    if ((mode != "lifetime" && mode != "async-pending" && mode != "async-overflow") || std::filesystem::exists(output)) return 64;
    std::filesystem::create_directories(output);
    Value report; report["schema"] = "symocraft.d3d12-lifetime-async.v1"; report["mode"] = mode;
    report["success"] = false; report["public_facade_and_production_backend"] = true;
    report["performance_acceptance"] = false;
    const auto started = Clock::now(); int code = 1;
    try {
        Platform::Runtime runtime;
        auto window = Window::Create(runtime,"Symocraft D3D12 lifetime/async",640,360,false,true,WindowMode::Native);
        { Scene scene(*window, argv[2], output, report); Run(mode, scene, report); }
        window->Destroy(); window.reset(); runtime.Shutdown();
        report["success"] = true; code = 0;
    } catch (const std::exception& error) { report["harness_error"] = error.what(); std::cerr << error.what() << '\n'; }
    report["elapsed_ms"] = std::chrono::duration<double,std::milli>(Clock::now()-started).count();
    std::ofstream file(output/"report.json"); Json(file,report); file << '\n'; return file ? code : 2;
}
