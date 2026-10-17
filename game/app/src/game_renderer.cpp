#include "game_renderer.h"
#include <symocraft/telemetry/document_io.h>
#include "cleanup_sequence.h"
#include "render_conversion.h"
#include "startup_options.h"
#include "world_render_cache.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/platform/window.h>
#include <symocraft/renderer/error.h>
#ifdef SYMOCRAFT_APP_OPENGL
#include <symocraft/renderer/gpu_timer.h>
#include <symocraft/renderer/legacy_renderer.h>
#endif
#include <symocraft/renderer/renderer.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/telemetry/performance.h>
#include <symocraft/world/world.h>
#include <array>
#include <algorithm>
#include <iostream>
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
double Milliseconds(std::chrono::nanoseconds duration) {
    return std::chrono::duration<double, std::milli>(duration).count();
}
const char* PresentName(Rendering::PresentMode mode) {
    switch (mode) {
    case Rendering::PresentMode::Immediate: return "Immediate";
    case Rendering::PresentMode::Fifo: return "Fifo";
    case Rendering::PresentMode::Mailbox: return "Mailbox";
    default: return "Unknown";
    }
}
const char* MissingName(Rendering::GpuSampleMissingReason reason) {
    switch (reason) {
    case Rendering::GpuSampleMissingReason::Unsupported: return "unsupported";
    case Rendering::GpuSampleMissingReason::DeviceFailure: return "device-failure";
    case Rendering::GpuSampleMissingReason::Discarded: return "discarded";
    default: return "invalid-backend-sample";
    }
}
struct ModernMeshPort {
    using MeshHandle = Rendering::MeshHandle;
    Rendering::Renderer* renderer{};
    MeshHandle CreateMesh(const MeshData& data) { return renderer->CreateMesh(data); }
    void UpdateMesh(MeshHandle handle, const MeshData& data) { renderer->UpdateMesh(handle, data); }
    void DestroyMesh(MeshHandle handle) { renderer->DestroyMesh(handle); }
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
    MeshUpload atlas_upload;
    std::uint32_t requested_samples{};
    std::vector<Rendering::MeshDraw> draws;
    Rendering::Extent2D extent;
    bool extent_pending{};
#ifdef SYMOCRAFT_APP_OPENGL
    std::unique_ptr<GpuTimer> legacy_timer;
    bool legacy_started{};
#endif
    bool stopped{};

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

GameRenderer::GameRenderer(Window& window, const StartupOptions& options, bool collect_samples)
    : impl_(std::make_unique<Impl>(window)) {
    SelectWindowMode(options);
    auto& state = *impl_;
    state.requested_samples = options.requested_samples;
    if (options.renderer == "d3d12" || options.renderer == "vulkan") {
        Rendering::RendererConfig config;
        config.backend = options.renderer == "vulkan" ? Rendering::Backend::Vulkan : Rendering::Backend::D3D12;
        config.vsync = collect_samples ? options.vsync : true;
        config.requested_samples = options.requested_samples;
        config.enable_validation = options.enable_validation;
        config.shader_asset_root = Assets::Resolve(options.renderer == "vulkan" ? "shaders/vulkan" : "shaders/d3d12");
        state.modern = std::make_unique<Rendering::Renderer>(window, config);
        state.mesh_port.renderer = state.modern.get();
        state.cache = std::make_unique<WorldRenderCache<ModernMeshPort>>(state.mesh_port);
        state.extent = state.modern->GetInfo().framebuffer_extent;
        const auto info = state.modern->GetInfo();
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
            if (collect_samples) {
                Renderer::SetVsync(options.vsync);
                state.legacy_timer = std::make_unique<GpuTimer>();
            }
        } catch (...) {
            const auto failure = std::current_exception();
            state.legacy_timer.reset();
            if (std::exchange(state.legacy_started, false)) {
                try { Renderer::Free(); } catch (...) {}
            }
            std::rethrow_exception(failure);
        }
    }
#endif
}
GameRenderer::~GameRenderer() noexcept {
    try { Shutdown(); }
    catch (const std::exception& error) { std::cerr << "[renderer] adapter destructor: " << error.what() << '\n'; }
    catch (...) { std::cerr << "[renderer] adapter destructor: unknown failure\n"; }
}
bool GameRenderer::Modern() const noexcept { return impl_->modern != nullptr; }
std::uint16_t GameRenderer::LoadAtlas(const std::filesystem::path& path) {
    auto& state = *impl_;
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) return Renderer::LoadTextureAtlas(path);
#endif
    if (!state.atlas.IsNull()) throw std::logic_error("game atlas is already loaded");
    const auto data = ToRendererAtlas(Assets::DecodeImage(path, false));
    const auto upload_start = Performance::Clock::now();
    state.atlas = state.modern->CreateTextureArray(data);
    state.atlas_upload = {static_cast<std::uint64_t>(data.pixels.size()), 1, Performance::Milliseconds(upload_start)};
    return static_cast<std::uint16_t>(data.layers);
}
MeshUpload GameRenderer::AtlasUpload() const noexcept { return impl_->atlas_upload; }
MeshUpload GameRenderer::Synchronize(const World::VoxelWorld& world) {
    auto& state = *impl_;
    const auto start = Performance::Clock::now();
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        world.VisitMeshes([](const World::WorldMeshRecord& record) { Renderer::AppendMesh(record.vertices); });
        return {};
    }
