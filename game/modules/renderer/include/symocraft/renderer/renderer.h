#pragma once

#include "symocraft/renderer/error.h"
#include "symocraft/renderer/types.h"

#include <memory>
#include <vector>

namespace SymoCraft { class Window; }

namespace SymoCraft::Rendering {

class Renderer final {
public:
    Renderer(SymoCraft::Window& window, const RendererConfig& config);
    ~Renderer() noexcept;
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    MeshHandle CreateMesh(const MeshData& data);
    void UpdateMesh(MeshHandle handle, const MeshData& data);
    void DestroyMesh(MeshHandle handle);
    TextureArrayHandle CreateTextureArray(const TextureArrayData& data);
    void DestroyTextureArray(TextureArrayHandle handle);

    FrameResult Render(const FrameInput& frame);
    void Resize(Extent2D framebuffer_extent);
    void SetVSync(bool enabled);
    RendererInfo GetInfo() const;
    std::vector<GpuFrameSample> PollGpuSamples();
    void Shutdown();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace SymoCraft::Rendering
