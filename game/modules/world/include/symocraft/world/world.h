#pragma once

#include <symocraft/world/block_definition.h>
#include <symocraft/world/generation.h>
#include <symocraft/scene/mesh.h>
#include <cstdint>
#include <functional>
#include <string>

namespace SymoCraft {
    inline constexpr uint16 k_chunk_length = 16, k_chunk_width = 16, k_chunk_height = 256;
    inline constexpr int max_biome_height = 145, min_biome_height = 55, sea_level = 85;
}
namespace SymoCraft::World {
    inline constexpr uint16 chunk_radius = 10;
    using BlockCoord = glm::ivec3;
    using ChunkCoord = glm::ivec2;
    using WorldId = std::uint64_t;
    using Revision = std::uint64_t;
    enum class BlockQueryStatus { Found, OutsideWorld };
    struct BlockQueryResult { BlockQueryStatus status; Block block; };
    enum class EditOperation { Set, Remove };
    struct EditRequest { EditOperation operation; BlockCoord position; BlockId id{}; };
    enum class EditStatus { Rejected, Unchanged, Changed };
    enum class EditRejection { None, OutsideWorld, UnknownBlockId };
    struct EditResult {
        EditStatus status;
        EditRejection rejection = EditRejection::None;
        bool Accepted() const noexcept { return status != EditStatus::Rejected; }
    };
    struct MeshIdentity {
        WorldId world;
        ChunkCoord chunk;
        bool operator==(const MeshIdentity&) const = default;
    };
    struct WorldMeshRecord {
        MeshIdentity identity;
        Revision revision;
        Revision mesh_input_revision;
        MeshView vertices;
    };
    std::optional<BlockCoord> TryToBlockCoord(const glm::vec3& position) noexcept;
    class VoxelWorld {
    public:
        static std::unique_ptr<VoxelWorld> Create(BlockDefinition definition,
            Generation::Settings settings, Generation::Timings* timings = nullptr);
        ~VoxelWorld();
        VoxelWorld(const VoxelWorld&) = delete;
        VoxelWorld& operator=(const VoxelWorld&) = delete;
        VoxelWorld(VoxelWorld&&) = delete;
        VoxelWorld& operator=(VoxelWorld&&) = delete;
        BlockQueryResult QueryBlock(BlockCoord position) const noexcept;
        std::optional<BlockDefinitionEntry> DescribeBlock(BlockId id) const;
        std::optional<BlockId> FindBlockId(std::string_view name) const;
        EditResult TryEdit(EditRequest request);
        std::size_t RebuildDirtyMeshes();
        // Borrow only inside the callback; modifying/re-entering this world is rejected.
        void VisitMeshes(const std::function<void(const WorldMeshRecord&)>& visitor) const;
        std::string Digest() const;
        glm::vec3 FindSpawn() const;
        Data::Value Describe() const;
        void ValidateTextureLayers(std::size_t count) const;
        std::size_t ChunkCount() const noexcept;
        WorldId Identity() const noexcept;
    private:
        struct Impl;
        explicit VoxelWorld(std::unique_ptr<Impl> impl);
        std::unique_ptr<Impl> impl_;
        friend struct WorldTestAccess;
    };
}