#endif
    const auto upload = state.cache->Synchronize(world);
    state.draws.clear();
    state.cache->AppendDraws(state.atlas, state.draws);
    return {upload.upload_bytes, upload.created_meshes + upload.updated_meshes,
            Performance::Milliseconds(start)};
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
    const Rendering::Extent2D extent{
        state.window.Minimized() ? 0u : static_cast<std::uint32_t>(std::max(state.window.width, 0)),
        state.window.Minimized() ? 0u : static_cast<std::uint32_t>(std::max(state.window.height, 0))};
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
void GameRenderer::DescribeSession(Performance::Session& session) const {
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        const auto& device = Renderer::Device();
        session.metadata["gl_vendor"] = device.vendor;
        session.metadata["gl_renderer"] = device.renderer;
        session.metadata["gl_version"] = device.version;
        session.metadata["msaa_samples"] = device.msaa_samples;
        session.metadata["allocated_vbo_bytes"] = Renderer::AllocatedBufferBytes();
        session.metadata["gpu_timer_supported"] = impl_->legacy_timer->Supported();
        session.metadata["nvx_memory_supported"] = device.nvx_memory_supported;
        return;
    }
#endif
    const auto info = impl_->modern->GetInfo();
    Performance::RendererInfo description;
    description.backend = info.backend == Rendering::Backend::Vulkan ? "Vulkan" : "D3D12";
    description.device_name = info.device_name;
    description.device_identity = info.device_identity;
    description.driver_version = info.driver_version;
    description.api_version = info.api_version;
    description.framebuffer_format = info.framebuffer_format;
    description.framebuffer_width = info.framebuffer_extent.width;
    description.framebuffer_height = info.framebuffer_extent.height;
    description.actual_samples = info.actual_samples;
    description.actual_present_mode = PresentName(info.actual_present_mode);
    description.validation_enabled = info.validation_enabled;
    description.texture_arrays = info.capabilities.texture_arrays;
    description.selection_boxes = info.capabilities.selection_boxes;
    description.screenshots = info.capabilities.screenshots;
    description.gpu_timestamps = info.capabilities.gpu_timestamps;
    description.gpu_time_scope = "native timestamps around mesh/selection draw, excluding clear, resolve, screenshot copy and Present";
    session.SetRendererInfo(description);
    session.metadata["requested_msaa_samples"] = impl_->requested_samples;
    session.metadata["shader_generation"] = Data::LoadYamlFile(Assets::Resolve(
        info.backend == Rendering::Backend::Vulkan ? "shaders/vulkan/build.json" : "shaders/d3d12/build.json"));
    session.metadata["renderer_resource_upload_scope"] = info.backend == Rendering::Backend::Vulkan
        ? "successful cache Create/Update bytes, counted once; Render upload bytes include local constants and selection staging"
        : "successful cache Create/Update bytes, counted once; Render upload bytes separately include selection staging";
    session.metadata["renderer_cpu_prepare_scope"] = "facade plus backend preparation, including frame-slot waits; excludes World cache upload and simulation";
}
void GameRenderer::SampleDeviceMemory(Performance::Memory& memory) const {
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        const auto device = Renderer::SampleDeviceMemory();
        memory.device_dedicated_kib = device.dedicated_kib;
        memory.device_available_kib = device.available_kib;
    }
#else
    (void)memory;
#endif
}
void GameRenderer::PollGpu(Performance::Session& session, std::optional<std::uint64_t> excluded_frame) {
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        for (const auto& sample : impl_->legacy_timer->Poll()) session.SetGpu(sample.frame, sample.milliseconds);
        return;
    }
