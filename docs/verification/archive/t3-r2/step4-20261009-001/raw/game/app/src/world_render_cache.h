#pragma once

#include "render_conversion.h"

#include <symocraft/world/world.h>

#include <map>
#include <type_traits>
#include <utility>

namespace SymoCraft::AppRendering {

struct MeshCacheStatistics {
    std::uint64_t created_meshes = 0;
    std::uint64_t updated_meshes = 0;
    std::uint64_t upload_bytes = 0;
};

namespace Detail {
MeshCacheStatistics AfterMeshAcceptance(MeshCacheStatistics previous,
                                        std::size_t vertex_count, bool creation);

struct MeshIdentityLess {
    bool operator()(const World::MeshIdentity& left, const World::MeshIdentity& right) const noexcept {
        if (left.world != right.world) return left.world < right.world;
        if (left.chunk.x != right.chunk.x) return left.chunk.x < right.chunk.x;
        return left.chunk.y < right.chunk.y;
    }
};
} // namespace Detail

// The port and renderer must outlive this cache. Release is explicit, never inferred from a visit.
template <class MeshPort>
class WorldRenderCache final {
public:
    using Handle = typename MeshPort::MeshHandle;
    static_assert(std::is_trivially_copyable_v<Handle>);

    struct Entry {
        Handle handle{};
        World::Revision accepted_revision = 0;
        std::size_t vertex_count = 0;
    };

    explicit WorldRenderCache(MeshPort& port) : port_(port) {}
    WorldRenderCache(const WorldRenderCache&) = delete;
    WorldRenderCache& operator=(const WorldRenderCache&) = delete;
    WorldRenderCache(WorldRenderCache&&) = delete;
    WorldRenderCache& operator=(WorldRenderCache&&) = delete;

    bool Accept(const World::WorldMeshRecord& record) {
        auto found = entries_.find(record.identity);
        if (found != entries_.end() && found->second.accepted_revision == record.revision)
            return false;

        // Copy while the World callback borrow is valid; no caller span survives this call.
        const auto owned = CopyRendererMesh(record.vertices);
        const auto next_statistics = Detail::AfterMeshAcceptance(
            statistics_, owned.vertices.size(), found == entries_.end());
        if (found != entries_.end()) {
            port_.UpdateMesh(found->second.handle, owned);
            found->second.accepted_revision = record.revision;
            found->second.vertex_count = owned.vertices.size();
        } else {
            // Allocate bookkeeping first, so allocation failure cannot orphan an accepted resource.
            const auto inserted = entries_.try_emplace(record.identity).first;
            try {
                inserted->second.handle = port_.CreateMesh(owned);
                inserted->second.accepted_revision = record.revision;
                inserted->second.vertex_count = owned.vertices.size();
            } catch (...) {
                entries_.erase(inserted);
                throw;
            }
        }
        statistics_ = next_statistics;
        return true;
    }

    MeshCacheStatistics Synchronize(const World::VoxelWorld& world) {
        world.VisitMeshes([this](const auto& record) { Accept(record); });
        return TakeStatistics();
    }

    [[nodiscard]] const Entry* Find(World::MeshIdentity identity) const noexcept {
        const auto found = entries_.find(identity);
        return found == entries_.end() ? nullptr : &found->second;
    }

    [[nodiscard]] std::size_t Size() const noexcept { return entries_.size(); }
    [[nodiscard]] MeshCacheStatistics PendingStatistics() const noexcept { return statistics_; }

    // Failed visits keep statistics for any earlier successful per-record operations.
    MeshCacheStatistics TakeStatistics() noexcept { return std::exchange(statistics_, {}); }

    void AppendDraws(Rendering::TextureArrayHandle texture_array,
                     std::vector<Rendering::MeshDraw>& draws) const {
        for (const auto& [identity, entry] : entries_) {
            (void)identity;
            if (entry.vertex_count != 0)
                draws.push_back({entry.handle, texture_array, glm::mat4{1.0f}});
        }
    }

    bool ReleaseMesh(World::MeshIdentity identity) {
        const auto found = entries_.find(identity);
        if (found == entries_.end()) return false;
        port_.DestroyMesh(found->second.handle);
        entries_.erase(found);
        return true;
    }

    void ReleaseWorld(World::WorldId identity) {
        for (auto entry = entries_.begin(); entry != entries_.end();) {
            if (entry->first.world != identity) { ++entry; continue; }
            port_.DestroyMesh(entry->second.handle);
            entry = entries_.erase(entry);
        }
    }

    void ReleaseAll() {
        for (auto entry = entries_.begin(); entry != entries_.end();) {
            port_.DestroyMesh(entry->second.handle);
            entry = entries_.erase(entry);
        }
    }

private:
    MeshPort& port_;
    std::map<World::MeshIdentity, Entry, Detail::MeshIdentityLess> entries_;
    MeshCacheStatistics statistics_;
};

} // namespace SymoCraft::AppRendering
