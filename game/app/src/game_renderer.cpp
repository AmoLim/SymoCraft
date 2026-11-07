#include "game_renderer.h"
#include "cleanup_sequence.h"
#include "render_conversion.h"
#include "startup_options.h"
#include "world_render_cache.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/error.h>
#ifdef SYMOCRAFT_APP_OPENGL
#include <symocraft/renderer/legacy_renderer.h>
#endif
#include <symocraft/renderer/renderer.h>
#include <symocraft/simulation/session.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <symocraft/world/world.h>
#include <symocraft/profiling/profiling.h>
#include <array>
#include <algorithm>
#include <iostream>
#include <exception>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace SymoCraft::Application::Detail {
using AppRendering::ToRendererAtlas;
using AppRendering::ToRendererCamera;
using AppRendering::WorldRenderCache;
namespace {
const char* PresentName(Rendering::PresentMode mode) {
    switch (mode) {
    case Rendering::PresentMode::Immediate: return "Immediate";
    case Rendering::PresentMode::Fifo: return "Fifo";
    case Rendering::PresentMode::Mailbox: return "Mailbox";
    default: return "Unknown";
    }
}
struct ModernMeshPort {
    using MeshHandle = Rendering::MeshHandle;
    Rendering::Renderer* renderer{};
    MeshHandle CreateMesh(const MeshData& data) { return renderer->CreateMesh(data); }
    void UpdateMesh(MeshHandle handle, const MeshData& data) { renderer->UpdateMesh(handle, data); }
    void DestroyMesh(MeshHandle handle) { renderer->DestroyMesh(handle); }
};
struct StopOnFailure {
    bool& allowed;
    int exceptions = std::uncaught_exceptions();
    ~StopOnFailure() { if (std::uncaught_exceptions() > exceptions) allowed = false; }
};
}

WindowMode SelectWindowMode(const StartupOptions& options) {
#ifdef SYMOCRAFT_APP_OPENGL
    if (options.renderer == "opengl") return WindowMode::OpenGL;
#endif
#ifdef SYMOCRAFT_APP_D3D12
    if (options.renderer == "d3d12") return WindowMode::Native;
#endif
#ifdef SYMOCRAFT_APP_VULKAN
    if (options.renderer == "vulkan") return WindowMode::Vulkan;
#endif
    throw Rendering::RenderError(Rendering::RenderErrorCategory::Unsupported,
        options.renderer == "opengl" ? "OpenGL" : options.renderer == "vulkan" ? "Vulkan" : "D3D12", "SelectBackend",
        "requested renderer is not implemented or compiled into this game; no fallback");
}

bool CheckRendererAssets(const StartupOptions& options, std::ostream& errors) {
    if (!options.world_summary) SelectWindowMode(options);
#ifdef SYMOCRAFT_APP_OPENGL
    if (!options.world_summary && options.renderer == "opengl") return Assets::CheckRequiredAssets(errors);
#endif
    std::vector<std::string> files{
        "configs/blockFormats.yaml", "textures/texture_atlas.png",
        "shaders/d3d12/blocks.vs.dxil", "shaders/d3d12/blocks.ps.dxil",
        "shaders/d3d12/selection.vs.dxil", "shaders/d3d12/selection.ps.dxil"};
    const bool vulkan = options.renderer == "vulkan";
    const char* backend = options.world_summary ? "World summary" : vulkan ? "Vulkan" : "D3D12";
    if (vulkan) {
        files.resize(2);
        for (const auto* shader : {"blocks.vs.spv", "blocks.ps.spv", "selection.vs.spv", "selection.ps.spv"})
            files.emplace_back(std::string("shaders/vulkan/") + shader);
    }
    bool valid = true;
    for (const auto& file : files) {
        if (options.world_summary && std::string_view(file).starts_with("shaders/")) continue;
        try {
            const auto path = Assets::Resolve(file);
            std::error_code error;
            if (!std::filesystem::is_regular_file(path, error) ||
                std::filesystem::file_size(path, error) == 0 || error) {
                errors << backend
                       << " missing, empty or inaccessible asset: " << path.string() << '\n';
                valid = false;
            }
        } catch (const std::exception& error) {
            errors << backend << " asset " << file << ": " << error.what() << '\n';
            valid = false;
        }
    }
    return valid;
}

