#include "render_conversion.h"

#include <symocraft/assets/image.h>
#include <symocraft/simulation/camera.h>

#include <stdexcept>

namespace SymoCraft::AppRendering {

Rendering::CameraParameters ToRendererCamera(const Camera& camera) {
    Rendering::CameraParameters result;
    result.position = camera.GetCameraPos();
    result.forward = camera.GetCameraFront();
    result.up = camera.GetCameraUp();
    result.vertical_fov_radians = glm::radians(camera.GetFov());
    // The current Camera projection has fixed clipping planes, not public getters.
    result.near_plane = 0.1f;
    result.far_plane = 2000.0f;
    return result;
}

Rendering::TextureArrayData ToRendererAtlas(const Assets::Image& top_left_image) {
    constexpr std::uint32_t atlas_extent = 512;
    constexpr std::uint32_t tile_extent = 64;
    constexpr std::uint32_t tile_columns = atlas_extent / tile_extent;
    constexpr std::uint32_t layer_count = tile_columns * tile_columns;
    if (top_left_image.width != static_cast<int>(atlas_extent) ||
        top_left_image.height != static_cast<int>(atlas_extent))
        throw std::invalid_argument("game atlas must retain its 512x512 dimensions");
    if (top_left_image.channels != 3 && top_left_image.channels != 4)
        throw std::invalid_argument("game atlas requires RGB or RGBA bytes");
    const auto channels = static_cast<std::size_t>(top_left_image.channels);
    const auto expected_bytes = static_cast<std::size_t>(atlas_extent) * atlas_extent * channels;
    if (top_left_image.pixels.size() != expected_bytes)
        throw std::invalid_argument("game atlas bytes do not match its dimensions");

    Rendering::TextureArrayData result;
    result.width = result.height = tile_extent;
    result.layers = layer_count;
    result.format = Rendering::PixelFormat::RGBA8;
    // Old GL used RGB8/RGBA8 with no texture sRGB decode or output encoding.
    result.color_space = Rendering::ColorSpace::Linear;
    result.pixels.resize(static_cast<std::size_t>(atlas_extent) * atlas_extent * 4);
    for (std::uint32_t layer = 0; layer < layer_count; ++layer) {
        const auto tile_x = layer % tile_columns;
        const auto tile_y = tile_columns - 1 - layer / tile_columns;
        for (std::uint32_t y = 0; y < tile_extent; ++y)
            for (std::uint32_t x = 0; x < tile_extent; ++x) {
                const auto source = ((static_cast<std::size_t>(tile_y) * tile_extent + y) *
                                     atlas_extent + tile_x * tile_extent + x) * channels;
                const auto target = ((static_cast<std::size_t>(layer) * tile_extent + y) *
                                     tile_extent + x) * 4;
                for (std::size_t channel = 0; channel < 3; ++channel)
                    result.pixels[target + channel] = top_left_image.pixels[source + channel];
                result.pixels[target + 3] = channels == 4 ? top_left_image.pixels[source + 3] : 255;
            }
    }
    return result;
}

MeshData CopyRendererMesh(MeshView source) {
    MeshData result{{source.begin(), source.end()}};
    // World UVs use v=1 at the top; modern texture rows use top-left storage.
    for (auto& vertex : result.vertices) vertex.tex_coord.y = 1.0f - vertex.tex_coord.y;
    return result;
}

} // namespace SymoCraft::AppRendering
