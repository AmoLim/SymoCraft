#include "backend/backend_factory.h"

#include "symocraft/renderer/error.h"
#include "validation.h"
#ifdef SYMOCRAFT_HAS_D3D12_BACKEND
#include "backend/d3d12/d3d12_backend.h"
#endif
#ifdef SYMOCRAFT_HAS_VULKAN_BACKEND
#include "backend/vulkan/vulkan_backend.h"
#endif

namespace SymoCraft::Rendering::Detail {

const char* BackendName(Backend backend) noexcept {
    switch (backend) {
    case Backend::OpenGL: return "OpenGL";
    case Backend::D3D12: return "D3D12";
    case Backend::Vulkan: return "Vulkan";
    }
    return "Unknown";
}

std::unique_ptr<RenderBackend> CreateBackend(Window& window, const RendererConfig& config) {
    ValidateConfig(config);
#ifdef SYMOCRAFT_HAS_D3D12_BACKEND
    if (config.backend == Backend::D3D12) return CreateD3D12Backend(window, config);
#else
    (void)window;
#endif
#ifdef SYMOCRAFT_HAS_VULKAN_BACKEND
    if (config.backend == Backend::Vulkan) return CreateVulkanBackend(window, config);
#endif
    throw RenderError(RenderErrorCategory::Unsupported, BackendName(config.backend), "CreateBackend",
                      "requested backend is not implemented in the production Renderer facade");
}

} // namespace SymoCraft::Rendering::Detail
