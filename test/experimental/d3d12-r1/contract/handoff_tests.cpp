#include "handoff_candidates.h"
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/world/constants.h>

#include <functional>
#include <iostream>
#include <string>
#include <string_view>

namespace Candidate = SymoCraft::Experimental::D3D12R1::Candidate;
namespace Handoff = SymoCraft::Experimental::D3D12R1::Handoff;

namespace {
int checks = 0;
void Check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string{message});
}
template <class Function>
void Reject(Function&& operation, std::string_view message) {
    bool rejected = false;
    try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, message);
}
bool Near(double a, double b, double epsilon = 1e-5) { return std::abs(a - b) <= epsilon; }
bool MatrixNear(const glm::dmat4& a, const glm::mat4& b) {
    for (int column = 0; column < 4; ++column)
        for (int row = 0; row < 4; ++row)
            if (!Near(a[column][row], b[column][row])) return false;
    return true;
}
glm::dvec3 NDC(const glm::dmat4& matrix, glm::dvec3 point) {
    const auto clip = matrix * glm::dvec4{point, 1.0};
    return glm::dvec3{clip} / clip.w;
}

void CameraTransfer() {
    SymoCraft::ECS::Registry registry;
    registry.RegisterComponent<SymoCraft::Transform>("Transform");
    SymoCraft::Camera camera{registry, 1280, 720, {12.5f, 4.0f, -9.5f}};
    SymoCraft::TransformSystem::Update(registry);
    const Candidate::Extent2D extent{1280, 720};
    auto neutral = Handoff::FromLegacyCamera(camera);
    Check(neutral.position == camera.GetCameraPos(), "camera world position copied");
    Check(neutral.forward == camera.GetCameraFront() && neutral.up == camera.GetCameraUp(),
          "camera orientation copied without handedness change");
    Check(Near(neutral.vertical_fov_radians, std::numbers::pi / 4), "legacy 45 degree vertical FOV converted to radians");
    Check(neutral.near_plane == 0.1f && neutral.far_plane == 2000.0f, "legacy near/far preserved, not candidate defaults");
    Check(MatrixNear(Handoff::ViewRH(neutral), camera.GetCameraViewMat()), "RH view agrees with production Camera");
    Check(MatrixNear(Handoff::ProjectionRH_NO(neutral, extent), camera.GetCameraProjMat(1280.0f / 720.0f)),
          "explicit RH negative-one-to-one projection agrees with production Camera");
    camera.SetYaw(35.0f);
    camera.SetPitch(-22.0f);
    SymoCraft::TransformSystem::Update(registry);
    neutral = Handoff::FromLegacyCamera(camera);
    Check(MatrixNear(Handoff::ViewRH(neutral), camera.GetCameraViewMat()), "rotated translated view agrees with production Camera");
    const auto eye = Handoff::ViewRH(neutral) * glm::dvec4{glm::dvec3{neutral.position}, 1.0};
    Check(glm::length(glm::dvec3{eye}) < 1e-9, "camera eye maps to view origin");
    const auto in_front = Handoff::ViewRH(neutral) *
        glm::dvec4{glm::dvec3{neutral.position} + glm::dvec3{neutral.forward}, 1.0};
    Check(in_front.z < 0 && Near(in_front.x, 0) && Near(in_front.y, 0), "forward maps to RH negative view Z");
    camera.Scroll(1000);
    Check(Near(Handoff::FromLegacyCamera(camera).vertical_fov_radians, std::numbers::pi / 180), "minimum one degree zoom preserved");
    camera.Scroll(-1000);
    Check(Near(Handoff::FromLegacyCamera(camera).vertical_fov_radians, std::numbers::pi / 4), "maximum 45 degree zoom preserved");

    Candidate::CameraParameters axis;
    axis.vertical_fov_radians = neutral.vertical_fov_radians;
    axis.near_plane = neutral.near_plane;
    axis.far_plane = neutral.far_plane;
    const auto no = Handoff::ProjectionRH_NO(axis, extent);
    const auto zo = Handoff::ProjectionRH_ZO(axis, extent);
    Check(Near(NDC(no, {0, 0, -axis.near_plane}).z, -1, 1e-12), "NO near depth is negative one");
    Check(Near(NDC(no, {0, 0, -axis.far_plane}).z, 1, 1e-12), "NO far depth is one");
    Check(Near(NDC(zo, {0, 0, -axis.near_plane}).z, 0, 1e-12), "ZO near depth is zero");
    Check(Near(NDC(zo, {0, 0, -axis.far_plane}).z, 1, 1e-12), "ZO far depth is one");
    const glm::dvec3 sample{0.3, 0.2, -4.0};
    const auto old_ndc = NDC(no, sample), new_ndc = NDC(zo, sample);
    Check(Near(old_ndc.x, new_ndc.x, 1e-12) && Near(old_ndc.y, new_ndc.y, 1e-12), "depth conversion preserves projected XY");
    Check(Near(new_ndc.z, (old_ndc.z + 1) * 0.5, 1e-12), "ZO is the NO depth remap, not a handedness change");
    const auto tan_half = std::tan(static_cast<double>(axis.vertical_fov_radians) * 0.5);
    Check(Near(NDC(zo, {0, tan_half, -1}).y, 1, 1e-12), "vertical FOV top plane maps to positive one");
    Check(Near(NDC(zo, {tan_half * extent.AspectRatio(), 0, -1}).x, 1, 1e-12), "framebuffer aspect controls horizontal frustum");
    const auto portrait = Handoff::ProjectionRH_ZO(axis, {720, 1280});
    Check(portrait[0][0] > zo[0][0] && portrait[1][1] == zo[1][1], "resize changes horizontal scale, not vertical FOV");
    Reject([&] { (void)Handoff::ProjectionRH_ZO(axis, {0, 720}); }, "zero framebuffer does not create a projection");
    axis.position.x = std::numeric_limits<float>::max();
    const auto large_eye_view = Handoff::ViewRH(axis);
    Check(std::isfinite(large_eye_view[3][0]) && large_eye_view[0][0] == 1.0,
          "finite large eye does not erase the RH orientation basis");
}

