#include "world_render_cache.h"

#include <symocraft/assets/image.h>

#include <bit>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>

namespace AppRendering = SymoCraft::AppRendering;
namespace World = SymoCraft::World;

namespace {
std::size_t checks = 0;
void Check(bool condition, std::string_view message) {
    ++checks;
    if (!condition) throw std::runtime_error(std::string{message});
}
template <class Operation>
void Fails(Operation&& operation) {
    bool failed = false;
    try { operation(); } catch (const std::runtime_error&) { failed = true; }
    Check(failed, "injected mesh operation failure must propagate");
}

std::uint64_t MeshDigest(const SymoCraft::MeshData& mesh) {
    std::uint64_t digest = 14695981039346656037ull;
    const auto append = [&](std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            digest ^= (value >> shift) & 0xffu;
            digest *= 1099511628211ull;
        }
    };
    for (const auto& vertex : mesh.vertices) {
        for (int axis = 0; axis < 3; ++axis)
            append(static_cast<std::uint32_t>(vertex.pos_coord[axis]));
        for (int axis = 0; axis < 3; ++axis)
            append(std::bit_cast<std::uint32_t>(vertex.tex_coord[axis]));
        append(std::bit_cast<std::uint32_t>(vertex.normal));
    }
    return digest;
}

SymoCraft::MeshData Triangle(int x) {
    return {{{{x, 0, -3}, {0.25f, 0.75f, 2}, 0},
             {{x + 1, 0, -3}, {1.25f, 0.75f, 2}, 0},
             {{x, 1, -3}, {0.25f, 1.75f, 2}, 0}}};
}

struct MeshPort {
    using MeshHandle = std::uint64_t;
    std::map<MeshHandle, SymoCraft::MeshData> meshes;
    MeshHandle next_handle = 1;
    std::size_t creates = 0, updates = 0, destroys = 0, create_attempts = 0;
    std::size_t fail_create_attempt = 0;
    bool fail_create = false, fail_update = false, fail_destroy = false;
    const SymoCraft::BlockVertex3D* last_borrow = nullptr;

    MeshHandle CreateMesh(const SymoCraft::MeshData& mesh) {
        ++create_attempts;
        if (fail_create || create_attempts == fail_create_attempt)
            throw std::runtime_error("injected mesh create failure");
        const auto handle = next_handle++;
        meshes.emplace(handle, mesh);
        last_borrow = mesh.vertices.data();
        ++creates;
        return handle;
    }
    void UpdateMesh(MeshHandle handle, const SymoCraft::MeshData& mesh) {
        if (fail_update) throw std::runtime_error("injected mesh update failure");
        meshes.at(handle) = mesh;
        last_borrow = mesh.vertices.data();
        ++updates;
    }
    void DestroyMesh(MeshHandle handle) {
        if (fail_destroy) throw std::runtime_error("injected mesh destroy failure");
        if (meshes.erase(handle) != 1) throw std::runtime_error("unknown fake mesh handle");
        ++destroys;
    }
};

