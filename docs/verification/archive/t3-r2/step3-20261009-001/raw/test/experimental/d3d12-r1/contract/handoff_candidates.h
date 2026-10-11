#pragma once

#include "candidate_contract.h"
#include <symocraft/assets/image.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/world/world.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <map>

namespace SymoCraft::Experimental::D3D12R1::Handoff {

// Isolated transition candidates, not a production API or proof of GPU equivalence.
inline Candidate::CameraParameters FromLegacyCamera(const Camera& camera) {
    Candidate::CameraParameters result;
    result.position = camera.GetCameraPos();
    result.forward = camera.GetCameraFront();
    result.up = camera.GetCameraUp();
    result.vertical_fov_radians = glm::radians(camera.GetFov());
    // Camera has no public near/far getters; parity tests bind these to its implementation.
    result.near_plane = 0.1f;
    result.far_plane = 2000.0f;
    Candidate::ValidateCamera(result);
    return result;
}

inline glm::dmat4 ViewRH(const Candidate::CameraParameters& camera) {
    Candidate::ValidateCamera(camera);
    const glm::dvec3 position{camera.position};
    // Separate basis construction from translation so a large eye cannot erase a small direction.
    const auto orientation = glm::lookAtRH(glm::dvec3{0.0}, glm::dvec3{camera.forward}, glm::dvec3{camera.up});
    return glm::translate(orientation, -position);
}

inline glm::dmat4 ProjectionRH_NO(const Candidate::CameraParameters& camera,
                                Candidate::Extent2D extent) {
    Candidate::ValidateCamera(camera);
    return glm::perspectiveRH_NO(static_cast<double>(camera.vertical_fov_radians),
                                extent.AspectRatio(), static_cast<double>(camera.near_plane),
                                static_cast<double>(camera.far_plane));
}

inline glm::dmat4 ProjectionRH_ZO(const Candidate::CameraParameters& camera,
                                Candidate::Extent2D extent) {
    Candidate::ValidateCamera(camera);
    return glm::perspectiveRH_ZO(static_cast<double>(camera.vertical_fov_radians),
                                extent.AspectRatio(), static_cast<double>(camera.near_plane),
                                static_cast<double>(camera.far_plane));
}

inline Candidate::TextureArrayData FromLegacyAtlas(const Assets::Image& top_left_image,
                                                 std::uint32_t tile_size = 64) {
    if (top_left_image.width <= 0 || top_left_image.height <= 0 || tile_size == 0 ||
        static_cast<std::uint32_t>(top_left_image.width) % tile_size != 0 ||
        static_cast<std::uint32_t>(top_left_image.height) % tile_size != 0)
        throw std::invalid_argument("legacy atlas must contain complete positive square tiles");
    if (top_left_image.channels != 3 && top_left_image.channels != 4)
        throw std::invalid_argument("legacy atlas requires RGB or RGBA bytes");
    const auto image_width = static_cast<std::uint32_t>(top_left_image.width);
    const auto image_height = static_cast<std::uint32_t>(top_left_image.height);
    const auto channels = static_cast<std::size_t>(top_left_image.channels);
    const auto bytes = Candidate::CheckedMultiply(
        Candidate::CheckedMultiply(image_width, image_height), channels);
    if (top_left_image.pixels.size() != bytes)
        throw std::invalid_argument("legacy atlas bytes do not match dimensions");
    const auto columns = image_width / tile_size;
    const auto rows = image_height / tile_size;
    const auto layers = Candidate::CheckedMultiply(columns, rows);
    if (layers > std::numeric_limits<std::uint16_t>::max())
        throw std::invalid_argument("legacy atlas layer count exceeds its uint16 contract");

    Candidate::TextureArrayData result;
    result.width = result.height = tile_size;
    result.layers = static_cast<std::uint32_t>(layers);
    // Legacy GL used RGB8/RGBA8 without sRGB decode or framebuffer encoding.
    result.color_space = Candidate::ColorSpace::Linear;
    result.pixels.resize(Candidate::TextureByteSize(result));
    for (std::uint32_t layer = 0; layer < result.layers; ++layer) {
        const auto tile_x = layer % columns;
        const auto tile_y = rows - 1 - layer / columns;
        for (std::uint32_t y = 0; y < tile_size; ++y)
            for (std::uint32_t x = 0; x < tile_size; ++x) {
                const auto source = ((static_cast<std::size_t>(tile_y) * tile_size + y) *
                                     image_width + tile_x * tile_size + x) * channels;
                const auto target = ((static_cast<std::size_t>(layer) * tile_size + y) * tile_size + x) * 4;
                for (std::size_t channel = 0; channel < 3; ++channel)
                    result.pixels[target + channel] = top_left_image.pixels[source + channel];
                result.pixels[target + 3] = channels == 4 ? top_left_image.pixels[source + 3] : 255;
            }
    }
    return result;
}

// Preserve world UVs (v=1 at the top); convert once at backend sampling, not per layer.
inline glm::vec2 LegacyUVToTopLeftSampling(glm::vec2 uv) noexcept { return {uv.x, 1.0f - uv.y}; }

struct OwnedMeshRecord {
    World::MeshIdentity identity;
    World::Revision revision;
    World::Revision mesh_input_revision;
    MeshData mesh;
};

inline OwnedMeshRecord CopyMeshRecord(const World::WorldMeshRecord& record) {
    return {record.identity, record.revision, record.mesh_input_revision,
            MeshData{{record.vertices.begin(), record.vertices.end()}}};
}

struct IdentityLess {
    bool operator()(const World::MeshIdentity& left, const World::MeshIdentity& right) const noexcept {
        if (left.world != right.world) return left.world < right.world;
        if (left.chunk.x != right.chunk.x) return left.chunk.x < right.chunk.x;
        return left.chunk.y < right.chunk.y;
    }
};

struct MeshAcceptance {
    Candidate::MeshHandle handle;
    World::Revision accepted_revision = 0;
};

// Models callback ownership and successful-version bookkeeping, not GPU resource retirement.
class MeshPublicationCache final {
public:
    template <class Renderer>
    bool Accept(const World::WorldMeshRecord& record, Renderer& renderer) {
        auto found = entries_.find(record.identity);
        if (found != entries_.end() && found->second.accepted_revision == record.revision)
            return false;
        const auto owned = CopyMeshRecord(record);
        if (found != entries_.end()) {
            renderer.UpdateMesh(found->second.handle, owned.mesh);
            found->second.accepted_revision = owned.revision;
            return true;
        }
        // Allocate bookkeeping before publishing a resource, so insertion failure cannot leak one.
        auto inserted = entries_.try_emplace(owned.identity).first;
        try {
            inserted->second.handle = renderer.CreateMesh(owned.mesh);
            inserted->second.accepted_revision = owned.revision;
        } catch (...) {
            entries_.erase(inserted);
            throw;
        }
        return true;
    }

    [[nodiscard]] const MeshAcceptance* Find(World::MeshIdentity identity) const noexcept {
        const auto found = entries_.find(identity);
        return found == entries_.end() ? nullptr : &found->second;
    }
    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }

    template <class Renderer>
    void ReleaseWorld(World::WorldId identity, Renderer& renderer) {
        for (auto entry = entries_.begin(); entry != entries_.end();) {
            if (entry->first.world != identity) { ++entry; continue; }
            renderer.DestroyMesh(entry->second.handle);
            entry = entries_.erase(entry);
        }
    }

private:
    std::map<World::MeshIdentity, MeshAcceptance, IdentityLess> entries_;
};

} // namespace SymoCraft::Experimental::D3D12R1::Handoff
