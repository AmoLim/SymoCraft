#pragma once

#include "render_backend.h"

namespace SymoCraft { class Window; }

namespace SymoCraft::Rendering::Detail {

std::unique_ptr<RenderBackend> CreateD3D12Backend(Window& window, const RendererConfig& config);

} // namespace SymoCraft::Rendering::Detail
