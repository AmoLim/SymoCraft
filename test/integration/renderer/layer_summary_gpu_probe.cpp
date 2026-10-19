#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/renderer.h>
#include <array>
#include <cmath>
#include <iostream>
#include <memory>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using Color = std::array<int, 3>;
void Require(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
struct OwnedWindow {
    Platform::Runtime runtime;
    std::unique_ptr<Window> window;
    explicit OwnedWindow(Backend backend) {
        window = Window::Create(runtime, "SymoCraft layer summary GPU verification", 640, 480, false, true,
            backend == Backend::Vulkan ? WindowMode::Vulkan : WindowMode::Native);
    }
    ~OwnedWindow() {
        if (window) { window->Destroy(); window.reset(); }
        runtime.Shutdown();
    }
};
MeshData Quads(unsigned right_layer) {
    MeshData data;
    for (int side = 0; side < 2; ++side) {
        const int x = side ? 0 : -2;
        const glm::ivec3 p[]{{x,-1,0},{x+2,-1,0},{x+2,1,0},{x,-1,0},{x+2,1,0},{x,1,0}};
        const glm::vec2 uv[]{{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
        for (size_t i = 0; i < 6; ++i)
            data.vertices.push_back({p[i], {uv[i], side ? float(right_layer) : 0.f}, 0});
    }
    return data;
}
TextureArrayData Atlas(ColorSpace space) {
    TextureArrayData data;
    data.width = data.height = 2; data.layers = 4; data.color_space = space;
    data.pixels = {230,20,40,255,20,220,50,255,20,40,230,255,230,200,20,255};
    for (Color color : {Color{128,128,128}, Color{30,180,210}, Color{210,50,180}})
        for (int pixel = 0; pixel < 4; ++pixel) {
            for (int c : color) data.pixels.push_back(static_cast<uint8_t>(c));
            data.pixels.push_back(255);
        }
    return data;
}
Color Pixel(const Screenshot& image, unsigned x, unsigned y) {
    const auto i = (size_t(y) * image.extent.width + x) * 4;
    return {image.pixels.at(i), image.pixels.at(i + 1), image.pixels.at(i + 2)};
}
void CheckImage(const Screenshot& image, unsigned layer, bool srgb) {
    Require(image.extent.width == 640 && image.extent.height == 480 && image.pixels.size() == 640 * 480 * 4,
        "unexpected screenshot storage/extent");
    std::array<Color, 5> expected{{{230,20,40},{20,220,50},{20,40,230},{230,200,20},
        layer == 1 ? Color{128,128,128} : Color{30,180,210}}};
    if (srgb) for (auto& color : expected) for (int& channel : color) {
        const double c = channel / 255.;
        channel = int(std::lround((c <= .04045 ? c / 12.92 : std::pow((c + .055) / 1.055, 2.4)) * 255));
    }
    // The fixed camera places these probes safely inside the four left UV patches and right layer.
    const std::array<std::array<unsigned, 2>, 5> points{{{192,192},{288,192},{192,288},{288,288},{400,240}}};
    for (size_t i = 0; i < points.size(); ++i) {
        const auto actual = Pixel(image, points[i][0], points[i][1]);
        for (size_t c = 0; c < 3; ++c)
            Require(std::abs(actual[c] - expected[i][c]) <= 1, "UV/color/accepted texture layer mismatch");
    }
}
void Run(Backend backend, const std::filesystem::path& shaders, const std::filesystem::path& output) {
    OwnedWindow owner(backend);
    RendererConfig config; config.backend = backend; config.shader_asset_root = shaders;
    config.requested_samples = 4; config.enable_validation = true; config.vsync = false;
    Renderer renderer(*owner.window, config);
    const auto info = renderer.GetInfo();
    Require(info.backend == backend && info.device_name.find("RTX 5070 Ti") != std::string::npos &&
        info.actual_samples == 4 && info.validation_enabled && info.actual_present_mode == PresentMode::Immediate,
        "actual backend/GPU/configuration mismatch");
    Require(info.framebuffer_extent.width == 640 && info.framebuffer_extent.height == 480, "drawable size mismatch");
    std::cout << "backend=" << (backend == Backend::D3D12 ? "D3D12" : "Vulkan") << "; device=" << info.device_name
        << "; identity=" << info.device_identity << "; driver=" << info.driver_version << "; api=" << info.api_version
        << "; samples=4; present=Immediate; validation=1\n";
    auto source = Quads(1);
    const auto mesh = renderer.CreateMesh(source); source.vertices.clear();
    const auto linear = renderer.CreateTextureArray(Atlas(ColorSpace::Linear));
    const auto srgb = renderer.CreateTextureArray(Atlas(ColorSpace::SRGB));
    std::array<MeshDraw, 1> draws{{{mesh, linear}}};
    FrameInput frame; frame.camera.position = {0, 0, 4}; frame.draws = draws;
    unsigned captures = 0;
    auto render = [&](const char* name, unsigned layer, bool decode) {
        owner.window->PollInt(); ++frame.frame_id; frame.request_screenshot = true;
        const auto result = renderer.Render(frame);
        Require(result.status == FrameStatus::Presented && result.screenshot && result.statistics.draw_calls == 1 &&
            result.statistics.drawn_vertices == 12, "normal mesh did not draw/present/capture");
        CheckImage(*result.screenshot, layer, decode);
        Assets::WritePng(output / (std::string(name) + ".png"), 640, 480, 4, result.screenshot->pixels);
        ++captures;
        return result.screenshot->pixels;
    };
    const auto created = render("created-layer-1", 1, false);
    Require(render("repeated-layer-1", 1, false) == created, "unchanged accepted mesh changed pixels");
    source = Quads(2); renderer.UpdateMesh(mesh, source); source.vertices.clear();
    const auto updated = render("updated-layer-2", 2, false);
    Require(updated != created, "accepted update did not change pixels");
    Require(render("repeated-layer-2", 2, false) == updated, "updated mesh changed on repeated render");
    renderer.UpdateMesh(mesh, Quads(1));
    Require(render("restored-layer-1", 1, false) == created, "normal lower-layer update changed baseline pixels");
    draws[0].texture_array = srgb; render("rebound-srgb", 1, true);
    renderer.DestroyMesh(mesh); renderer.DestroyTextureArray(linear); renderer.DestroyTextureArray(srgb);
    renderer.Shutdown();
    std::cout << "normal_create_update_repeat_rebind=passed; saved_captures=" << captures
        << "; normal_shutdown=passed; boundary_regression_performance_tests=not_run\n";
}
}
int main(int argc, char** argv) {
    try {
        Require(argc == 4, "usage: probe d3d12|vulkan shader-root new-output-directory");
        const std::string selected = argv[1];
        Require(selected == "d3d12" || selected == "vulkan", "unknown backend");
        const auto output = std::filesystem::absolute(argv[3]);
        Require(!std::filesystem::exists(output), "output must be new"); std::filesystem::create_directories(output);
        Run(selected == "d3d12" ? Backend::D3D12 : Backend::Vulkan, std::filesystem::absolute(argv[2]), output);
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