struct GameRenderer::Impl {
    Window& window;
    std::unique_ptr<Rendering::Renderer> modern;
    ModernMeshPort mesh_port;
    std::unique_ptr<WorldRenderCache<ModernMeshPort>> cache;
    Rendering::TextureArrayHandle atlas;
    std::vector<Rendering::MeshDraw> draws;
    Rendering::Extent2D extent;
    bool extent_pending{};
#ifdef SYMOCRAFT_APP_OPENGL
    bool legacy_started{};
#endif
    bool stopped{};
    Rendering::Backend backend = Rendering::Backend::OpenGL;
    bool operations_allowed = true;
    ShutdownReport shutdown;

    explicit Impl(Window& value) : window(value) {}
};

void AttachRendererContext(Window& window, const StartupOptions& options) {
#ifdef SYMOCRAFT_APP_OPENGL
    if (options.renderer == "opengl") Renderer::AttachContext(window);
#else
    (void)window;
    (void)options;
#endif
}

GameRenderer::GameRenderer(Window& window, const StartupOptions& options)
    : impl_(std::make_unique<Impl>(window)) {
    SelectWindowMode(options);
    auto& state = *impl_;
    if (options.renderer == "d3d12" || options.renderer == "vulkan") {
        Rendering::RendererConfig config;
        config.backend = options.renderer == "vulkan" ? Rendering::Backend::Vulkan : Rendering::Backend::D3D12;
        config.vsync = (!options.profile_scenario.empty()) ? options.vsync : true;
        config.requested_samples = options.requested_samples;
        config.enable_validation = options.enable_validation;
        config.shader_asset_root = Assets::Resolve(options.renderer == "vulkan" ? "shaders/vulkan" : "shaders/d3d12");
        state.modern = std::make_unique<Rendering::Renderer>(window, config);
        state.mesh_port.renderer = state.modern.get();
        state.cache = std::make_unique<WorldRenderCache<ModernMeshPort>>(state.mesh_port);
        state.extent = state.modern->GetInfo().framebuffer_extent;
        const auto info = state.modern->GetInfo();
        state.backend = info.backend;
        std::cout << "[renderer] backend=" << (info.backend == Rendering::Backend::Vulkan ? "Vulkan" : "D3D12") << "; device=" << info.device_name
                  << "; identity=" << info.device_identity << "; driver=" << info.driver_version
                  << "; samples=" << info.actual_samples << "; present=" << PresentName(info.actual_present_mode)
                  << "; validation=" << info.validation_enabled << std::endl;
    }
#ifdef SYMOCRAFT_APP_OPENGL
    else {
        try {
            state.legacy_started = true;
            Renderer::Init();
            if (!options.profile_scenario.empty()) Renderer::SetVsync(options.vsync);
        } catch (...) {
            const auto failure = std::current_exception();

            if (std::exchange(state.legacy_started, false)) {
                try { Renderer::Free(); } catch (...) {}
            }
            std::rethrow_exception(failure);
        }
    }
#endif
}
GameRenderer::~GameRenderer() noexcept {
    if (impl_ && impl_->stopped) return;
    try { Shutdown(); }
    catch (const std::exception& error) { std::fprintf(stderr, "[renderer] adapter destructor: %s\n", error.what()); }
    catch (...) { std::fprintf(stderr, "[renderer] adapter destructor: unknown failure\n"); }
}
bool GameRenderer::Modern() const noexcept { return impl_->modern != nullptr; }
Rendering::Backend GameRenderer::Backend() const noexcept { return impl_->backend; }
std::string GameRenderer::ProfilingIdentity() const {
    std::string text = "Symocraft";
#ifdef NDEBUG
    text += "; build=Release";
#else
    text += "; build=Debug";
#endif
    if (!impl_->modern) return text + "; backend=OpenGL";
    const auto info = impl_->modern->GetInfo();
    return text + "; backend=" + (info.backend == Rendering::Backend::Vulkan ? "Vulkan" : "D3D12") +
        "; device=" + info.device_name + "; driver=" + info.driver_version + "; api=" + info.api_version +
        "; extent=" + std::to_string(info.framebuffer_extent.width) + "x" + std::to_string(info.framebuffer_extent.height) +
        "; samples=" + std::to_string(info.actual_samples) + "; present=" + PresentName(info.actual_present_mode) +
        "; validation=" + (info.validation_enabled ? "1" : "0");
}
std::uint16_t GameRenderer::LoadAtlas(const std::filesystem::path& path) {
    SYMOCRAFT_PROFILE_ZONE("renderer.atlas_upload");
    auto& state = *impl_;
    StopOnFailure guard{state.operations_allowed};
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) return Renderer::LoadTextureAtlas(path);
#endif
    if (!state.atlas.IsNull()) throw std::logic_error("game atlas is already loaded");
    const auto data = ToRendererAtlas(Assets::DecodeImage(path, false));
    state.atlas = state.modern->CreateTextureArray(data);
    return static_cast<std::uint16_t>(data.layers);
}
MeshUpload GameRenderer::Synchronize(const World::VoxelWorld& world) {
    SYMOCRAFT_PROFILE_ZONE("renderer.cache_upload_cpu");
    auto& state = *impl_;
    StopOnFailure guard{state.operations_allowed};
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        world.VisitMeshes([](const World::WorldMeshRecord& record) { Renderer::AppendMesh(record.vertices); });
        return {};
    }