void PublicationAcceptance() {
    MeshPort port;
    AppRendering::WorldRenderCache cache{port};
    const World::MeshIdentity identity{41, {-1, 2}};
    auto source = Triangle(-16);
    World::WorldMeshRecord record{identity, 4, 9, source.vertices};

    port.fail_create = true;
    Fails([&] { cache.Accept(record); });
    Check(cache.Size() == 0 && !cache.Find(identity), "failed create publishes no cache entry");
    Check(cache.TakeStatistics().created_meshes == 0, "failed create counts no accepted upload");
    port.fail_create = false;
    Check(cache.Accept(record), "first publication is accepted after create retry");
    const auto handle = cache.Find(identity)->handle;
    Check(cache.Find(identity)->accepted_revision == 4, "cache tracks publication, never mesh input revision");
    Check(port.last_borrow != source.vertices.data(), "mesh port receives owned storage, not the World borrow");
    Check(port.meshes.at(handle).vertices[0].pos_coord.x == -16, "world-space negative position is not translated twice");
    Check(port.meshes.at(handle).vertices[0].tex_coord == glm::vec3{0.25f, 0.25f, 2},
          "UV V is flipped exactly once while layer and U are unchanged");
    const auto accepted_digest = MeshDigest(port.meshes.at(handle));
    source.vertices[0].pos_coord.x = 999;
    Check(MeshDigest(port.meshes.at(handle)) == accepted_digest, "accepted source storage is independent of later mutation");

    record.mesh_input_revision = 12;
    Check(!cache.Accept(record) && port.creates == 1 && port.updates == 0,
          "dirty input with unchanged published revision does not re-upload");
    Check(cache.Find(identity)->accepted_revision == 4,
          "unchanged publication is not retagged with a newer input revision");
    const auto created = cache.TakeStatistics();
    Check(created.created_meshes == 1 && created.updated_meshes == 0 &&
          created.upload_bytes == 3 * sizeof(SymoCraft::BlockVertex3D),
          "creation reports exact accepted vertex upload bytes");
    Check(cache.TakeStatistics().upload_bytes == 0, "taking upload statistics drains them once");

    source = Triangle(-15);
    record.vertices = source.vertices;
    record.revision = 12;
    port.fail_update = true;
    Fails([&] { cache.Accept(record); });
    Check(cache.Find(identity)->handle == handle && cache.Find(identity)->accepted_revision == 4,
          "failed update preserves the previously accepted handle and revision");
    Check(MeshDigest(port.meshes.at(handle)) == accepted_digest, "failed update preserves old accepted geometry");
    Check(cache.PendingStatistics().upload_bytes == 0, "failed update reports no accepted upload");
    port.fail_update = false;
    Check(cache.Accept(record) && cache.Find(identity)->accepted_revision == 12,
          "same publication retries successfully after update failure");
    Check(port.meshes.at(handle).vertices[0].tex_coord.y == 0.25f,
          "update starts from original World UVs, not the already flipped accepted mesh");
    source.vertices.clear();
    record.vertices = source.vertices;
    record.revision = 13;
    Check(cache.Accept(record) && port.meshes.at(handle).vertices.empty(),
          "published empty mesh clears previously accepted geometry");
    Check(cache.Find(identity)->handle == handle && cache.Find(identity)->vertex_count == 0,
          "empty publication retains stable logical identity and handle");
    const auto updated = cache.TakeStatistics();
    Check(updated.updated_meshes == 2 && updated.upload_bytes == 3 * sizeof(SymoCraft::BlockVertex3D),
          "empty update is accepted with zero vertex upload bytes");

    source = Triangle(-15);
    record.vertices = source.vertices;
    record.identity.world = 42;
    record.revision = 1;
    Check(cache.Accept(record) && cache.Find(record.identity)->handle != handle,
          "equal chunk coordinates in different worlds cannot alias mesh handles");
    record.identity.chunk = {2, -1};
    Check(cache.Accept(record) && cache.Size() == 3,
          "chunk x and z are independently part of the stable identity");
    Check(cache.Size() == 3, "omitting callbacks by itself does not delete accepted geometry");

    port.fail_destroy = true;
    Fails([&] { cache.ReleaseWorld(41); });
    Check(cache.Find(identity) && port.meshes.contains(handle),
          "failed explicit release retains cache state for retry");
    port.fail_destroy = false;
    cache.ReleaseWorld(41);
    Check(!cache.Find(identity) && cache.Size() == 2 && !port.meshes.contains(handle),
          "explicit world release deletes only that world's resources");
    Check(cache.ReleaseMesh(record.identity) && !cache.ReleaseMesh(record.identity),
          "explicit single-mesh unload is idempotent when the identity is absent");
    cache.ReleaseAll();
    cache.ReleaseAll();
    Check(cache.Size() == 0 && port.meshes.empty() && port.destroys == 3,
          "explicit complete release drains all remaining mesh ownership");
}

void BorrowLifetime() {
    MeshPort port;
    AppRendering::WorldRenderCache cache{port};
    const World::MeshIdentity identity{73, {3, -2}};
    std::uint64_t expected = 0;
    {
        const auto source = Triangle(48);
        expected = MeshDigest(AppRendering::CopyRendererMesh(source.vertices));
        const World::WorldMeshRecord record{identity, 0, 7, source.vertices};
        cache.Accept(record);
    }
    const auto* accepted = cache.Find(identity);
    Check(accepted && accepted->accepted_revision == 0,
          "first published revision zero is distinct from an absent cache entry");
    Check(MeshDigest(port.meshes.at(accepted->handle)) == expected,
          "callback source and record may both die after synchronous acceptance");
    cache.ReleaseAll();
}

void DrawAssembly() {
    struct DrawPort {
        using MeshHandle = SymoCraft::Rendering::MeshHandle;
        MeshHandle CreateMesh(const SymoCraft::MeshData&) { return {}; }
        void UpdateMesh(MeshHandle, const SymoCraft::MeshData&) {}
        void DestroyMesh(MeshHandle) {}
    } port;
    AppRendering::WorldRenderCache cache{port};
    const auto mesh = Triangle(-16);
    cache.Accept({{11, {-1, 2}}, 1, 1, mesh.vertices});
    cache.Accept({{11, {2, -1}}, 1, 1, {}});
    std::vector<SymoCraft::Rendering::MeshDraw> draws;
    cache.AppendDraws({}, draws);
    Check(draws.size() == 1, "draw assembly excludes accepted empty geometry without deleting its identity");
    Check(cache.Size() == 2 && draws.front().object_transform == glm::mat4{1.0f},
          "draw assembly keeps the identity model for already-world-space chunk vertices");
    cache.AppendDraws({}, draws);
    Check(draws.size() == 2, "draw assembly appends to caller-owned storage without clearing earlier draws");
    cache.ReleaseAll();
}

