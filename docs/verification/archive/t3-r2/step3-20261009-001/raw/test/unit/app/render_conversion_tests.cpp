#include "render_conversion.h"

#include <symocraft/assets/image.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/world/constants.h>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>
#include <string>
#include <string_view>

namespace AppRendering = SymoCraft::AppRendering;
namespace Rendering = SymoCraft::Rendering;

namespace {
std::size_t checks = 0;
void Check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string{message});
}
template <class Operation>
void Reject(Operation&& operation) {
    bool rejected = false;
    try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "invalid game atlas must be explicitly rejected");
}
bool Near(double left, double right, double epsilon = 1e-5) {
    return std::abs(left - right) <= epsilon;
}
bool MatrixNear(const glm::mat4& left, const glm::mat4& right) {
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!Near(left[column][row], right[column][row])) return false;
    return true;
}

void CameraConversion() {
    SymoCraft::ECS::Registry registry;
    registry.RegisterComponent<SymoCraft::Transform>("Transform");
    SymoCraft::Camera camera{registry, 1280, 720, {12.5f, 4.0f, -9.5f}};
    SymoCraft::TransformSystem::Update(registry);
    auto result = AppRendering::ToRendererCamera(camera);
    Check(result.position == camera.GetCameraPos(), "camera position transfers in world space");
    Check(result.forward == camera.GetCameraFront() && result.up == camera.GetCameraUp(),
          "camera direction and up transfer without handedness changes");
    Check(Near(result.vertical_fov_radians, std::numbers::pi / 4), "45 degree vertical FOV converts to radians");
    Check(result.near_plane == 0.1f && result.far_plane == 2000.0f,
          "game clipping planes override renderer data defaults");
    const auto view = glm::lookAtRH(result.position, result.position + result.forward, result.up);
    Check(MatrixNear(view, camera.GetCameraViewMat()), "transferred RH view agrees with real Camera");
    const auto old_projection = glm::perspectiveRH_NO(
        result.vertical_fov_radians, 1280.0f / 720.0f, result.near_plane, result.far_plane);
    Check(MatrixNear(old_projection, camera.GetCameraProjMat(1280.0f / 720.0f)),
          "transferred clipping and FOV values preserve the current Camera projection");
    camera.SetYaw(35);
    camera.SetPitch(-22);
    SymoCraft::TransformSystem::Update(registry);
    result = AppRendering::ToRendererCamera(camera);
    Check(MatrixNear(glm::lookAtRH(result.position, result.position + result.forward, result.up),
                     camera.GetCameraViewMat()), "rotated camera uses actual current getters");
    const auto eye = glm::lookAtRH(result.position, result.position + result.forward, result.up) *
                     glm::vec4{result.position, 1};
    Check(glm::length(glm::vec3{eye}) < 1e-4, "camera eye maps to the RH view origin");
    camera.Scroll(1000);
    Check(Near(AppRendering::ToRendererCamera(camera).vertical_fov_radians, std::numbers::pi / 180),
          "minimum one-degree zoom remains supported");
    camera.Scroll(-1000);
    Check(Near(AppRendering::ToRendererCamera(camera).vertical_fov_radians, std::numbers::pi / 4),
          "maximum 45-degree zoom remains supported");
    const auto projection = glm::perspectiveRH_ZO(
        static_cast<double>(result.vertical_fov_radians), 1280.0 / 720.0,
        static_cast<double>(result.near_plane), static_cast<double>(result.far_plane));
    const auto near = projection * glm::dvec4{0, 0, -result.near_plane, 1};
    const auto far = projection * glm::dvec4{0, 0, -result.far_plane, 1};
    Check(Near(near.z / near.w, 0, 1e-12) && Near(far.z / far.w, 1, 1e-12),
          "neutral game camera permits RH zero-to-one projection with unchanged clipping planes");
}

SymoCraft::Assets::Image SyntheticAtlas(int channels) {
    SymoCraft::Assets::Image image{512, 512, channels, {}};
    image.pixels.resize(static_cast<std::size_t>(512 * 512 * channels));
    for (int y = 0; y < 512; ++y)
        for (int x = 0; x < 512; ++x) {
            const auto offset = static_cast<std::size_t>((y * 512 + x) * channels);
            image.pixels[offset] = static_cast<std::uint8_t>(x / 64);
            image.pixels[offset + 1] = static_cast<std::uint8_t>(y / 64);
            image.pixels[offset + 2] = static_cast<std::uint8_t>(y % 64);
            if (channels == 4) image.pixels[offset + 3] = static_cast<std::uint8_t>(x % 64);
        }
    return image;
}

