#pragma once

#include "candidate_contract.h"

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace SymoCraft { class Window; }

namespace SymoCraft::Experimental::D3D12R1::Candidate {

// Declaration-only proposal. No backend or production Renderer is implemented here.
enum class Backend { OpenGL, D3D12, Vulkan };

struct RendererConfig {
    Backend backend = Backend::D3D12;
    bool vsync = true;
    std::uint32_t requested_samples = 1;
    bool enable_validation = true;
    std::filesystem::path shader_asset_root;
    std::chrono::milliseconds wait_timeout{2500};
};

struct MeshDraw {
    MeshHandle mesh;
    TextureArrayHandle texture_array;
    // T1 vertices already use world-space positions; chunk draws start with identity.
    glm::mat4 object_transform{1.0f};
};

// Candidate world-space AABB; selection padding and style remain freeze decisions.
struct SelectionBox {
    glm::vec3 minimum{0.0f};
    glm::vec3 maximum{1.0f};
};

struct FrameInput {
    std::uint64_t frame_id = 0;
    CameraParameters camera;
    std::span<const MeshDraw> draws;
    std::optional<SelectionBox> selection;
    bool request_screenshot = false;
};

enum class FrameStatus { Presented, Skipped };
enum class FrameSkipReason { None, ZeroExtent, Occluded };

struct FrameStatistics {
    std::chrono::nanoseconds cpu_prepare{};
    std::chrono::nanoseconds cpu_submit{};
    std::chrono::nanoseconds present_wait{};
    std::uint64_t upload_bytes = 0;
    std::uint64_t draw_calls = 0;
    std::uint64_t drawn_vertices = 0;
};

struct Screenshot {
    Extent2D extent;
    // Owned top-left, tightly packed RGBA8 rows, independent of FrameInput storage.
    std::vector<std::uint8_t> pixels;
};

struct FrameResult {
    std::uint64_t frame_id = 0;
    FrameStatus status = FrameStatus::Skipped;
    FrameSkipReason skip_reason = FrameSkipReason::None;
    FrameStatistics statistics;
    std::optional<Screenshot> screenshot;
};

enum class PresentMode { Unknown, Immediate, Fifo, Mailbox };

struct RendererCapabilities {
    bool texture_arrays = false;
    bool selection_boxes = false;
    bool screenshots = false;
    bool gpu_timestamps = false;
};

struct RendererInfo {
    Backend backend = Backend::D3D12;
    std::string device_name;
    std::string device_identity;
    std::string driver_version;
    std::string api_version;
    std::string framebuffer_format;
    Extent2D framebuffer_extent;
    std::uint32_t actual_samples = 0;
    PresentMode actual_present_mode = PresentMode::Unknown;
    bool validation_enabled = false;
    RendererCapabilities capabilities;
};

enum class GpuSampleMissingReason { None, Unsupported, Discarded, DeviceFailure };

struct GpuFrameSample {
    std::uint64_t frame_id = 0;
    bool valid = false;
    // Missing/invalid samples have no duration; they are not zero-cost samples.
    std::optional<std::chrono::nanoseconds> gpu_draw_time;
    GpuSampleMissingReason missing_reason = GpuSampleMissingReason::Discarded;
};

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

// Error taxonomy and translation of existing std exceptions are not frozen yet.
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

// Only declarations: consumers may compile against this shape, not run a facade.
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

} // namespace SymoCraft::Experimental::D3D12R1::Candidate
