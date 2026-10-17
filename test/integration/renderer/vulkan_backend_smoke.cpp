#include <symocraft/renderer/renderer.h>
#include <symocraft/platform/window.h>
#include <symocraft/assets/image.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
MeshData Quads() {
    MeshData data;
    for (int layer = 0; layer < 2; ++layer) {
        const int x = layer ? 0 : -2;
        const glm::ivec3 positions[]{{x,-1,0},{x+2,-1,0},{x+2,1,0},{x,-1,0},{x+2,1,0},{x,1,0}};
        const glm::vec2 uvs[]{{0,1},{1,1},{1,0},{0,1},{1,0},{0,0}};
        for (size_t i = 0; i < 6; ++i) data.vertices.push_back({positions[i],{uvs[i],static_cast<float>(layer)},0});
    }
    return data;
}
TextureArrayData Atlas(ColorSpace space) {
    TextureArrayData data; data.width = data.height = 2; data.layers = 2; data.color_space = space;
    data.pixels = {230,20,40,255, 20,220,50,255, 20,40,230,255, 230,200,20,255,
                   128,128,128,255, 128,128,128,255, 128,128,128,255, 128,128,128,255};
    return data;
}
std::array<uint8_t,3> Pixel(const Screenshot& image, uint32_t x, uint32_t y) {
    const auto i = (static_cast<size_t>(y)*image.extent.width+x)*4;
    return {image.pixels.at(i),image.pixels.at(i+1),image.pixels.at(i+2)};
}
void CheckImage(const Screenshot& image, bool srgb) {
    // Fixed camera projects both quads across the middle, with four asymmetric UV corners on the left.
    std::array<std::array<uint8_t,3>,5> expected{{{230,20,40},{20,220,50},{20,40,230},{230,200,20},{128,128,128}}};
    if (srgb) for (auto& color : expected) for (auto& channel : color) {
        const double c = channel / 255.0;
        channel = static_cast<uint8_t>(std::lround((c <= .04045 ? c/12.92 : std::pow((c+.055)/1.055,2.4))*255));
    }
    std::array<size_t,5> found{};
    for (uint32_t y = 0; y < image.extent.height; ++y) for (uint32_t x = 0; x < image.extent.width; ++x) {
        const auto pixel = Pixel(image,x,y);
        for (size_t c = 0; c < expected.size(); ++c) {
            bool match = true;
            for (size_t channel = 0; channel < 3; ++channel) if (std::abs(int(pixel[channel])-int(expected[c][channel])) > 1) match = false;
            if (match) ++found[c];
        }
    }
    for (auto count : found) Require(count > 100, "expected texture color/layer not drawn");
    const auto top = Pixel(image,image.extent.width*3/10,image.extent.height*2/5);
    const auto bottom = Pixel(image,image.extent.width*3/10,image.extent.height*3/5);
    Require(top[0] > top[1] && top[0] > top[2], "top-left UV is not red");
    Require(bottom[2] > bottom[0] && bottom[2] > bottom[1], "bottom-left UV is not blue");
}
}

