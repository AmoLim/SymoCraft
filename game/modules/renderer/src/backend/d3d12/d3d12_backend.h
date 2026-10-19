#pragma once

#include "render_backend.h"

namespace SymoCraft { class Window; }

namespace SymoCraft::Rendering::Detail {

std::unique_ptr<RenderBackend> CreateD3D12Backend(Window& window, const RendererConfig& config);
struct D3D12DiagnosticAccess;
std::unique_ptr<RenderBackend> CreateD3D12DiagnosticBackend(Window& window, const RendererConfig& config,
    std::shared_ptr<D3D12DiagnosticAccess> diagnostics);

} // namespace SymoCraft::Rendering::Detail
