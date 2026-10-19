#include "diagnostic_access.h"
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/renderer.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
using Color = std::array<int, 3>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct OwnedWindow {
    Platform::Runtime runtime;
    std::unique_ptr<Window> window;
    OwnedWindow() {
        window = Window::Create(runtime, "SymoCraft D3D12 SRV cache GPU verification", 640, 480, false, true, WindowMode::Native);
    }
    ~OwnedWindow() {
        if (window) { window->Destroy(); window.reset(); }
        runtime.Shutdown();
    }
};
MeshData Quad(bool right) {
    MeshData data;
    const int x = right ? 0 : -2;
    const glm::ivec3 p[]{{x,-1,0},{x+2,-1,0},{x+2,1,0},{x,-1,0},{x+2,1,0},{x,1,0}};
    const glm::vec2 uv[]{{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
    for (size_t i = 0; i < 6; ++i) data.vertices.push_back({p[i], {uv[i], right ? 1.f : 0.f}, 0});
    return data;
}
const std::array<Color, 4> PaletteA{{{230,20,40},{20,220,50},{20,40,230},{230,200,20}}};
const std::array<Color, 4> PaletteC{{{20,180,210},{200,30,170},{240,100,20},{30,150,60}}};
const Color RightColor{180,40,190};
TextureArrayData Atlas(const std::array<Color, 4>& palette, Color solid) {
    TextureArrayData data; data.width = data.height = data.layers = 2;
    const auto pixel = [&](Color color) {
        for (int c : color) data.pixels.push_back(static_cast<uint8_t>(c));
        data.pixels.push_back(255);
    };
    for (auto color : palette) pixel(color);
    for (int i = 0; i < 4; ++i) pixel(solid);
    return data;
}
void CheckImage(const Screenshot& image, const std::array<Color, 4>& palette) {
    Require(image.extent.width == 640 && image.extent.height == 480 && image.pixels.size() == 640 * 480 * 4,
        "unexpected actual screenshot extent/storage");
    const std::array<std::array<unsigned, 2>, 5> points{{{192,192},{288,192},{192,288},{288,288},{400,240}}};
    for (size_t i = 0; i < points.size(); ++i) {
        const auto expected = i < 4 ? palette[i] : RightColor;
        const auto offset = (size_t(points[i][1]) * 640 + points[i][0]) * 4;
        for (size_t c = 0; c < 3; ++c)
            Require(std::abs(int(image.pixels.at(offset + c)) - expected[c]) <= 1, "texture binding/UV/color mismatch");
    }
}
void Run(const std::filesystem::path& shaders, const std::filesystem::path& output) {
    OwnedWindow owner;
    auto observed = std::make_shared<D3D12DiagnosticAccess>();
    observed->wait_timeout = RendererConfig{}.wait_timeout;
    ScopedD3D12Diagnostics binding(observed);
    RendererConfig config; config.backend = Backend::D3D12; config.shader_asset_root = shaders;
    config.requested_samples = 4; config.enable_validation = true; config.vsync = false;
    Renderer renderer(*owner.window, config);
    const auto info = renderer.GetInfo();
    Require(info.backend == Backend::D3D12 && info.device_name.find("RTX 5070 Ti") != std::string::npos &&
        info.actual_samples == 4 && info.validation_enabled && info.actual_present_mode == PresentMode::Immediate &&
        info.framebuffer_extent.width == 640 && info.framebuffer_extent.height == 480, "actual hardware/configuration mismatch");
    std::cout << "backend=D3D12; device=" << info.device_name << "; identity=" << info.device_identity << "; driver="
        << info.driver_version << "; api=" << info.api_version << "; samples=4; present=Immediate; validation=1\n";
    const auto left = renderer.CreateMesh(Quad(false)), right = renderer.CreateMesh(Quad(true));
    const auto a = renderer.CreateTextureArray(Atlas(PaletteA, {100,100,100}));
    const auto b = renderer.CreateTextureArray(Atlas(PaletteA, RightColor));
    const auto c = renderer.CreateTextureArray(Atlas(PaletteC, {30,100,190}));
    std::array<MeshDraw, 2> draws{{{left,a},{right,b}}};
    FrameInput frame; frame.camera.position = {0,0,4}; frame.draws = draws; frame.request_screenshot = true;
    std::ofstream rows(output / "observations.csv");
    rows << "stage,frame_id,slot,heap_creations_delta,srv_writes_delta,capacity,slot_heap_creations,slot_srv_writes\n";
    Require(bool(rows), "cannot open native observation output");
    unsigned captures = 0;
    auto stage = [&](const char* name, const std::array<Color, 4>& palette, unsigned writes_per_slot, bool warmup,
                     const std::vector<uint8_t>* reference = nullptr) {
        std::array<bool, 3> visited{};
        std::vector<uint8_t> first;
        for (unsigned attempt = 0; attempt < 24 && !std::all_of(visited.begin(), visited.end(), [](bool x) { return x; }); ++attempt) {
            const auto before = observed->srv_slots;
            owner.window->PollInt(); ++frame.frame_id;
            const auto result = renderer.Render(frame);
            Require(result.status == FrameStatus::Presented && result.screenshot && result.frame_id == frame.frame_id &&
                result.statistics.draw_calls == 2 && result.statistics.drawn_vertices == 12, "normal frame did not draw/present/read back");
            CheckImage(*result.screenshot, palette);
            if (reference) Require(result.screenshot->pixels == *reference, "repeat/reorder/restoration changed expected RGBA image");
            if (first.empty()) {
                first = result.screenshot->pixels;
                Assets::WritePng(output / (std::string(name) + ".png"), 640, 480, 4, first); ++captures;
            } else Require(result.screenshot->pixels == first, "frame slots produce different images");
            const auto slot = observed->last_srv_slot;
            Require(slot < visited.size() && observed->last_srv_frame_id == frame.frame_id, "missing current native slot/frame observation");
            const auto& after = observed->srv_slots[slot];
            const auto heaps = after.heaps_created - before[slot].heaps_created;
            const auto writes = after.views_written - before[slot].views_written;
            const unsigned expected_writes = visited[slot] ? 0 : writes_per_slot;
            rows << name << ',' << frame.frame_id << ',' << slot << ',' << heaps << ',' << writes << ',' << after.capacity
                << ',' << after.heaps_created << ',' << after.views_written << '\n'; rows.flush();
            Require(writes == expected_writes && heaps == (warmup && !visited[slot] ? 1 : 0) &&
                after.heaps_created == 1 && after.capacity == 2, "actual SRV heap reuse/differential write count mismatch");
            for (size_t other = 0; other < visited.size(); ++other) if (other != slot)
                Require(observed->srv_slots[other].heaps_created == before[other].heaps_created &&
                    observed->srv_slots[other].views_written == before[other].views_written, "another frame slot was mutated");
            visited[slot] = true;
        }
        Require(std::all_of(visited.begin(), visited.end(), [](bool x) { return x; }), "all three actual frame slots were not visited");
        return first;
    };
    const auto baseline = stage("initial", PaletteA, 2, true);
    stage("initial-repeat", PaletteA, 0, false, &baseline);
    draws[0].texture_array = c;
    const auto switched = stage("texture-switch", PaletteC, 1, false);
    Require(switched != baseline, "texture switch did not change actual pixels");
    stage("switch-repeat", PaletteC, 0, false, &switched);
    std::swap(draws[0], draws[1]);
    stage("draw-reordered", PaletteC, 2, false, &switched);
    stage("reorder-repeat", PaletteC, 0, false, &switched);
    draws = {{{left,a},{right,b}}};
    stage("restored", PaletteA, 2, false, &baseline);
    stage("restored-repeat", PaletteA, 0, false, &baseline);
    Require(bool(rows), "native observation write failed");
    for (const auto& slot : observed->srv_slots)
        Require(slot.heaps_created == 1 && slot.views_written == 7 && slot.capacity == 2, "final native totals differ");
    renderer.DestroyMesh(left); renderer.DestroyMesh(right);
    renderer.DestroyTextureArray(a); renderer.DestroyTextureArray(b); renderer.DestroyTextureArray(c);
    renderer.Shutdown();
    Require(observed->native_cleared && !observed->quarantined && observed->live_resource_bytes == 0 &&
        observed->mesh_buffers_created == observed->mesh_buffers_released && observed->textures_created == observed->textures_released,
        "normal native shutdown did not retire owners");
    Require(observed->validation_warnings == 0 && observed->validation_errors == 0 && observed->validation_corruptions == 0 &&
        observed->info_queue_checks > 0, "native Debug Layer diagnostics failed");
    observed->queue.Reset(); observed->completion.Reset(); observed->device.Reset();
    std::cout << "normal_gpu_srv_cache=passed; frames=" << frame.frame_id << "; saved_captures=" << captures
        << "; slots=3; heap_creations=3; srv_writes=21; warnings=0; errors=0; corruptions=0; normal_shutdown=passed\n";
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 3, "usage: probe shader-root new-output-directory");
        const auto output = std::filesystem::absolute(argv[2]);
        Require(!std::filesystem::exists(output), "output must be new"); std::filesystem::create_directories(output);
        Run(std::filesystem::absolute(argv[1]), output); return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
