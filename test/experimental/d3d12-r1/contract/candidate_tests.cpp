#include "candidate_contract.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <type_traits>

namespace Candidate = SymoCraft::Experimental::D3D12R1::Candidate;

namespace {
bool fail_next_allocation = false;
}

// One-shot allocation failure tests the update transaction without changing the candidate API.
void* operator new(std::size_t size) {
    if (fail_next_allocation) {
        fail_next_allocation = false;
        throw std::bad_alloc{};
    }
    if (auto* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    throw std::bad_alloc{};
}
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }

namespace {
int checks = 0;

void Check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string{message});
}

template <class Function>
void Reject(Function&& operation, std::string_view message) {
    bool rejected = false;
    try {
        operation();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    Check(rejected, message);
}

template <class Function>
void AllocationFailure(Function&& operation, std::string_view message) {
    bool failed = false;
    fail_next_allocation = true;
    try {
        operation();
    } catch (const std::bad_alloc&) {
        failed = true;
    } catch (...) {
        fail_next_allocation = false;
        throw;
    }
    fail_next_allocation = false;
    Check(failed, message);
}

SymoCraft::MeshData Triangle(float layer = 0.0f) {
    return {{{{0, 0, 0}, {0.0f, 0.0f, layer}, 0.0f},
             {{1, 0, 0}, {1.0f, 0.0f, layer}, 0.0f},
             {{0, 1, 0}, {0.0f, 1.0f, layer}, 0.0f}}};
}

Candidate::TextureArrayData Texture(std::uint32_t layers = 2) {
    Candidate::TextureArrayData result;
    result.width = 2;
    result.height = 2;
    result.layers = layers;
    result.pixels.resize(Candidate::TextureByteSize(result));
    for (std::size_t i = 0; i < result.pixels.size(); ++i)
        result.pixels[i] = static_cast<std::uint8_t>(i);
    return result;
}

void MeshValidation() {
    static_assert(sizeof(SymoCraft::BlockVertex3D) == 28);
    static_assert(offsetof(SymoCraft::BlockVertex3D, pos_coord) == 0);
    static_assert(offsetof(SymoCraft::BlockVertex3D, tex_coord) == 12);
    static_assert(offsetof(SymoCraft::BlockVertex3D, normal) == 24);
    static_assert(!std::is_convertible_v<Candidate::MeshHandle, Candidate::TextureArrayHandle>);
    static_assert(!std::is_constructible_v<Candidate::MeshHandle, std::uint64_t, std::size_t, std::uint64_t>);
    Candidate::ValidateMesh({});
    Candidate::ValidateMesh(Triangle());
    Check(true, "empty and complete triangle inputs accepted");
    auto mesh = Triangle();
    mesh.vertices.pop_back();
    Reject([&] { Candidate::ValidateMesh(mesh); }, "incomplete triangle rejected");
    mesh = Triangle(-1.0f);
    Reject([&] { Candidate::ValidateMesh(mesh); }, "negative layer rejected");
    mesh = Triangle(0.5f);
    Reject([&] { Candidate::ValidateMesh(mesh); }, "fractional layer rejected");
    mesh = Triangle(std::numeric_limits<float>::infinity());
    Reject([&] { Candidate::ValidateMesh(mesh); }, "infinite layer rejected");
    mesh = Triangle(4294967296.0f);
    Reject([&] { Candidate::ValidateMesh(mesh); }, "layer larger than uint32 rejected");
    mesh = Triangle();
    mesh.vertices[0].tex_coord.x = std::numeric_limits<float>::quiet_NaN();
    Reject([&] { Candidate::ValidateMesh(mesh); }, "nonfinite UV rejected");
    mesh = Triangle();
    mesh.vertices[0].normal = std::numeric_limits<float>::infinity();
    Reject([&] { Candidate::ValidateMesh(mesh); }, "nonfinite normal attribute rejected");
    Candidate::ValidateMeshTextureLayers(Triangle(1), 2);
    Check(true, "last valid texture layer accepted");
    Reject([&] { Candidate::ValidateMeshTextureLayers(Triangle(2), 2); }, "layer bounds checked at draw");
}

void TextureValidation() {
    auto texture = Texture();
    Candidate::ValidateTextureArray(texture);
    Check(Candidate::TextureByteSize(texture) == 32, "RGBA8 byte count");
    Check(Candidate::TexelByteOffset(texture, 0, 0, 0) == 0, "top-left texel starts storage");
    Check(Candidate::TexelByteOffset(texture, 0, 1, 0) == 4, "adjacent texel is tightly packed");
    Check(Candidate::TexelByteOffset(texture, 0, 0, 1) == 8, "rows progress top to bottom");
    Check(Candidate::TexelByteOffset(texture, 1, 0, 0) == 16, "layers stored sequentially");
    texture.color_space = Candidate::ColorSpace::SRGB;
    Candidate::ValidateTextureArray(texture);
    Check(true, "explicit sRGB input accepted");
    texture.pixels.pop_back();
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "short texture storage rejected");
    texture = Texture();
    texture.pixels.push_back(0);
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "extra texture storage rejected");
    texture = Texture();
    texture.width = 0;
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "zero texture dimension rejected");
    texture = Texture();
    texture.layers = 0;
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "zero texture layer count rejected");
    texture = Texture();
    texture.format = static_cast<Candidate::PixelFormat>(9);
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "unknown pixel format rejected");
    texture = Texture();
    texture.color_space = static_cast<Candidate::ColorSpace>(9);
    Reject([&] { Candidate::ValidateTextureArray(texture); }, "unknown color space rejected");
    texture = Texture();
    texture.width = std::numeric_limits<std::uint32_t>::max();
    texture.height = std::numeric_limits<std::uint32_t>::max();
    texture.layers = std::numeric_limits<std::uint32_t>::max();
    Reject([&] { Candidate::TextureByteSize(texture); }, "texture size overflow rejected without allocation");
    texture = Texture();
    Reject([&] { Candidate::TexelByteOffset(texture, 2, 0, 0); }, "out of bounds layer rejected");
    Reject([&] { Candidate::TexelByteOffset(texture, 0, 2, 0); }, "out of bounds x rejected");
    Reject([&] { Candidate::TexelByteOffset(texture, 0, 0, 2); }, "out of bounds y rejected");
}

