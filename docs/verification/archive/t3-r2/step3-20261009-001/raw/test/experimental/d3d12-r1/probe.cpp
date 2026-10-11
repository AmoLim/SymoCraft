#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <native_bridge.h>
#include <symocraft/platform/window.h>
#include "renderer_probe.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
using namespace SymoCraft::Experimental::D3D12R1;
using SymoCraft::Window;
using Clock = std::chrono::steady_clock;

void Require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void CleanupDiagnostic(std::ostream& output, const char* stage, const char* message) noexcept {
    try { output << stage << '=' << message << '\n'; }
    catch (...) {}
}

std::string Quote(const std::string& value) {
    std::ostringstream out;
    out << '"';
    constexpr char digits[] = "0123456789abcdef";
    for (const unsigned char ch : value) {
        if (ch == '"' || ch == '\\') out << '\\' << ch;
        else if (ch < 32) out << "\\u00" << digits[ch >> 4] << digits[ch & 15];
        else out << ch;
    }
    out << '"';
    return out.str();
}

struct Report {
    std::map<std::string, std::string> fields;
    void String(const std::string& name, const std::string& value) { fields[name] = Quote(value); }
    void Flag(const std::string& name, bool value) { fields[name] = value ? "true" : "false"; }
    template<class T> void Number(const std::string& name, T value) {
        fields[name] = std::to_string(value);
    }
    void Save(const std::filesystem::path& path) const {
        std::ofstream out(path);
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out << "{\n";
        bool first = true;
        for (const auto& [name, value] : fields) {
            if (!first) out << ",\n";
            out << "  " << Quote(name) << ": " << value;
            first = false;
        }
        out << "\n}\n";
    }
};

SymoCraft::MeshData Fixture(bool swap_layers = false) {
    SymoCraft::MeshData mesh;
    const auto quad = [&](int x0, int x1, int layer) {
        const float z = static_cast<float>(swap_layers ? 1 - layer : layer);
        const SymoCraft::BlockVertex3D tl{{x0, 0, 0}, {0, 0, z}, 0};
        const SymoCraft::BlockVertex3D tr{{x1, 0, 0}, {1, 0, z}, 0};
        const SymoCraft::BlockVertex3D bl{{x0, 4, 0}, {0, 1, z}, 0};
        const SymoCraft::BlockVertex3D br{{x1, 4, 0}, {1, 1, z}, 0};
        mesh.vertices.insert(mesh.vertices.end(), {tl, tr, bl, tr, br, bl});
    };
    quad(0, 2, 0);
    quad(2, 4, 1);
    return mesh;
}

std::vector<std::uint8_t> Pixels() {
    return {255,0,0,255, 0,255,0,255, 0,0,255,255, 255,255,255,255,
            255,255,0,255, 0,255,255,255, 128,128,128,255, 255,0,255,0};
}

std::array<int, 4> Pixel(const Image& image, double x, double y) {
    Require(image.extent.width > 0 && image.extent.height > 0, "Empty screenshot extent");
    const auto px = static_cast<std::size_t>(x * image.extent.width);
    const auto py = static_cast<std::size_t>(y * image.extent.height);
    const auto offset = (py * image.extent.width + px) * 4;
    Require(offset + 4 <= image.rgba.size(), "Screenshot pixel outside RGBA storage");
    return {image.rgba[offset], image.rgba[offset+1], image.rgba[offset+2], image.rgba[offset+3]};
}

void CheckPixel(const Image& image, double x, double y, std::array<int, 4> expected) {
    const auto actual = Pixel(image, x, y);
    for (std::size_t channel = 0; channel < actual.size(); ++channel) {
        if (std::abs(actual[channel] - expected[channel]) > 2) {
            std::ostringstream error;
            error << "Pixel mismatch at " << x << ',' << y << " channel=" << channel
                  << " expected=" << expected[channel] << " actual=" << actual[channel];
            throw std::runtime_error(error.str());
        }
    }
}