SymoCraft::Assets::Image SyntheticAtlas(int channels = 4) {
    SymoCraft::Assets::Image image{4, 4, channels, {}};
    image.pixels.resize(static_cast<std::size_t>(4 * 4 * channels));
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 4; ++x)
            for (int channel = 0; channel < channels; ++channel)
                image.pixels[static_cast<std::size_t>((y * 4 + x) * channels + channel)] =
                    static_cast<std::uint8_t>(y * 40 + x * 5 + channel);
    return image;
}

void AtlasTransfer(const std::filesystem::path& atlas_path) {
    auto image = SyntheticAtlas();
    auto layers = Handoff::FromLegacyAtlas(image, 2);
    Candidate::ValidateTextureArray(layers);
    Check(layers.layers == 4 && layers.width == 2 && layers.height == 2, "square atlas tiles become texture layers");
    Check(layers.pixels[0] == image.pixels[32], "layer zero is original PNG bottom-left tile, stored top row first");
    Check(layers.pixels[16] == image.pixels[40], "layer one progresses right across original PNG bottom row");
    Check(layers.pixels[32] == image.pixels[0], "next layer row progresses upward in original PNG");
    Check(layers.pixels[8] == image.pixels[48], "a layer's storage progresses top to bottom without second flip");
    Check(layers.color_space == Candidate::ColorSpace::Linear, "legacy RGBA8 interpreted without new sRGB conversion");
    Check(Handoff::LegacyUVToTopLeftSampling({1, 1}) == glm::vec2{1, 0}, "legacy top-right UV samples top-right top-origin pixel");
    Check(Handoff::LegacyUVToTopLeftSampling({0, 0}) == glm::vec2{0, 1}, "legacy bottom-left UV samples bottom-left top-origin pixel");
    Check(Handoff::LegacyUVToTopLeftSampling({-0.25f, 1.25f}) == glm::vec2{-0.25f, -0.25f}, "sampling normalization preserves repeat coordinates without clamping");
    image.pixels[32] = 255;
    Check(layers.pixels[0] != image.pixels[32], "atlas conversion returns owned pixel storage");
    image = SyntheticAtlas(3);
    layers = Handoff::FromLegacyAtlas(image, 2);
    Check(layers.pixels[3] == 255 && layers.pixels[0] == image.pixels[24], "legacy RGB converted to opaque RGBA without changing RGB");
    image = SyntheticAtlas();
    image.channels = 2;
    Reject([&] { (void)Handoff::FromLegacyAtlas(image, 2); }, "unsupported atlas channels rejected");
    image = SyntheticAtlas(); image.width = 3;
    Reject([&] { (void)Handoff::FromLegacyAtlas(image, 2); }, "partial atlas tile rejected");
    image = SyntheticAtlas(); image.pixels.pop_back();
    Reject([&] { (void)Handoff::FromLegacyAtlas(image, 2); }, "short image storage rejected");
    image = SyntheticAtlas();
    Reject([&] { (void)Handoff::FromLegacyAtlas(image, 0); }, "zero tile size rejected");

    // Decode the actual production asset through its CPU-only production decoder.
    const auto production_image = SymoCraft::Assets::DecodeImage(atlas_path, false);
    const auto legacy_flipped_image = SymoCraft::Assets::DecodeImage(atlas_path, true);
    const auto production_layers = Handoff::FromLegacyAtlas(production_image);
    Check(production_image.width == 512 && production_image.height == 512 && production_image.channels == 4,
          "production atlas is the expected 512x512 RGBA workload");
    Check(production_layers.layers == 64 && production_layers.pixels.size() == 1048576, "all 64 production 64x64 layers retained");
    bool all_bytes_match = true;
    std::size_t alpha_cutouts = 0;
    for (std::uint32_t layer = 0; layer < 64; ++layer)
        for (std::uint32_t y = 0; y < 64; ++y)
            for (std::uint32_t x = 0; x < 64; ++x) {
                const auto source_x = (layer % 8) * 64 + x;
                const auto source_y = (layer / 8) * 64 + (63 - y);
                const auto source = (static_cast<std::size_t>(source_y) * 512 + source_x) * 4;
                const auto target = ((static_cast<std::size_t>(layer) * 64 + y) * 64 + x) * 4;
                for (std::size_t channel = 0; channel < 4; ++channel)
                    all_bytes_match &= production_layers.pixels[target + channel] == legacy_flipped_image.pixels[source + channel];
                alpha_cutouts += production_layers.pixels[target + 3] < 3;
            }
    Check(all_bytes_match, "every production texel matches old GL bottom-up layer storage after one direction normalization");
    Check(alpha_cutouts != 0, "actual atlas cutout alpha bytes retained");
}

