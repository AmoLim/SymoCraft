#pragma once

#include "symocraft/scene/mesh.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <numbers>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace SymoCraft::Rendering {

enum class Backend { OpenGL, D3D12, Vulkan };
enum class PixelFormat { RGBA8 };
enum class ColorSpace { Linear, SRGB };

struct RendererConfig {
    Backend backend = Backend::D3D12;
    bool vsync = true;
    std::uint32_t requested_samples = 1;
    bool enable_validation = true;
    std::filesystem::path shader_asset_root;
    std::chrono::milliseconds wait_timeout{2500};
};

struct TextureArrayData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t layers = 0;
    PixelFormat format = PixelFormat::RGBA8;
    ColorSpace color_space = ColorSpace::Linear;
    // Top-left RGBA8 rows, tightly packed, followed by ascending layers.
    std::vector<std::uint8_t> pixels;
};

struct Extent2D {
    std::uint32_t width = 0;
    std::uint32_t height = 0;

    [[nodiscard]] bool IsSuspended() const noexcept { return width == 0 || height == 0; }
    [[nodiscard]] double AspectRatio() const {
        if (IsSuspended()) throw std::invalid_argument("zero extent has no aspect ratio");
        return static_cast<double>(width) / height;
    }
};

struct CameraParameters {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 forward{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float vertical_fov_radians = std::numbers::pi_v<float> / 3.0f;
    float near_plane = 0.1f;
    float far_plane = 1000.0f;
};

namespace Detail {
class ResourceRegistry;
struct MeshTag;
struct TextureArrayTag;
}

template <class Tag>
class ResourceHandle final {
public:
    ResourceHandle() = default;
    [[nodiscard]] bool IsNull() const noexcept { return owner_ == 0; }
    friend bool operator==(const ResourceHandle&, const ResourceHandle&) = default;

private:
    friend class Detail::ResourceRegistry;
    ResourceHandle(std::uint64_t owner, std::size_t slot, std::uint64_t generation)
        : owner_(owner), slot_(slot), generation_(generation) {}

    std::uint64_t owner_ = 0;
    std::size_t slot_ = 0;
    std::uint64_t generation_ = 0;
};

using MeshHandle = ResourceHandle<Detail::MeshTag>;
using TextureArrayHandle = ResourceHandle<Detail::TextureArrayTag>;

struct MeshDraw {
    MeshHandle mesh;
    TextureArrayHandle texture_array;
    // Scene mesh positions already use world space.
    glm::mat4 object_transform{1.0f};
};

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
    // Owned top-left, tightly packed RGBA8 rows.
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
    // A missing or invalid duration is not a zero-cost sample.
    std::optional<std::chrono::nanoseconds> gpu_draw_time;
    GpuSampleMissingReason missing_reason = GpuSampleMissingReason::Discarded;
};

} // namespace SymoCraft::Rendering