void AtlasConversion(const std::filesystem::path& atlas_path) {
    auto source = SyntheticAtlas(4);
    const auto array = AppRendering::ToRendererAtlas(source);
    Check(array.width == 64 && array.height == 64 && array.layers == 64 &&
          array.pixels.size() == 1048576, "fixed game atlas retains every 64x64 tile and all 64 layers");
    Check(array.format == Rendering::PixelFormat::RGBA8 && array.color_space == Rendering::ColorSpace::Linear,
          "game atlas remains RGBA8 without adding sRGB interpretation");
    Check(array.pixels[0] == 0 && array.pixels[1] == 7 && array.pixels[2] == 0,
          "layer zero is the bottom-left tile with its top row first");
    Check(array.pixels[64 * 64 * 4] == 1 && array.pixels[64 * 64 * 4 + 1] == 7,
          "layer one advances right along the bottom tile row");
    Check(array.pixels[8 * 64 * 64 * 4 + 1] == 6,
          "layer eight advances upward one tile row");
    Check(array.pixels[64 * 4 + 2] == 1 && array.pixels[4 + 3] == 1,
          "each tile preserves top-to-bottom rows and all alpha bytes");
    source.pixels[static_cast<std::size_t>(448 * 512 * 4)] = 255;
    Check(array.pixels[0] == 0, "converted texture array owns storage independent of the source image");
    const auto rgb = AppRendering::ToRendererAtlas(SyntheticAtlas(3));
    Check(rgb.pixels[0] == 0 && rgb.pixels[1] == 7 && rgb.pixels[3] == 255,
          "RGB source expands to opaque RGBA without changing colors");
    source = SyntheticAtlas(4);
    source.width = 511;
    Reject([&] { AppRendering::ToRendererAtlas(source); });
    source = SyntheticAtlas(4);
    source.height = 256;
    Reject([&] { AppRendering::ToRendererAtlas(source); });
    source = SyntheticAtlas(4);
    source.channels = 2;
    Reject([&] { AppRendering::ToRendererAtlas(source); });
    source = SyntheticAtlas(4);
    source.pixels.pop_back();
    Reject([&] { AppRendering::ToRendererAtlas(source); });
    source = SyntheticAtlas(4);
    source.pixels.push_back(0);
    Reject([&] { AppRendering::ToRendererAtlas(source); });

    const auto real_top_left = SymoCraft::Assets::DecodeImage(atlas_path, false);
    const auto real_gl_bottom_left = SymoCraft::Assets::DecodeImage(atlas_path, true);
    const auto real_array = AppRendering::ToRendererAtlas(real_top_left);
    Check(real_top_left.width == 512 && real_top_left.height == 512 && real_top_left.channels == 4,
          "unchanged real production atlas has its expected workload dimensions");
    bool matched = true;
    std::size_t transparent = 0;
    for (std::uint32_t layer = 0; layer < 64; ++layer)
        for (std::uint32_t y = 0; y < 64; ++y)
            for (std::uint32_t x = 0; x < 64; ++x) {
                const auto gl_x = layer % 8 * 64 + x;
                const auto gl_y = layer / 8 * 64 + (63 - y);
                const auto gl_offset = (static_cast<std::size_t>(gl_y) * 512 + gl_x) * 4;
                const auto modern_offset = ((static_cast<std::size_t>(layer) * 64 + y) * 64 + x) * 4;
                for (std::size_t channel = 0; channel < 4; ++channel)
                    matched &= real_array.pixels[modern_offset + channel] ==
                               real_gl_bottom_left.pixels[gl_offset + channel];
                transparent += real_array.pixels[modern_offset + 3] < 3;
            }
    Check(matched, "all real atlas RGBA bytes match GL bottom-first slicing after one row-direction conversion");
    Check(transparent != 0, "real cutout alpha bytes are retained");
}

void MeshConversion() {
    const SymoCraft::MeshData source{{{{-16, 12, 32}, {-0.25f, 1.25f, 4.5f}, 0.75f},
                                    {{-15, 12, 32}, {1, 0, 2}, 0.5f},
                                    {{-16, 13, 32}, {0, 1, 2}, 0.25f}}};
    const auto result = AppRendering::CopyRendererMesh(source.vertices);
    Check(result.vertices.size() == source.vertices.size() && result.vertices.data() != source.vertices.data(),
          "mesh conversion takes an owned copy of callback vertices");
    bool preserved = true;
    for (std::size_t index = 0; index < source.vertices.size(); ++index) {
        const auto& original = source.vertices[index];
        const auto& converted = result.vertices[index];
        preserved &= original.pos_coord == converted.pos_coord && original.normal == converted.normal &&
                     original.tex_coord.x == converted.tex_coord.x && original.tex_coord.z == converted.tex_coord.z;
    }
    Check(preserved, "world position, normal, vertex order, U and fractional layer remain unchanged");
    Check(result.vertices[0].tex_coord.y == -0.25f && result.vertices[1].tex_coord.y == 1 &&
          result.vertices[2].tex_coord.y == 0, "single V conversion preserves repeat coordinates without clamping");
    Check(source.vertices[0].tex_coord.y == 1.25f, "mesh conversion does not mutate borrowed source UVs");
    Check(AppRendering::CopyRendererMesh({}).vertices.empty(), "empty published mesh produces a valid owned empty mesh");
    const std::array<glm::ivec3, 6> normals{{{1, 0, 0}, {0, 0, 1}, {-1, 0, 0},
                                         {0, 0, -1}, {0, 1, 0}, {0, -1, 0}}};
    for (std::size_t face = 0; face < normals.size(); ++face) {
        const auto offset = face * 4;
        const auto a = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[offset]];
        const auto b = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[offset + 1]];
        const auto c = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[offset + 2]];
        Check(glm::cross(glm::vec3{b - a}, glm::vec3{c - a}) == glm::vec3{normals[face]},
              "production mesh face indices retain outward RH winding");
    }
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("provide the production texture_atlas.png path");
        CameraConversion();
        AtlasConversion(argv[1]);
        MeshConversion();
        std::cout << "Application render conversion: " << checks << " CPU checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Application render conversion failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
