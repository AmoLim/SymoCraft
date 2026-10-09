#pragma once

#include "symocraft/scene/mesh.h"

#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <vector>

namespace SymoCraft::Experimental::D3D12R1::Candidate {

// Experiment-only candidates, not a frozen production Renderer v1 header.
enum class PixelFormat { RGBA8 };
enum class ColorSpace { Linear, SRGB };

struct TextureArrayData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t layers = 0;
    PixelFormat format = PixelFormat::RGBA8;
    ColorSpace color_space = ColorSpace::Linear;
    // RGBA bytes: top-left origin, tightly packed rows, then layers in ascending order.
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

inline std::size_t CheckedMultiply(std::size_t a, std::size_t b) {
    if (a != 0 && b > std::numeric_limits<std::size_t>::max() / a)
        throw std::invalid_argument("texture byte size overflow");
    return a * b;
}

inline std::size_t TextureByteSize(const TextureArrayData& data) {
    if (data.width == 0 || data.height == 0 || data.layers == 0)
        throw std::invalid_argument("texture dimensions and layers must be positive");
    if (data.format != PixelFormat::RGBA8)
        throw std::invalid_argument("unsupported texture pixel format");
    return CheckedMultiply(CheckedMultiply(CheckedMultiply(data.width, data.height), data.layers), 4);
}

inline void ValidateTextureArray(const TextureArrayData& data) {
    if (data.color_space != ColorSpace::Linear && data.color_space != ColorSpace::SRGB)
        throw std::invalid_argument("unsupported texture color space");
    if (data.pixels.size() != TextureByteSize(data))
        throw std::invalid_argument("texture pixels must exactly match tightly packed RGBA8 size");
}

inline std::size_t TexelByteOffset(const TextureArrayData& data, std::uint32_t layer,
                                 std::uint32_t x, std::uint32_t y) {
    ValidateTextureArray(data);
    if (layer >= data.layers || x >= data.width || y >= data.height)
        throw std::invalid_argument("texel outside texture array");
    return ((static_cast<std::size_t>(layer) * data.height + y) * data.width + x) * 4;
}

inline void ValidateMesh(const MeshData& data) {
    if (data.vertices.size() % 3 != 0)
        throw std::invalid_argument("mesh must contain complete nonindexed triangles");
    for (const auto& vertex : data.vertices) {
        const auto& uv_layer = vertex.tex_coord;
        if (!std::isfinite(uv_layer.x) || !std::isfinite(uv_layer.y) ||
            !std::isfinite(uv_layer.z) || !std::isfinite(vertex.normal))
            throw std::invalid_argument("mesh attributes must be finite");
        const double layer = uv_layer.z;
        if (layer < 0.0 || layer != std::floor(layer) ||
            layer > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("texture layer must be a nonnegative uint32 integer");
    }
}

inline void ValidateMeshTextureLayers(const MeshData& mesh, std::uint32_t layers) {
    ValidateMesh(mesh);
    if (layers == 0) throw std::invalid_argument("texture array must have layers");
    for (const auto& vertex : mesh.vertices)
        if (static_cast<double>(vertex.tex_coord.z) >= layers)
            throw std::invalid_argument("mesh texture layer outside bound texture array");
}

inline void ValidateCamera(const CameraParameters& camera) {
    const auto finite = [](const glm::vec3& value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!finite(camera.position) || !finite(camera.forward) || !finite(camera.up) ||
        !std::isfinite(camera.vertical_fov_radians) || !std::isfinite(camera.near_plane) ||
        !std::isfinite(camera.far_plane))
        throw std::invalid_argument("camera parameters must be finite");
    if (camera.vertical_fov_radians <= 0.0f ||
        camera.vertical_fov_radians >= std::numbers::pi_v<float> ||
        camera.near_plane <= 0.0f || camera.far_plane <= camera.near_plane)
        throw std::invalid_argument("camera perspective range is invalid");
    // Use double for validation so finite float directions cannot overflow dot/cross products.
    const glm::dvec3 forward{camera.forward};
    const glm::dvec3 up{camera.up};
    const auto cross = glm::cross(forward, up);
    if (glm::dot(forward, forward) == 0.0 || glm::dot(up, up) == 0.0 ||
        glm::dot(cross, cross) == 0.0)
        throw std::invalid_argument("camera forward and up must form a nondegenerate basis");
}

class RendererRegistry;
namespace Detail {
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
    friend class RendererRegistry;
    ResourceHandle(std::uint64_t owner, std::size_t slot, std::uint64_t generation)
        : owner_(owner), slot_(slot), generation_(generation) {}

    std::uint64_t owner_ = 0;
    std::size_t slot_ = 0;
    std::uint64_t generation_ = 0;
};

using MeshHandle = ResourceHandle<Detail::MeshTag>;
using TextureArrayHandle = ResourceHandle<Detail::TextureArrayTag>;

// This registry models CPU acceptance and logical invalidation only, never GPU retirement.
class RendererRegistry final {
public:
    RendererRegistry() : identity_(NextIdentity()) {}
    RendererRegistry(const RendererRegistry&) = delete;
    RendererRegistry& operator=(const RendererRegistry&) = delete;
    RendererRegistry(RendererRegistry&&) = delete;
    RendererRegistry& operator=(RendererRegistry&&) = delete;

    MeshHandle CreateMesh(const MeshData& data) {
        ValidateMesh(data);
        return Create<Detail::MeshTag>(data, meshes_);
    }
    void UpdateMesh(MeshHandle handle, const MeshData& data) {
        auto& slot = Find(handle, meshes_);
        ValidateMesh(data);
        auto accepted = std::make_unique<MeshData>(data);
        slot.data.swap(accepted);
    }
    void DestroyMesh(MeshHandle handle) { Destroy(Find(handle, meshes_)); }
    [[nodiscard]] MeshData SnapshotMesh(MeshHandle handle) const { return *Find(handle, meshes_).data; }

    TextureArrayHandle CreateTextureArray(const TextureArrayData& data) {
        ValidateTextureArray(data);
        return Create<Detail::TextureArrayTag>(data, textures_);
    }
    // Texture update is an experiment extension, not an addition to the T3 draft facade.
    void UpdateTextureArray(TextureArrayHandle handle, const TextureArrayData& data) {
        auto& slot = Find(handle, textures_);
        ValidateTextureArray(data);
        auto accepted = std::make_unique<TextureArrayData>(data);
        slot.data.swap(accepted);
    }
    void DestroyTextureArray(TextureArrayHandle handle) { Destroy(Find(handle, textures_)); }
    [[nodiscard]] TextureArrayData SnapshotTextureArray(TextureArrayHandle handle) const {
        return *Find(handle, textures_).data;
    }

    void ValidateDraw(MeshHandle mesh, TextureArrayHandle texture) const {
        const auto& mesh_data = *Find(mesh, meshes_).data;
        const auto& texture_data = *Find(texture, textures_).data;
        ValidateMeshTextureLayers(mesh_data, texture_data.layers);
    }

private:
    template <class Data>
    struct Slot {
        std::uint64_t generation = 1;
        std::unique_ptr<Data> data;
    };

    static std::uint64_t NextIdentity() {
        static std::atomic<std::uint64_t> next{1};
        auto candidate = next.load(std::memory_order_relaxed);
        for (;;) {
            if (candidate == std::numeric_limits<std::uint64_t>::max())
                throw std::overflow_error("renderer registry identity exhausted");
            if (next.compare_exchange_weak(candidate, candidate + 1, std::memory_order_relaxed))
                return candidate;
        }
    }

    template <class Tag, class Data>
    ResourceHandle<Tag> Create(const Data& data, std::vector<Slot<Data>>& slots) {
        auto accepted = std::make_unique<Data>(data);
        std::size_t index = 0;
        while (index < slots.size() && (slots[index].data || slots[index].generation == 0)) ++index;
        if (index == slots.size()) slots.emplace_back();
        slots[index].data = std::move(accepted);
        return ResourceHandle<Tag>{identity_, index, slots[index].generation};
    }

    template <class Tag, class Data>
    const Slot<Data>& Find(ResourceHandle<Tag> handle, const std::vector<Slot<Data>>& slots) const {
        if (handle.owner_ != identity_ || handle.slot_ >= slots.size())
            throw std::invalid_argument("invalid or foreign renderer resource handle");
        const auto& slot = slots[handle.slot_];
        if (!slot.data || handle.generation_ != slot.generation)
            throw std::invalid_argument("stale renderer resource handle");
        return slot;
    }

    template <class Tag, class Data>
    Slot<Data>& Find(ResourceHandle<Tag> handle, std::vector<Slot<Data>>& slots) {
        static_cast<const RendererRegistry&>(*this).Find(handle, slots);
        return slots[handle.slot_];
    }

    template <class Data>
    static void Destroy(Slot<Data>& slot) noexcept {
        slot.data.reset();
        // Generation zero permanently retires a slot rather than reusing an old identity.
        slot.generation = slot.generation == std::numeric_limits<std::uint64_t>::max()
                              ? 0
                              : slot.generation + 1;
    }

    const std::uint64_t identity_;
    std::vector<Slot<MeshData>> meshes_;
    std::vector<Slot<TextureArrayData>> textures_;
};

} // namespace SymoCraft::Experimental::D3D12R1::Candidate