int main(int argc, char** argv) {
    try {
        Require(argc == 4, "usage: smoke shader-root new-output-directory samples(1/4)");
        const std::filesystem::path output = std::filesystem::absolute(argv[2]);
        Require(!std::filesystem::exists(output), "output directory must be new"); std::filesystem::create_directories(output);
        Window::Init();
        auto window = std::unique_ptr<Window>(Window::Create("Vulkan production v1 smoke",640,480,false,true,WindowMode::Vulkan));
        {
            RendererConfig config; config.backend = Backend::Vulkan; config.shader_asset_root = std::filesystem::absolute(argv[1]);
            config.enable_validation = true; config.vsync = false; config.requested_samples = static_cast<uint32_t>(std::stoul(argv[3]));
            Renderer renderer(*window,config);
            const auto info = renderer.GetInfo();
            Require(info.backend == Backend::Vulkan && info.device_name.find("RTX 5070 Ti") != std::string::npos, "wrong actual GPU/backend");
            Require(info.actual_samples == config.requested_samples && info.validation_enabled && info.actual_present_mode == PresentMode::Immediate, "actual configuration differs");
            std::cout << "backend=Vulkan; device=" << info.device_name << "; identity=" << info.device_identity << "; driver=" << info.driver_version
                      << "; api=" << info.api_version << "; samples=" << info.actual_samples << '\n';
            auto mesh_data = Quads(); auto mesh = renderer.CreateMesh(mesh_data); mesh_data.vertices.clear();
            auto texture_data = Atlas(ColorSpace::Linear); const auto linear = renderer.CreateTextureArray(texture_data);
            texture_data.pixels.assign(texture_data.pixels.size(),0);
            const auto srgb = renderer.CreateTextureArray(Atlas(ColorSpace::SRGB));
            std::array<MeshDraw,1> draws{{{mesh,linear,glm::mat4(1)}}};
            FrameInput frame; frame.camera.position = {0,0,4}; frame.draws = draws; uint64_t id = 0;
            std::set<uint64_t> submitted, sampled;
            const auto render = [&](bool capture) {
                window->PollInt(); frame.frame_id = id++; frame.request_screenshot = capture;
                auto result = renderer.Render(frame); Require(result.status == FrameStatus::Presented, "frame did not present");
                submitted.insert(result.frame_id); return result;
            };
            auto image = render(true); Require(image.screenshot.has_value(), "capture missing"); CheckImage(*image.screenshot,false);
            Assets::WritePng(output/"linear.png",640,480,4,image.screenshot->pixels);
            draws[0].texture_array = srgb; image = render(true); CheckImage(*image.screenshot,true);
            Assets::WritePng(output/"srgb.png",640,480,4,image.screenshot->pixels);
            frame.selection = SelectionBox{{-1,-1,-.1f},{1,1,.1f}};
            image = render(true); Require(image.statistics.draw_calls == 2, "selection not submitted");
            Assets::WritePng(output/"selection.png",640,480,4,image.screenshot->pixels); frame.selection.reset();
            for (int i=0;i<6;++i) render(false);
            renderer.UpdateMesh(mesh,{}); image = render(true);
            Require(image.statistics.draw_calls == 0, "empty mesh retained geometry");
            for (size_t p=4;p<image.screenshot->pixels.size();p+=4)
                Require(image.screenshot->pixels[p] == image.screenshot->pixels[0], "empty mesh screenshot not cleared");
            renderer.UpdateMesh(mesh,Quads()); image = render(true); CheckImage(*image.screenshot,true);
            render(false); renderer.DestroyMesh(mesh);
            bool rejected = false; try { render(false); } catch (const RenderError& error) { rejected = error.Category() == RenderErrorCategory::InvalidHandle; }
            Require(rejected,"destroyed handle was accepted");
            mesh = renderer.CreateMesh(Quads()); draws[0].mesh = mesh;
            window->SetSize(800,600); window->PollInt(); renderer.Resize({static_cast<uint32_t>(window->width),static_cast<uint32_t>(window->height)});
            image = render(true); CheckImage(*image.screenshot,true);
            Assets::WritePng(output/"resize.png",static_cast<int>(image.screenshot->extent.width),static_cast<int>(image.screenshot->extent.height),4,image.screenshot->pixels);
            renderer.Resize({0,0}); frame.frame_id = id++; frame.request_screenshot = false;
            auto skipped = renderer.Render(frame); Require(skipped.skip_reason == FrameSkipReason::ZeroExtent,"zero extent not suspended");
            renderer.Resize({static_cast<uint32_t>(window->width),static_cast<uint32_t>(window->height)});
            renderer.SetVSync(true); Require(renderer.GetInfo().actual_present_mode == PresentMode::Fifo,"FIFO not reported"); render(true);
            renderer.SetVSync(false); for (int i=0;i<8;++i) render(false);
            // One capture retires the graphics queue without adding a wait to PollGpuSamples.
            render(true);
            for (auto sample : renderer.PollGpuSamples()) {
                Require(submitted.contains(sample.frame_id) && sampled.insert(sample.frame_id).second,"query ID missing/duplicate");
                Require(sample.valid && sample.gpu_draw_time && sample.gpu_draw_time->count() >= 0,"missing native query");
            }
            Require(sampled.size() == submitted.size(),"submitted/query frame IDs differ");
            renderer.DestroyMesh(mesh); renderer.DestroyTextureArray(linear); renderer.DestroyTextureArray(srgb);
            renderer.Shutdown(); renderer.Shutdown();
            std::cout << "saved_captures=4; mesh_update_empty_destroy=passed; resize_zero_restore=passed; vsync=passed; gpu_samples=" << sampled.size() << "; shutdown=passed\n";
        }
        window->Destroy(); window.reset(); Window::Free(); return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