void RegistryOwnershipAndHandles() {
    Candidate::RendererRegistry registry;
    Candidate::RendererRegistry other;
    auto mesh = Triangle(1);
    const auto mesh_handle = registry.CreateMesh(mesh);
    mesh.vertices[0].pos_coord.x = 99;
    mesh.vertices.clear();
    Check(registry.SnapshotMesh(mesh_handle).vertices[0].pos_coord.x == 0,
          "mesh create owns snapshot before caller mutation/destruction");
    mesh = Triangle(1);
    mesh.vertices[0].pos_coord.x = 4;
    registry.UpdateMesh(mesh_handle, mesh);
    mesh.vertices[0].pos_coord.x = 88;
    Check(registry.SnapshotMesh(mesh_handle).vertices[0].pos_coord.x == 4,
          "mesh update owns snapshot before caller mutation");
    mesh.vertices.pop_back();
    Reject([&] { registry.UpdateMesh(mesh_handle, mesh); }, "invalid mesh update rejected");
    Check(registry.SnapshotMesh(mesh_handle).vertices[0].pos_coord.x == 4,
          "failed mesh update preserves accepted content");
    mesh = Triangle();
    AllocationFailure([&] { registry.UpdateMesh(mesh_handle, mesh); }, "mesh update allocation failure injected");
    Check(registry.SnapshotMesh(mesh_handle).vertices[0].pos_coord.x == 4,
          "allocation failure preserves accepted mesh content");
    auto snapshot = registry.SnapshotMesh(mesh_handle);
    snapshot.vertices.clear();
    Check(registry.SnapshotMesh(mesh_handle).vertices.size() == 3, "snapshot copy does not expose registry storage");

    auto texture = Texture();
    const auto texture_handle = registry.CreateTextureArray(texture);
    texture.pixels[0] = 99;
    texture.pixels.clear();
    Check(registry.SnapshotTextureArray(texture_handle).pixels[0] == 0,
          "texture create owns snapshot before caller mutation/destruction");
    texture = Texture();
    texture.pixels[0] = 42;
    texture.color_space = Candidate::ColorSpace::SRGB;
    registry.UpdateTextureArray(texture_handle, texture);
    texture.pixels[0] = 98;
    Check(registry.SnapshotTextureArray(texture_handle).pixels[0] == 42,
          "texture update owns snapshot before caller mutation");
    Check(registry.SnapshotTextureArray(texture_handle).color_space == Candidate::ColorSpace::SRGB,
          "texture snapshot retains explicit color space");
    texture.pixels.pop_back();
    Reject([&] { registry.UpdateTextureArray(texture_handle, texture); }, "invalid texture update rejected");
    Check(registry.SnapshotTextureArray(texture_handle).pixels[0] == 42,
          "failed texture update preserves accepted content");
    texture = Texture();
    AllocationFailure([&] { registry.UpdateTextureArray(texture_handle, texture); },
                      "texture update allocation failure injected");
    Check(registry.SnapshotTextureArray(texture_handle).pixels[0] == 42,
          "allocation failure preserves accepted texture content");

    registry.ValidateDraw(mesh_handle, texture_handle);
    Check(true, "valid mesh and texture pair accepted");
    const auto one_layer = registry.CreateTextureArray(Texture(1));
    Reject([&] { registry.ValidateDraw(mesh_handle, one_layer); }, "draw checks bound texture layer count");
    registry.UpdateMesh(mesh_handle, {});
    Check(registry.SnapshotMesh(mesh_handle).vertices.empty(), "empty update replaces old geometry");
    registry.ValidateDraw(mesh_handle, one_layer);
    Check(true, "empty mesh draw pair accepted");

    Reject([&] { (void)other.SnapshotMesh(mesh_handle); }, "cross renderer mesh rejected");
    Reject([&] { other.DestroyTextureArray(texture_handle); }, "cross renderer texture rejected");
    Reject([&] { registry.DestroyMesh({}); }, "default invalid handle rejected");
    Reject([&] { registry.ValidateDraw(mesh_handle, {}); }, "default invalid draw texture rejected");
    registry.DestroyMesh(mesh_handle);
    Reject([&] { (void)registry.SnapshotMesh(mesh_handle); }, "destroy invalidates mesh immediately");
    Reject([&] { registry.DestroyMesh(mesh_handle); }, "double mesh destroy rejected");
    const auto replacement_mesh = registry.CreateMesh(Triangle());
    Check(replacement_mesh != mesh_handle, "mesh slot reuse advances generation");
    Reject([&] { registry.UpdateMesh(mesh_handle, Triangle()); }, "old mesh handle cannot update reused slot");
    registry.DestroyTextureArray(texture_handle);
    Reject([&] { (void)registry.SnapshotTextureArray(texture_handle); }, "destroy invalidates texture immediately");
    Reject([&] { registry.DestroyTextureArray(texture_handle); }, "double texture destroy rejected");
    const auto replacement_texture = registry.CreateTextureArray(Texture());
    Check(replacement_texture != texture_handle, "texture slot reuse advances generation");
    Reject([&] { registry.ValidateDraw(replacement_mesh, texture_handle); }, "old texture handle cannot draw reused slot");
    Check(!replacement_mesh.IsNull() && !replacement_texture.IsNull(), "published handle tokens are nondefault");
}