#endif
    const auto upload = state.cache->Synchronize(world);
    state.draws.clear();
    state.cache->AppendDraws(state.atlas, state.draws);
    return {upload.upload_bytes, upload.created_meshes + upload.updated_meshes};
}
void GameRenderer::ReleaseWorld(std::uint64_t identity) {
    if (Modern()) {
        impl_->draws.clear();
        impl_->cache->ReleaseWorld(identity);
    }
}
void GameRenderer::Resize() {
    if (!Modern()) return;
    auto& state = *impl_;
    StopOnFailure guard{state.operations_allowed};
    const Rendering::Extent2D extent{
        state.window.Minimized() ? 0u : static_cast<std::uint32_t>(std::max(state.window.Width(), 0)),
        state.window.Minimized() ? 0u : static_cast<std::uint32_t>(std::max(state.window.Height(), 0))};
    if (state.extent_pending || extent.width != state.extent.width || extent.height != state.extent.height) {
        try { state.modern->Resize(extent); }
        catch (const Rendering::RenderError& error) {
            if (error.BackendName() != "Vulkan" || error.Operation() != "Resize" ||
                error.Category() != Rendering::RenderErrorCategory::InvalidState || error.Diagnostic() != "surface_extent_pending") throw;
            state.extent_pending = true;
            state.window.WaitEvents(.01);
            return;
        }
        state.extent = extent;
        state.extent_pending = false;
    }
}
GameFrame GameRenderer::Render(std::uint64_t frame_id, const Simulation::PlayerView& camera,
    std::optional<glm::vec3> selection, bool screenshot) {
    SYMOCRAFT_PROFILE_ZONE("renderer.frame_cpu");
    auto& state = *impl_;
    StopOnFailure guard{state.operations_allowed};
    GameFrame output;
    if (Modern()) {
        Resize();
        if (state.extent_pending) {
            output.result.frame_id = frame_id;
            output.result.skip_reason = Rendering::FrameSkipReason::Occluded;

            return output;
        }
        Rendering::FrameInput frame;
        frame.frame_id = frame_id;
        frame.camera = ToRendererCamera(camera);
        frame.draws = state.draws;
        if (selection) frame.selection = Rendering::SelectionBox{glm::floor(*selection), glm::floor(*selection) + glm::vec3(1.0f)};
        frame.request_screenshot = screenshot;
        output.result = state.modern->Render(frame);
        if (output.result.skip_reason == Rendering::FrameSkipReason::ZeroExtent) state.extent = {};

        return output;
    }
#ifdef SYMOCRAFT_APP_OPENGL
    Renderer::SetViewport(state.window.Width(), state.window.Height());
    Renderer::SetSelection(selection);
    Renderer::Render({glm::perspectiveRH_NO(glm::radians(camera.fov), state.window.GetAspectRatio(), camera.near_plane, camera.far_plane),
                      glm::lookAtRH(camera.position, camera.position + camera.forward, camera.up)});

    Renderer::Present(state.window);

    output.result.frame_id = frame_id;
    output.result.status = Rendering::FrameStatus::Presented;
    return output;
#else
    throw std::logic_error("GameRenderer has no accepted modern backend");
#endif
}
void GameRenderer::Capture(std::uint64_t frame_id, const Simulation::PlayerView& camera,
    std::optional<glm::vec3> selection, const std::filesystem::path& path) {
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        Renderer::SetSelection(selection);
        Renderer::Render({glm::perspectiveRH_NO(glm::radians(camera.fov), impl_->window.GetAspectRatio(), camera.near_plane, camera.far_plane),
                          glm::lookAtRH(camera.position, camera.position + camera.forward, camera.up)});
        Renderer::CaptureFramebuffer(impl_->window.Width(), impl_->window.Height(), path);
        return;
    }