SymoCraft::MeshData Triangle(int x = 0) {
    return {{{{x, 0, 0}, {0, 0, 0}, 0}, {{x + 1, 0, 0}, {1, 0, 0}, 0}, {{x, 1, 0}, {0, 1, 0}, 0}}};
}
struct FailureRenderer {
    Candidate::RendererRegistry registry;
    bool fail = false;
    std::size_t creates = 0, updates = 0;
    Candidate::MeshHandle CreateMesh(const SymoCraft::MeshData& mesh) {
        if (fail) throw std::runtime_error("injected CPU acceptance failure");
        ++creates; return registry.CreateMesh(mesh);
    }
    void UpdateMesh(Candidate::MeshHandle handle, const SymoCraft::MeshData& mesh) {
        if (fail) throw std::runtime_error("injected CPU acceptance failure");
        ++updates; registry.UpdateMesh(handle, mesh);
    }
    void DestroyMesh(Candidate::MeshHandle handle) { registry.DestroyMesh(handle); }
};
template <class Function>
void Fail(Function&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::runtime_error&) { failed = true; }
    Check(failed, "CPU acceptance failure is propagated");
}

void MeshTransfer() {
    const SymoCraft::World::MeshIdentity identity{17, {-1, 2}};
    Handoff::OwnedMeshRecord snapshot;
    const auto visit = [&](const std::function<void(const SymoCraft::World::WorldMeshRecord&)>& callback) {
        auto mesh = Triangle(-16);
        const SymoCraft::World::WorldMeshRecord record{identity, 4, 5, mesh.vertices};
        callback(record);
        mesh.vertices[0].pos_coord.x = 999;
    };
    visit([&](const auto& record) { snapshot = Handoff::CopyMeshRecord(record); });
    Check(snapshot.mesh.vertices[0].pos_coord.x == -16, "callback mesh copied before borrowed source mutates or dies");
    Check(snapshot.revision == 4 && snapshot.mesh_input_revision == 5, "published and newer dirty input revisions stay distinct");
    Check(snapshot.identity == identity, "world and negative x/positive z chunk identity retained");

    Handoff::MeshPublicationCache cache;
    FailureRenderer renderer;
    auto mesh = Triangle(-16);
    SymoCraft::World::WorldMeshRecord record{identity, 4, 5, mesh.vertices};
    renderer.fail = true;
    Fail([&] { cache.Accept(record, renderer); });
    Check(cache.Size() == 0 && !cache.Find(identity), "failed first create publishes neither handle nor revision");
    renderer.fail = false;
    Check(cache.Accept(record, renderer), "first published mesh accepted");
    const auto handle = cache.Find(identity)->handle;
    Check(cache.Find(identity)->accepted_revision == 4, "accepted revision is published revision, not mesh input revision");
    record.mesh_input_revision = 6;
    Check(!cache.Accept(record, renderer) && renderer.creates == 1 && renderer.updates == 0,
          "dirty input with old published vertices does not re-upload or retag");
    mesh = Triangle(-15); record.vertices = mesh.vertices; record.revision = 6;
    renderer.fail = true;
    Fail([&] { cache.Accept(record, renderer); });
    Check(cache.Find(identity)->accepted_revision == 4 && cache.Find(identity)->handle == handle,
          "failed resource update preserves old accepted revision and handle");
    Check(renderer.registry.SnapshotMesh(handle).vertices[0].pos_coord.x == -16, "failed update keeps previous accepted geometry");
    renderer.fail = false;
    Check(cache.Accept(record, renderer) && cache.Find(identity)->accepted_revision == 6, "successful retry advances accepted published revision");
    mesh.vertices.clear(); record.vertices = mesh.vertices; record.revision = 7; record.mesh_input_revision = 7;
    Check(cache.Accept(record, renderer) && renderer.registry.SnapshotMesh(handle).vertices.empty(), "published empty mesh clears old geometry without losing identity");
    const SymoCraft::World::MeshIdentity another_world{18, identity.chunk};
    record.identity = another_world; record.revision = 1; record.mesh_input_revision = 1;
    Check(cache.Accept(record, renderer) && cache.Size() == 2 && cache.Find(another_world)->handle != handle,
          "same chunk coordinate in another World never aliases a handle");
    Check(cache.Size() == 2, "absence of a callback is not an unload instruction");
    cache.ReleaseWorld(identity.world, renderer);
    Check(cache.Size() == 1 && !cache.Find(identity) && cache.Find(another_world), "world shutdown releases only its explicit identity");
    Reject([&] { (void)renderer.registry.SnapshotMesh(handle); }, "released world handle is logically invalid");

    const std::array<glm::ivec3, 6> normals{{{1,0,0},{0,0,1},{-1,0,0},{0,0,-1},{0,1,0},{0,-1,0}}};
    for (std::size_t face = 0; face < normals.size(); ++face) {
        const auto start = face * 4;
        const auto a = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[start]];
        const auto b = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[start + 1]];
        const auto c = SymoCraft::BlockConstants::pos_coords[SymoCraft::BlockConstants::vertex_indices[start + 2]];
        Check(glm::cross(glm::vec3{b - a}, glm::vec3{c - a}) == glm::vec3{normals[face]},
              "production face order has outward RH winding");
    }
}