void RealWorldAcceptance(const std::filesystem::path& atlas_path) {
    const auto config_path = atlas_path.parent_path().parent_path() / "configs" / "blockFormats.yaml";
    const auto bytes = SymoCraft::Assets::ReadBytes(config_path);
    const auto definition = World::BlockDefinition::FromConfig(
        std::string_view{reinterpret_cast<const char*>(bytes.data()), bytes.size()});
    auto world = World::VoxelWorld::Create(definition, {424242, 2, false});
    const auto initial_digest = world->Digest();
    MeshPort port;
    AppRendering::WorldRenderCache cache{port};
    auto prior = Triangle(32);
    const World::MeshIdentity omitted{world->Identity(), {2, 2}};
    cache.Accept({omitted, 1, 1, prior.vertices});
    cache.TakeStatistics();
    Check(cache.Synchronize(*world).created_meshes == 0 && cache.Find(omitted),
          "actual World visit with no published records is not a deletion notification");
    cache.ReleaseMesh(omitted);

    Check(world->RebuildDirtyMeshes() == 9, "real fixture first publishes its nine non-fringe meshes");
    port.fail_create_attempt = port.create_attempts + 2;
    Fails([&] { cache.Synchronize(*world); });
    Check(cache.Size() == 1 && cache.PendingStatistics().created_meshes == 1,
          "per-record create failure preserves the earlier accepted publication and statistics");
    const auto first = cache.Synchronize(*world);
    Check(first.created_meshes == 9 && first.updated_meshes == 0 && cache.Size() == 9,
          "retry accepts remaining records without repeating the first successful create");
    Check(world->Digest() == initial_digest, "cache consumption leaves real World content and digest unchanged");
    const auto idle = cache.Synchronize(*world);
    Check(idle.created_meshes == 0 && idle.updated_meshes == 0 && idle.upload_bytes == 0,
          "static real world uploads no accepted geometry a second time");

    const World::MeshIdentity target{world->Identity(), {-1, -1}};
    const auto old_revision = cache.Find(target)->accepted_revision;
    const auto handle = cache.Find(target)->handle;
    const auto old_digest = MeshDigest(port.meshes.at(handle));
    Check(world->TryEdit({World::EditOperation::Set, {-1, 200, -1}, 5}).status == World::EditStatus::Changed,
          "real negative chunk-corner edit is accepted");
    const auto dirty = cache.Synchronize(*world);
    Check(dirty.updated_meshes == 0 && cache.Find(target)->accepted_revision == old_revision,
          "dirty unrebuilt World records remain valid old publications without uploads");
    Check(world->RebuildDirtyMeshes() == 3, "real edit rebuilds only owner and cardinal boundary neighbors");
    port.fail_update = true;
    Fails([&] { cache.Synchronize(*world); });
    Check(cache.Find(target)->accepted_revision == old_revision && MeshDigest(port.meshes.at(handle)) == old_digest,
          "real failed publication acceptance leaves old accepted geometry and revision intact");
    port.fail_update = false;
    const auto retry = cache.Synchronize(*world);
    Check(retry.updated_meshes == 3 && retry.created_meshes == 0 &&
          cache.Find(target)->accepted_revision > old_revision,
          "real retry accepts exactly the three changed publications");
    Check(cache.Synchronize(*world).upload_bytes == 0 && world->RebuildDirtyMeshes() == 0,
          "post-edit idle state repeats neither World rebuild nor renderer upload");
    std::uint64_t expected = 0;
    world->VisitMeshes([&](const auto& record) {
        if (record.identity == target)
            expected = MeshDigest(AppRendering::CopyRendererMesh(record.vertices));
    });
    const auto world_id = world->Identity();
    world.reset();
    Check(MeshDigest(port.meshes.at(handle)) == expected,
          "accepted real mesh survives destruction of its source World");
    cache.ReleaseWorld(world_id);
    Check(cache.Size() == 0 && port.meshes.empty(), "explicit real World release drains only its accepted meshes");
}

void StatisticsOverflow() {
    const auto maximum = std::numeric_limits<std::uint64_t>::max();
    Fails([&] { AppRendering::Detail::AfterMeshAcceptance({maximum, 0, 0}, 0, true); });
    Fails([&] { AppRendering::Detail::AfterMeshAcceptance({0, maximum, 0}, 0, false); });
    Fails([&] { AppRendering::Detail::AfterMeshAcceptance({0, 0, maximum}, 1, true); });
    Fails([&] { AppRendering::Detail::AfterMeshAcceptance({}, std::numeric_limits<std::size_t>::max(), true); });
    const auto empty = AppRendering::Detail::AfterMeshAcceptance({0, 0, maximum}, 0, false);
    Check(empty.updated_meshes == 1 && empty.upload_bytes == maximum,
          "accepted empty updates do not fabricate bytes or overflow a full byte counter");
}
} // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::invalid_argument("provide the production texture_atlas.png path");
        PublicationAcceptance();
        BorrowLifetime();
        DrawAssembly();
        RealWorldAcceptance(argv[1]);
        StatisticsOverflow();
        std::cout << "Application World render cache: " << checks << " CPU checks passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Application World render cache failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