void CheckLayers(const Image& image, bool swapped = false, bool srgb = false) {
    const double left = swapped ? 0.5 : 0.0;
    const double right = swapped ? 0.0 : 0.5;
    CheckPixel(image, left + .125, .25, {255,0,0,255});
    CheckPixel(image, left + .375, .25, {0,255,0,255});
    CheckPixel(image, left + .125, .75, {0,0,255,255});
    CheckPixel(image, left + .375, .75, {255,255,255,255});
    CheckPixel(image, right + .125, .25, {255,255,0,255});
    CheckPixel(image, right + .375, .25, {0,255,255,255});
    const int gray = srgb ? 128 : 188;
    CheckPixel(image, right + .125, .75, {gray,gray,gray,255});
    CheckPixel(image, right + .375, .75, {0,0,0,255});
}

const Image& Capture(const FrameResult& frame) {
    Require(frame.presented && !frame.skipped && frame.screenshot.has_value(), "Frame did not present/read back");
    const auto& image = *frame.screenshot;
    Require(image.rgba.size() == static_cast<std::size_t>(image.extent.width) * image.extent.height * 4,
            "Screenshot storage is not tightly packed RGBA8");
    return image;
}

void SaveImage(const std::filesystem::path& output, const char* name, const Image& image) {
    const auto path = output / name;
    Require(stbi_write_png(path.string().c_str(), static_cast<int>(image.extent.width),
        static_cast<int>(image.extent.height), 4, image.rgba.data(),
        static_cast<int>(image.extent.width * 4)) != 0, "PNG write failed");
}

template<class Operation> void MustReject(Operation operation) {
    bool rejected = false;
    try { operation(); }
    catch (const std::invalid_argument&) { rejected = true; }
    catch (const std::logic_error&) { rejected = true; }
    Require(rejected, "Stale resource handle was not rejected");
}

Extent ActualExtent(Window& window, HWND hwnd) {
    window.PollInt();
    RECT client{};
    Require(GetClientRect(hwnd, &client) != 0, "GetClientRect failed");
    Require(window.width == client.right && window.height == client.bottom,
            "Platform pixel size disagrees with actual native client extent");
    return {static_cast<std::uint32_t>(window.width), static_cast<std::uint32_t>(window.height)};
}

void AwaitWindow(Window& window, const std::function<bool()>& condition) {
    const auto deadline = Clock::now() + std::chrono::seconds(3);
    do {
        window.PollInt();
        if (condition()) return;
        window.WaitEvents(.02);
    } while (Clock::now() < deadline);
    throw std::runtime_error("Window operation unavailable after bounded event wait");
}
}

