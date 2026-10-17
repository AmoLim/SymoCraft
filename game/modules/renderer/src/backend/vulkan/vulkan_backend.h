#pragma once
#include "render_backend.h"
#include <memory>

namespace SymoCraft { class Window; }

namespace SymoCraft::Rendering::Detail {
    std::unique_ptr<RenderBackend> CreateVulkanBackend(Window& window, const RendererConfig& config);
}
