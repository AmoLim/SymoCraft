#pragma once

#include "backend/render_backend.h"

namespace SymoCraft { class Window; }

namespace SymoCraft::Rendering::Detail {

const char* BackendName(Backend backend) noexcept;
std::unique_ptr<RenderBackend> CreateBackend(Window& window, const RendererConfig& config);

} // namespace SymoCraft::Rendering::Detail
