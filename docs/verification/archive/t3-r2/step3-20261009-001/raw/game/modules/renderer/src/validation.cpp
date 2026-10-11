#include "validation.h"

#include <cmath>
#include <limits>

namespace SymoCraft::Rendering::Detail {
namespace {

bool Finite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

std::size_t CheckedMultiply(std::size_t left, std::size_t right) {
    if (left != 0 && right > std::numeric_limits<std::size_t>::max() / left)
        throw std::invalid_argument("texture byte size overflow");
    return left * right;
}

} // namespace

void ValidateConfig(const RendererConfig& config) {
    if (config.backend != Backend::OpenGL && config.backend != Backend::D3D12 &&
        config.backend != Backend::Vulkan)
        throw std::invalid_argument("unknown renderer backend");
    if (config.requested_samples == 0)
        throw std::invalid_argument("requested sample count must be positive");
    if (config.wait_timeout.count() <= 0)
        throw std::invalid_argument("renderer wait timeout must be positive");
}

void ValidateMesh(const MeshData& mesh) {
    if (mesh.vertices.size() % 3 != 0)
        throw std::invalid_argument("mesh must contain complete nonindexed triangles");
    for (const auto& vertex : mesh.vertices) {
        if (!Finite(glm::vec3{vertex.pos_coord}) || !Finite(vertex.tex_coord) || !std::isfinite(vertex.normal))
            throw std::invalid_argument("mesh attributes must be finite");
        const double layer = vertex.tex_coord.z;
        if (layer < 0.0 || layer != std::floor(layer) ||
            layer > std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("texture layer must be a nonnegative uint32 integer");
    }
}

void ValidateTextureArray(const TextureArrayData& texture) {
    if (texture.width == 0 || texture.height == 0 || texture.layers == 0)
        throw std::invalid_argument("texture dimensions and layer count must be positive");
    if (texture.format != PixelFormat::RGBA8)
        throw std::invalid_argument("unsupported texture pixel format");
    if (texture.color_space != ColorSpace::Linear && texture.color_space != ColorSpace::SRGB)
        throw std::invalid_argument("unsupported texture color space");
    const auto bytes = CheckedMultiply(CheckedMultiply(
        CheckedMultiply(texture.width, texture.height), texture.layers), 4);
    if (texture.pixels.size() != bytes)
        throw std::invalid_argument("texture pixels must exactly match tightly packed RGBA8 size");
}

void ValidateMeshTextureLayers(const MeshData& mesh, std::uint32_t layers) {
    if (layers == 0) throw std::invalid_argument("texture array must have layers");
    for (const auto& vertex : mesh.vertices)
        if (static_cast<double>(vertex.tex_coord.z) >= layers)
            throw std::invalid_argument("mesh texture layer outside bound texture array");
}

void ValidateCamera(const CameraParameters& camera) {
    if (!Finite(camera.position) || !Finite(camera.forward) || !Finite(camera.up) ||
        !std::isfinite(camera.vertical_fov_radians) || !std::isfinite(camera.near_plane) ||
        !std::isfinite(camera.far_plane))
        throw std::invalid_argument("camera parameters must be finite");
    if (camera.vertical_fov_radians <= 0.0f ||
        camera.vertical_fov_radians >= std::numbers::pi_v<float> ||
        camera.near_plane <= 0.0f || camera.far_plane <= camera.near_plane)
        throw std::invalid_argument("camera perspective range is invalid");
    const glm::dvec3 forward{camera.forward};
    const glm::dvec3 up{camera.up};
    const auto cross = glm::cross(forward, up);
    if (glm::dot(forward, forward) == 0.0 || glm::dot(up, up) == 0.0 ||
        glm::dot(cross, cross) == 0.0)
        throw std::invalid_argument("camera forward and up must form a nondegenerate basis");
}

void ValidateTransform(const glm::mat4& transform) {
    for (glm::length_t column = 0; column < 4; ++column)
        for (glm::length_t row = 0; row < 4; ++row)
            if (!std::isfinite(transform[column][row]))
                throw std::invalid_argument("object transform must be finite");
}

void ValidateSelection(const SelectionBox& selection) {
    if (!Finite(selection.minimum) || !Finite(selection.maximum) ||
        selection.minimum.x > selection.maximum.x ||
        selection.minimum.y > selection.maximum.y ||
        selection.minimum.z > selection.maximum.z)
        throw std::invalid_argument("selection must be a finite ordered AABB");
}

} // namespace SymoCraft::Rendering::Detail