int main(int argc, char** argv) {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::filesystem::path output;
    std::filesystem::path shaders = SYMOCRAFT_R1_SHADER_DIRECTORY;
    bool inject_timeout = false;
    try {
        std::set<std::string> seen;
        for (int i = 1; i < argc; ++i) {
            const std::string option = argv[i];
            Require(seen.insert(option).second, "Duplicate probe option");
            const auto value = [&]() {
                if (i + 1 >= argc || std::string(argv[i + 1]).starts_with("--"))
                    throw std::invalid_argument("Unknown or incomplete probe argument: " + option);
                return std::string(argv[++i]);
            };
            if (option == "--output") output = value();
            else if (option == "--shaders") shaders = value();
            else if (option == "--inject-timeout") inject_timeout = true;
            else if (option == "--require-debug-layer") {}
            else throw std::invalid_argument("Unknown or incomplete probe argument: " + option);
        }
        Require(!output.empty(), "--output is required");
        output = std::filesystem::absolute(output);
        std::filesystem::create_directories(output);
        Require(!std::filesystem::exists(output / "result.json") &&
            !std::filesystem::exists(output / "lifecycle.log"), "Do not overwrite an earlier experiment result");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }

    Report report;
    report.String("status", "fail");
    report.String("backend", "D3D12");
    report.String("window_mode", "Native SDL3 / private HWND bridge");
    report.Flag("timeout_injected", inject_timeout);
    report.Flag("e01_passed", false);
    report.Flag("e02_passed", false);
    report.Flag("e03_passed", false);
    report.Flag("shutdown_completed", false);
    std::ofstream lifecycle;
    bool initialized = false;
    std::unique_ptr<Window> window;
    std::unique_ptr<ProbeRenderer> renderer;
    int exit_code = 1;
    const auto start = Clock::now();
    try {
        lifecycle.exceptions(std::ios::badbit | std::ios::failbit);
        lifecycle.open(output / "lifecycle.log");
        lifecycle << std::unitbuf;
        Window::Init();
        initialized = true;
        window.reset(Window::Create("SymoCraft T3 R1 D3D12", 640, 480, false, true, SymoCraft::WindowMode::Native));
        const HWND hwnd = SymoCraft::Platform::NativeBridge::GetHandle(*window);
        const auto initial_extent = ActualExtent(*window, hwnd);
        renderer = std::make_unique<ProbeRenderer>(hwnd, initial_extent, shaders, lifecycle, true);
        const auto info = renderer->Info();
        report.String("adapter", info.name);
        report.String("adapter_luid", info.luid);
        report.Number("adapter_vendor_id", info.vendorId);
        report.Number("adapter_device_id", info.deviceId);
        report.Flag("device_luid_matches_dxgi_adapter", true);
        report.String("driver", info.driver);
        report.String("feature_level", info.featureLevel);
        report.String("shader_model", info.shaderModel);
        report.String("render_target_format", info.renderTargetFormat);
        report.Flag("software_adapter", info.softwareAdapter);
        report.Flag("debug_layer_enabled", info.debugLayerEnabled);
        Require(info.vendorId == 0x10de && info.deviceId == 0x2c05 &&
                info.name == "NVIDIA GeForce RTX 5070 Ti",
                "Current R1 validation requires the actual local RTX 5070 Ti; another adapter is not accepted");
        const auto record_presentation = [&](const std::string& prefix) {
            const auto actual = renderer->Presentation();
            report.Number(prefix + "width", actual.extent.width);
            report.Number(prefix + "height", actual.extent.height);
            report.Number(prefix + "format", actual.format);
            report.Number(prefix + "buffer_count", actual.bufferCount);
            report.Number(prefix + "sample_count", actual.sampleCount);
            report.Number(prefix + "sample_quality", actual.sampleQuality);
            report.Number(prefix + "swap_effect", actual.swapEffect);
            report.Number(prefix + "flags", actual.flags);
            report.Flag(prefix + "windowed", actual.windowed);
        };
        record_presentation("swapchain_initial_");
        report.Number("present_sync_interval", 0);
        report.Number("present_flags", 0);
        report.Number("cpu_vertex_stride", sizeof(SymoCraft::BlockVertex3D));
        auto pixels = Pixels();
        renderer->CreateTexture(pixels, 2, 2, 2, false);
        report.String("texture_format", renderer->Info().textureFormat);
        std::fill(pixels.begin(), pixels.end(), 0);
        auto mesh = Fixture();
        const auto handle = renderer->UploadMesh(mesh);
        mesh.vertices.clear();
        auto initial = renderer->Render(handle, true);
        CheckLayers(Capture(initial));
        SaveImage(output, "e01-linear.png", Capture(initial));

        if (inject_timeout) {
            renderer->SetInjectTimeout(true);
            renderer->Resize({800, 480});
            throw std::runtime_error("Injected timeout did not fail the GPU wait");
        }

        renderer->BeginGpuGate();
        const auto held = renderer->Render(handle, false, false);
        Require(held.submittedFence > held.completedFence, "GPU gate did not prove an incomplete submission");
        auto updated = Fixture(true);
        renderer->UpdateMesh(handle, updated);
        updated.vertices.clear();
        const auto pending_update = renderer->Stats();
        Require(pending_update.retiredPending > 0 && pending_update.completedFence < held.submittedFence,
                "Old mesh was not retained while the submitted frame was incomplete");
        report.Number("update_inflight_fence", held.submittedFence);
        report.Number("update_completed_at_retirement", pending_update.completedFence);
        renderer->EndGpuGate();
        auto update_frame = renderer->Render(handle, true);
        CheckLayers(Capture(update_frame), true);
        SaveImage(output, "e02-updated.png", Capture(update_frame));

        renderer->BeginGpuGate();
        const auto held_delete = renderer->Render(handle, false, false);
        Require(held_delete.submittedFence > held_delete.completedFence, "Delete experiment had no incomplete frame");
        renderer->DestroyMesh(handle);
        MustReject([&] { renderer->Render(handle, false, false); });
        MustReject([&] { renderer->UpdateMesh(handle, Fixture()); });
        const auto pending_delete = renderer->Stats();
        Require(pending_delete.retiredPending > 0 && pending_delete.completedFence < held_delete.submittedFence,
                "Deleted mesh was reclaimed before its frame completed");
        report.Number("delete_inflight_fence", held_delete.submittedFence);
        report.Number("delete_completed_at_retirement", pending_delete.completedFence);
        renderer->EndGpuGate();

        const auto empty_handle = renderer->UploadMesh({});
        const auto empty_frame = renderer->Render(empty_handle, true);
        for (const double x : {.125, .375, .625, .875})
            for (const double y : {.25, .75}) CheckPixel(Capture(empty_frame), x, y, {0,0,0,255});
        SaveImage(output, "e02-empty.png", Capture(empty_frame));
        renderer->UpdateMesh(empty_handle, Fixture());
        renderer->UpdateMesh(empty_handle, {});
        const auto emptied_update = renderer->Render(empty_handle, true);
        CheckPixel(Capture(emptied_update), .125, .25, {0,0,0,255});
        renderer->UpdateMesh(empty_handle, Fixture());
        Require(renderer->Stats().retiredPending == 0, "Completed retired mesh resources were not collected");
        report.Flag("e02_passed", true);
        lifecycle << "e02_completed entering_window_resize=true\n";

        for (const auto [width, height] : {std::pair{800, 480}, std::pair{512, 384}, std::pair{720, 540}}) {
            lifecycle << "resize_query_before request=" << width << 'x' << height << '\n';
            const auto before = ActualExtent(*window, hwnd);
            lifecycle << "resize_setsize_begin\n";
            window->SetSize(width, height);
            lifecycle << "resize_setsize_returned waiting_for_pixels=true\n";
            AwaitWindow(*window, [&] { return window->width > 0 && window->height > 0 &&
                (window->width != static_cast<int>(before.width) || window->height != static_cast<int>(before.height)); });
            const auto extent = ActualExtent(*window, hwnd);
            renderer->Resize(extent);
            renderer->Resize(extent);
            const auto resized = renderer->Render(empty_handle, true);
            CheckLayers(Capture(resized));
            Require(Capture(resized).extent.width == extent.width && Capture(resized).extent.height == extent.height,
                    "Resize readback dimensions disagree with actual framebuffer");
            lifecycle << "window_pixels=" << extent.width << 'x' << extent.height << '\n';
        }
        ShowWindow(hwnd, SW_MINIMIZE);
        AwaitWindow(*window, [&] { return window->Minimized(); });
        // Windows may retain a positive client extent when minimized; zero is an explicit contract input.
        report.Flag("zero_extent_input_is_injected", true);
        renderer->Resize({0, 0});
        const auto before_pause = renderer->Stats().submittedFrames;
        const auto paused_until = Clock::now() + std::chrono::milliseconds(10100);
        std::uint64_t pause_polls = 0;
        while (Clock::now() < paused_until) {
            const auto skipped = renderer->Render(empty_handle);
            Require(skipped.skipped && !skipped.presented, "Zero extent did not skip rendering");
            window->WaitEvents(.1);
            window->PollInt();
            ++pause_polls;
        }
        Require(renderer->Stats().submittedFrames == before_pause, "Suspended renderer submitted GPU work");
        report.Number("suspended_event_waits", pause_polls);
        report.Number("suspended_submissions", renderer->Stats().submittedFrames - before_pause);
        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        ShowWindow(hwnd, SW_RESTORE);
        AwaitWindow(*window, [&] { return !window->Minimized() && window->width > 0 && window->height > 0; });
        const auto restored_extent = ActualExtent(*window, hwnd);
        renderer->Resize(restored_extent);
        const auto restored = renderer->Render(empty_handle, true);
        CheckLayers(Capture(restored));
        SaveImage(output, "e03-restored.png", Capture(restored));
        report.Number("width", restored_extent.width);
        report.Number("height", restored_extent.height);
        record_presentation("swapchain_restored_");
        report.Flag("e03_passed", true);
        renderer->DestroyMesh(empty_handle);
        renderer->Shutdown();
        renderer->Shutdown();
        const auto stats = renderer->Stats();
        report.Number("submitted_frames", stats.submittedFrames);
        report.Number("retired_collected", stats.retiredCollected);
        report.Number("debug_error_count", stats.debugErrors);
        report.Number("debug_warning_count", stats.debugWarnings);
        Require(stats.debugErrors == 0 && stats.debugWarnings == 0, "Unresolved D3D12 debug diagnostics");
        Require(stats.retiredPending == 0, "Shutdown left retired mesh resources pending");
        renderer.reset();

        renderer = std::make_unique<ProbeRenderer>(hwnd, restored_extent, shaders, lifecycle, true);
        Require(renderer->Info().luid == info.luid,
                "sRGB renderer changed the actual adapter");
        const auto srgb_pixels = Pixels();
        renderer->CreateTexture(srgb_pixels, 2, 2, 2, true);
        const auto srgb_mesh = renderer->UploadMesh(Fixture());
        const auto srgb_frame = renderer->Render(srgb_mesh, true);
        CheckLayers(Capture(srgb_frame), false, true);
        SaveImage(output, "e01-srgb.png", Capture(srgb_frame));
        renderer->Shutdown();
        const auto srgb_stats = renderer->Stats();
        report.Number("debug_error_count", stats.debugErrors + srgb_stats.debugErrors);
        report.Number("debug_warning_count", stats.debugWarnings + srgb_stats.debugWarnings);
        Require(srgb_stats.debugErrors == 0 && srgb_stats.debugWarnings == 0, "sRGB run has unresolved debug diagnostics");
        report.Flag("e01_passed", true);
        report.Flag("shutdown_completed", true);
        renderer.reset();
        window->Destroy();
        window.reset();
        Window::Free();
        initialized = false;
        report.String("status", "pass");
        exit_code = 0;
    } catch (const std::exception& error) {
        report.String("error", error.what());
        std::cerr << "D3D12 R1: " << error.what() << '\n';
        if (renderer) {
            try {
                const auto stats = renderer->Stats();
                report.Number("debug_error_count", stats.debugErrors);
                report.Number("debug_warning_count", stats.debugWarnings);
            } catch (const std::exception& diagnostic) {
                CleanupDiagnostic(lifecycle, "failure_stats", diagnostic.what());
            }
        }
    }

    // Preserve the primary failure while still attempting every owned cleanup stage.
    if (renderer) {
        try { renderer->Shutdown(); }
        catch (const std::exception& error) { CleanupDiagnostic(lifecycle, "cleanup_renderer", error.what()); }
        try {
            const auto stats = renderer->Stats();
            report.Number("debug_error_count", stats.debugErrors);
            report.Number("debug_warning_count", stats.debugWarnings);
        } catch (const std::exception& error) { CleanupDiagnostic(lifecycle, "cleanup_stats", error.what()); }
        renderer.reset();
    }
    if (window) {
        try { window->Destroy(); }
        catch (const std::exception& error) { CleanupDiagnostic(lifecycle, "cleanup_window", error.what()); }
        window.reset();
    }
    if (initialized) {
        try { Window::Free(); }
        catch (const std::exception& error) { CleanupDiagnostic(lifecycle, "cleanup_video", error.what()); }
    }
    report.Flag("cleanup_completed", true);
    report.Number("elapsed_ms", std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count());
    try {
        report.Save(output / "result.json");
        lifecycle.flush();
    } catch (const std::exception& error) {
        std::cerr << "Evidence write: " << error.what() << '\n';
        return 1;
    }
    return exit_code;
}