void CameraAndExtent() {
    Candidate::CameraParameters camera;
    Candidate::ValidateCamera(camera);
    Check(true, "default neutral camera accepted");
    camera.forward = {0.0f, 0.0f, -2.0f};
    camera.up = {0.0f, 3.0f, 0.0f};
    Candidate::ValidateCamera(camera);
    Check(true, "candidate accepts nonnormalized nondegenerate camera basis");
    camera.forward = {0.0f, 0.0f, 0.0f};
    Reject([&] { Candidate::ValidateCamera(camera); }, "zero camera forward rejected");
    camera = {};
    camera.up = camera.forward;
    Reject([&] { Candidate::ValidateCamera(camera); }, "parallel camera basis rejected");
    camera = {};
    camera.position.x = std::numeric_limits<float>::quiet_NaN();
    Reject([&] { Candidate::ValidateCamera(camera); }, "nonfinite camera position rejected");
    camera = {};
    camera.vertical_fov_radians = std::numbers::pi_v<float>;
    Reject([&] { Candidate::ValidateCamera(camera); }, "camera FOV must be less than pi radians");
    camera = {};
    camera.near_plane = 0.0f;
    Reject([&] { Candidate::ValidateCamera(camera); }, "camera near plane must be positive");
    camera = {};
    camera.far_plane = camera.near_plane;
    Reject([&] { Candidate::ValidateCamera(camera); }, "camera far must exceed near");
    Check(Candidate::Extent2D{0, 720}.IsSuspended(), "zero width suspends extent");
    Check(Candidate::Extent2D{1280, 0}.IsSuspended(), "zero height suspends extent");
    Check(!Candidate::Extent2D{1280, 720}.IsSuspended(), "nonzero extent is active");
    Check(std::abs(Candidate::Extent2D{1280, 720}.AspectRatio() - 16.0 / 9.0) < 1e-12,
          "extent aspect ratio is framebuffer-based");
    Reject([] { (void)Candidate::Extent2D{}.AspectRatio(); }, "suspended extent does not produce a projection ratio");
}
} // namespace

int main() {
    try {
        MeshValidation();
        TextureValidation();
        RegistryOwnershipAndHandles();
        CameraAndExtent();
        std::cout << "D3D12 R1 CPU candidate: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "D3D12 R1 CPU candidate failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
