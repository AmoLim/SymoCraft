#include "symocraft/renderer/renderer.h"

#include "backend/backend_factory.h"
#include "renderer_impl.h"
#include "symocraft/renderer/error.h"

namespace SymoCraft::Rendering {

class Renderer::Impl final {
public:
    Impl(Window& window, const RendererConfig& config)
        : state(config, Detail::CreateBackend(window, config)) {}

    Detail::RendererState state;
};

Renderer::Renderer(Window& window, const RendererConfig& config) {
    try {
        impl_ = std::make_unique<Impl>(window, config);
    } catch (const Detail::ResourceRegistryError& error) {
        throw RenderError(error.category, Detail::BackendName(config.backend), "Initialize", error.what());
    } catch (const RenderError&) {
        throw;
    } catch (const std::bad_alloc&) {
        throw RenderError(RenderErrorCategory::AllocationFailure, Detail::BackendName(config.backend),
                          "Initialize", "CPU allocation failed");
    } catch (const std::invalid_argument& error) {
        throw RenderError(RenderErrorCategory::InvalidInput, Detail::BackendName(config.backend),
                          "Initialize", error.what());
    } catch (const std::exception& error) {
        throw RenderError(RenderErrorCategory::InitializationFailure, Detail::BackendName(config.backend),
                          "Initialize", error.what());
    } catch (...) {
        throw RenderError(RenderErrorCategory::InitializationFailure, Detail::BackendName(config.backend),
                          "Initialize", "unknown backend initialization failure");
    }
}

Renderer::~Renderer() noexcept = default;

MeshHandle Renderer::CreateMesh(const MeshData& data) { return impl_->state.CreateMesh(data); }
void Renderer::UpdateMesh(MeshHandle handle, const MeshData& data) { impl_->state.UpdateMesh(handle, data); }
void Renderer::DestroyMesh(MeshHandle handle) { impl_->state.DestroyMesh(handle); }
TextureArrayHandle Renderer::CreateTextureArray(const TextureArrayData& data) {
    return impl_->state.CreateTextureArray(data);
}
void Renderer::DestroyTextureArray(TextureArrayHandle handle) { impl_->state.DestroyTextureArray(handle); }
FrameResult Renderer::Render(const FrameInput& frame) { return impl_->state.Render(frame); }
void Renderer::Resize(Extent2D extent) { impl_->state.Resize(extent); }
void Renderer::SetVSync(bool enabled) { impl_->state.SetVSync(enabled); }
RendererInfo Renderer::GetInfo() const { return impl_->state.GetInfo(); }
std::vector<GpuFrameSample> Renderer::PollGpuSamples() { return impl_->state.PollGpuSamples(); }
void Renderer::Shutdown() { impl_->state.Shutdown(); }

} // namespace SymoCraft::Rendering
