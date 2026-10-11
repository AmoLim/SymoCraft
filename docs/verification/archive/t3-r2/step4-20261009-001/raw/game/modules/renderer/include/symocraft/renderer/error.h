#pragma once

#include <stdexcept>
#include <string>

namespace SymoCraft::Rendering {

enum class RenderErrorCategory {
    InvalidInput,
    InvalidHandle,
    AllocationFailure,
    Unsupported,
    AssetUnavailable,
    InitializationFailure,
    DeviceLost,
    SubmissionFailure,
    Timeout,
    InvalidState,
    WrongThread,
    ReentrantCall
};

class RenderError final : public std::runtime_error {
public:
    RenderError(RenderErrorCategory category, std::string backend_name,
                std::string operation, std::string message,
                std::string native_code = {}, std::string diagnostic = {});

    RenderErrorCategory Category() const noexcept;
    const std::string& BackendName() const noexcept;
    const std::string& Operation() const noexcept;
    const std::string& NativeCode() const noexcept;
    const std::string& Diagnostic() const noexcept;

private:
    RenderErrorCategory category_;
    std::string backend_name_;
    std::string operation_;
    std::string native_code_;
    std::string diagnostic_;
};

} // namespace SymoCraft::Rendering
