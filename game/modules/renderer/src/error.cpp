#include "symocraft/renderer/error.h"

#include <utility>

namespace SymoCraft::Rendering {

RenderError::RenderError(RenderErrorCategory category, std::string backend_name,
                         std::string operation, std::string message,
                         std::string native_code, std::string diagnostic)
    : std::runtime_error(std::move(message)), category_(category),
      backend_name_(std::move(backend_name)), operation_(std::move(operation)),
      native_code_(std::move(native_code)), diagnostic_(std::move(diagnostic)) {}

RenderErrorCategory RenderError::Category() const noexcept { return category_; }

const std::string& RenderError::BackendName() const noexcept { return backend_name_; }

const std::string& RenderError::Operation() const noexcept { return operation_; }

const std::string& RenderError::NativeCode() const noexcept { return native_code_; }

const std::string& RenderError::Diagnostic() const noexcept { return diagnostic_; }

} // namespace SymoCraft::Rendering