#endif
    for (const auto& sample : impl_->modern->PollGpuSamples()) {
        if (excluded_frame == sample.frame_id) continue;
        if (sample.valid && sample.gpu_draw_time)
            session.SetGpuSample(sample.frame_id, Milliseconds(*sample.gpu_draw_time), {});
        else session.SetGpuSample(sample.frame_id, std::nullopt, MissingName(sample.missing_reason));
    }
}
GameFrame GameRenderer::Render(std::uint64_t frame_id, const Camera& camera,
    std::optional<glm::vec3> selection, Performance::Session* session, bool screenshot) {
    auto& state = *impl_;
    GameFrame output;
    if (Modern()) {
        const auto resize_start = Performance::Clock::now();
        Resize();
        if (state.extent_pending) {
            output.result.frame_id = frame_id;
            output.result.skip_reason = Rendering::FrameSkipReason::Occluded;
            output.result.statistics.cpu_prepare = std::chrono::duration_cast<std::chrono::nanoseconds>(Performance::Clock::now() - resize_start);
            output.render_cpu_ms = Milliseconds(output.result.statistics.cpu_prepare);
            return output;
        }
        // Ordinary gameplay has no collector, but must still drain the bounded result ring.
        if (!session && !screenshot) (void)state.modern->PollGpuSamples();
        Rendering::FrameInput frame;
        frame.frame_id = frame_id;
        frame.camera = ToRendererCamera(camera);
        frame.draws = state.draws;
        if (selection) frame.selection = Rendering::SelectionBox{glm::floor(*selection), glm::floor(*selection) + glm::vec3(1.0f)};
        frame.request_screenshot = screenshot;
        output.result = state.modern->Render(frame);
        if (output.result.skip_reason == Rendering::FrameSkipReason::ZeroExtent) state.extent = {};
        output.render_cpu_ms = Milliseconds(output.result.statistics.cpu_prepare) + Milliseconds(output.result.statistics.cpu_submit);
        output.present_ms = Milliseconds(output.result.statistics.present_wait);
        return output;
    }
#ifdef SYMOCRAFT_APP_OPENGL
    const auto start = Performance::Clock::now();
    Renderer::SetViewport(state.window.width, state.window.height);
    Renderer::SetSelection(selection);
    RenderStats stats;
    if (session) state.legacy_timer->BeginFrame(session->NextFrame());
    Renderer::Render({camera.GetCameraProjMat(state.window.GetAspectRatio()), camera.GetCameraViewMat()},
                     session ? &stats : nullptr, state.legacy_timer.get());
    if (state.legacy_timer) state.legacy_timer->EndFrame();
    output.render_cpu_ms = Performance::Milliseconds(start);
    output.upload_cpu_ms = stats.upload_cpu_ms;
    const auto present_start = Performance::Clock::now();
    Renderer::Present(state.window);
    output.present_ms = Performance::Milliseconds(present_start);
    output.result.frame_id = frame_id;
    output.result.status = Rendering::FrameStatus::Presented;
    output.result.statistics.draw_calls = stats.draw_calls;
    output.result.statistics.drawn_vertices = stats.vertices;
    output.result.statistics.upload_bytes = stats.upload_bytes;
    return output;
#else
    throw std::logic_error("GameRenderer has no accepted modern backend");
#endif
}
void GameRenderer::Capture(std::uint64_t frame_id, const Camera& camera,
    std::optional<glm::vec3> selection, const std::filesystem::path& path, Performance::Session& session) {
#ifdef SYMOCRAFT_APP_OPENGL
    if (!Modern()) {
        Renderer::SetSelection(selection);
        Renderer::Render({camera.GetCameraProjMat(impl_->window.GetAspectRatio()), camera.GetCameraViewMat()});
        Renderer::CaptureFramebuffer(impl_->window.width, impl_->window.height, path);
        return;
    }
#endif
    const auto frame = Render(frame_id, camera, selection, nullptr, true);
    if (!frame.result.screenshot) throw std::runtime_error("modern renderer diagnostic frame did not produce a screenshot");
    const auto& image = *frame.result.screenshot;
    constexpr auto encoder_limit = static_cast<std::uint32_t>(std::numeric_limits<int>::max());
    if (image.extent.width > encoder_limit || image.extent.height > encoder_limit)
        throw std::length_error("screenshot exceeds PNG encoder dimensions");
    Assets::WritePng(path, static_cast<int>(image.extent.width), static_cast<int>(image.extent.height), 4, image.pixels);
    // Screenshot waits are outside measurement and also permit previous timestamps to become ready.
    PollGpu(session, frame_id);
}
void GameRenderer::Shutdown() {
    if (!impl_ || impl_->stopped) return;
    auto& state = *impl_;
    state.stopped = true;
    CleanupSequence cleanup(std::cerr);
    if (state.modern) {
        cleanup.Run("world mesh cache", [&] { state.cache->ReleaseAll(); });
        cleanup.Run("texture array", [&] { if (!state.atlas.IsNull()) state.modern->DestroyTextureArray(state.atlas); });
        cleanup.Run("Renderer::Shutdown", [&] { state.modern->Shutdown(); });
        state.draws.clear();
        state.cache.reset();
        state.modern.reset();
    }
#ifdef SYMOCRAFT_APP_OPENGL
    else {
        cleanup.Run("legacy GPU timer", [&] { state.legacy_timer.reset(); });
        cleanup.RunOnce("Renderer::Free", state.legacy_started, [] { Renderer::Free(); });
    }
#endif
    cleanup.RethrowFailure();
}
}