#endif
    const auto frame = Render(frame_id, camera, selection, true);
    if (!frame.result.screenshot) throw std::runtime_error("modern renderer diagnostic frame did not produce a screenshot");
    const auto& image = *frame.result.screenshot;
    constexpr auto encoder_limit = static_cast<std::uint32_t>(std::numeric_limits<int>::max());
    if (image.extent.width > encoder_limit || image.extent.height > encoder_limit)
        throw std::length_error("screenshot exceeds PNG encoder dimensions");
    Assets::WritePng(path, static_cast<int>(image.extent.width), static_cast<int>(image.extent.height), 4, image.pixels);
}
ShutdownReport GameRenderer::Shutdown() {
    if (!impl_) return {};
    auto& state = *impl_;
    if (state.stopped) {
        if (!state.shutdown.Succeeded()) throw ShutdownError(state.shutdown);
        return state.shutdown;
    }
    state.stopped = true;
    auto& report = state.shutdown;
    if (state.modern) {
        if (state.operations_allowed) {
            report.Attempt(ShutdownStep::MeshBindings, state.cache != nullptr, [&] { state.cache->ReleaseAll(); });
            report.Attempt(ShutdownStep::Texture, !state.atlas.IsNull(), [&] { state.modern->DestroyTextureArray(state.atlas); });
        } else {
            if (state.cache) report.Unavailable(ShutdownStep::MeshBindings);
            if (!state.atlas.IsNull()) report.Unavailable(ShutdownStep::Texture);
        }
        report.Attempt(ShutdownStep::Renderer, true, [&] { state.modern->Shutdown(); });
        if (report.items[static_cast<std::size_t>(ShutdownStep::Renderer)].fact == ReleaseFact::Released) {
            for (auto step : {ShutdownStep::MeshBindings, ShutdownStep::Texture}) {
                auto& item = report.items[static_cast<std::size_t>(step)];
                if (item.fact == ReleaseFact::Unavailable) item.fact = ReleaseFact::Released;
            }
        }
        state.draws.clear();
        state.cache.reset();
        state.modern.reset();
    }
#ifdef SYMOCRAFT_APP_OPENGL
    else {

        report.Attempt(ShutdownStep::Renderer, std::exchange(state.legacy_started, false), [] { Renderer::Free(); });
    }
#endif
    if (!report.Succeeded()) throw ShutdownError(report);
    return report;
}
}