void RealWorldTransfer(const std::filesystem::path& atlas_path) {
    const auto config_path = atlas_path.parent_path().parent_path() / "configs" / "blockFormats.yaml";
    const auto config_bytes = SymoCraft::Assets::ReadBytes(config_path);
    const auto definition = SymoCraft::World::BlockDefinition::FromConfig(
        std::string_view{reinterpret_cast<const char*>(config_bytes.data()), config_bytes.size()});
    definition.ValidateTextureLayers(64);
    auto world = SymoCraft::World::VoxelWorld::Create(definition, {424242, 2, false});
    Check(world->ChunkCount() == 25, "CPU-only transfer fixture keeps real production World including fringe");
    std::size_t visits = 0;
    world->VisitMeshes([&](const auto&) { ++visits; });
    Check(visits == 0, "actual World does not visit never-published meshes");
    Check(world->RebuildDirtyMeshes() == 9, "first actual publication includes all non-fringe chunks");
    Handoff::MeshPublicationCache cache;
    FailureRenderer renderer;
    world->VisitMeshes([&](const auto& record) { cache.Accept(record, renderer); });
    Check(cache.Size() == 9 && renderer.creates == 9, "actual callback records are accepted once by stable identity");
    const SymoCraft::World::MeshIdentity target{world->Identity(), {-1, -1}};
    const auto old_revision = cache.Find(target)->accepted_revision;
    const auto target_handle = cache.Find(target)->handle;
    const auto old_mesh = renderer.registry.SnapshotMesh(target_handle);
    Check(world->TryEdit({SymoCraft::World::EditOperation::Set, {-1, 200, -1}, 5}).status ==
          SymoCraft::World::EditStatus::Changed, "real negative chunk-corner edit accepted");
    std::size_t dirty_old_records = 0;
    world->VisitMeshes([&](const auto& record) {
        dirty_old_records += record.mesh_input_revision > record.revision;
        cache.Accept(record, renderer);
    });
    Check(dirty_old_records == 3, "actual chunk-corner edit dirties owner and two cardinal neighbors");
    Check(cache.Find(target)->accepted_revision == old_revision && renderer.updates == 0,
          "actual dirty old publication does not upload or advance accepted revision");
    Check(world->RebuildDirtyMeshes() == 3, "actual rebuild publishes just affected mesh inputs");
    renderer.fail = true;
    Fail([&] {
        world->VisitMeshes([&](const auto& record) {
            if (record.identity == target) cache.Accept(record, renderer);
        });
    });
    Check(cache.Find(target)->accepted_revision == old_revision && cache.Find(target)->handle == target_handle,
          "actual rebuilt record failed acceptance preserves the renderer's old version");
    Check(renderer.registry.SnapshotMesh(target_handle).vertices.size() == old_mesh.vertices.size(),
          "actual failed acceptance preserves previous accepted mesh storage");
    renderer.fail = false;
    Handoff::OwnedMeshRecord owned;
    world->VisitMeshes([&](const auto& record) {
        cache.Accept(record, renderer);
        if (record.identity == target) owned = Handoff::CopyMeshRecord(record);
    });
    Check(renderer.updates == 3 && cache.Find(target)->accepted_revision == owned.revision &&
          owned.revision > old_revision, "actual successful retry advances only published affected versions");
    Check(world->RebuildDirtyMeshes() == 0, "unchanged actual world has no new mesh publication");
    const auto updates = renderer.updates;
    world->VisitMeshes([&](const auto& record) { cache.Accept(record, renderer); });
    Check(renderer.updates == updates, "static actual world does not re-upload geometry");
    const auto identity = world->Identity();
    const auto copied_count = owned.mesh.vertices.size();
    world.reset();
    Check(copied_count != 0 && owned.mesh.vertices.size() == copied_count && owned.identity == target,
          "callback-owned real mesh and identity survive World destruction");
    Check(renderer.registry.SnapshotMesh(target_handle).vertices.size() == copied_count,
          "accepted renderer CPU resource survives original World destruction");
    cache.ReleaseWorld(identity, renderer);
    Check(cache.Size() == 0, "explicit World shutdown drains its candidate mapping");
    Reject([&] { (void)renderer.registry.SnapshotMesh(target_handle); }, "actual world release invalidates accepted mesh handle");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("provide the unchanged production texture_atlas.png path");
        CameraTransfer();
        AtlasTransfer(argv[1]);
        MeshTransfer();
        RealWorldTransfer(argv[1]);
        std::cout << "D3D12 R1 CPU handoff candidates: " << checks << " checks passed; GPU equivalence not tested\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "D3D12 R1 CPU handoff candidate failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
