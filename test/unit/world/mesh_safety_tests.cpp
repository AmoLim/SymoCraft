#include "world_test_access.h"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    using namespace SymoCraft;
    using namespace SymoCraft::World;
    void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    void Set(VoxelWorld& world, BlockCoord position, BlockId id)
    {
        Require(world.TryEdit({EditOperation::Set, position, id}).Accepted(), "Fixture insertion failed");
    }
    void Remove(VoxelWorld& world, BlockCoord position)
    {
        Require(world.TryEdit({EditOperation::Remove, position}).Accepted(), "Fixture removal failed");
    }
    std::size_t Count(VoxelWorld& world, ChunkCoord coordinate)
    {
        std::size_t count = SIZE_MAX;
        world.VisitMeshes([&](const WorldMeshRecord& record) { if (record.identity.chunk == coordinate) count = record.vertices.size(); });
        Require(count != SIZE_MAX, "Published chunk was not visited");
        return count;
    }
    void CheckMissingNeighbors(VoxelWorld& world)
    {
        const std::array<BlockCoord, 8> outside{{{-49,10,0},{48,10,0},{0,10,-49},{0,10,48},
            {0,-1,0},{0,256,0},{48,256,48},{-49,-1,-49}}};
        for (const auto& position : outside) {
            Require(world.QueryBlock(position).status == BlockQueryStatus::OutsideWorld, "Missing neighbor read did not report OutsideWorld");
            Require(world.TryEdit({EditOperation::Set,position,2}).status == EditStatus::Rejected, "Out-of-world insertion accepted");
            Require(world.TryEdit({EditOperation::Remove,position}).status == EditStatus::Rejected, "Out-of-world removal accepted");
        }
        Require(world.QueryBlock({0,10,0}).block == BlockConstants::AIR_BLOCK, "Rejected writes changed local chunk");
    }
    void CheckNeighborWrites(VoxelWorld& world)
    {
        world.RebuildDirtyMeshes();
        const auto left_input = WorldTestAccess::Chunk(world,{0,0}).mesh_input_revision;
        const auto right_input = WorldTestAccess::Chunk(world,{1,0}).mesh_input_revision;
        Set(world,{16,64,3},2);
        Require(world.QueryBlock({16,64,3}).block.block_id == 2, "Cross-chunk write indexed wrong block");
        Require(WorldTestAccess::Chunk(world,{0,0}).mesh_input_revision == left_input + 1 &&
            WorldTestAccess::Chunk(world,{1,0}).mesh_input_revision == right_input + 1, "Boundary write did not invalidate both meshes");
        Remove(world,{16,64,3});
        Require(world.QueryBlock({16,64,3}).block == BlockConstants::AIR_BLOCK, "Boundary removal did not create air");
        Require(world.TryEdit({EditOperation::Set,{16,256,3},2}).status == EditStatus::Rejected, "Vertical bounds not checked");
    }
    void CheckFaceCounts(VoxelWorld& world)
    {
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == 0, "Air chunk generated vertices");
        Set(world,{8,80,8},2);
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == 36, "Isolated cube must contain 36 vertices");
        Set(world,{9,80,8},2);
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == 60, "Adjacent cubes did not cull shared face");
        Remove(world,{8,80,8}); Remove(world,{9,80,8});
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == 0, "Regeneration retained stale vertices");
    }
    void CheckLargeMesh(VoxelWorld& world)
    {
        std::size_t blocks = 0;
        for (int y = 1; y < k_chunk_height - 1; y += 2)
            for (int x = 1; x < k_chunk_length - 1; x += 2)
                for (int z = 1; z < k_chunk_width - 1; z += 2) { Set(world,{x,y,z},2); ++blocks; }
        const auto expected = blocks * 36;
        Require(expected > UINT16_MAX, "Large mesh fixture did not cross old 16-bit limit");
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == expected, "Mesh vertex count overflowed or lost faces");
        Require(world.RebuildDirtyMeshes() == 0 && Count(world,{0,0}) == expected, "Repeated meshing retained stale data");
        Remove(world,{1,1,1}); Set(world,{1,1,1},2);
        world.RebuildDirtyMeshes();
        Require(Count(world,{0,0}) == expected, "Rebuilt large mesh lost ownership or faces");
    }
}

int main(int argc, char** argv)
{
    using namespace SymoCraft;
    static_assert(!std::is_copy_constructible_v<World::Detail::Chunk>);
    static_assert(!std::is_copy_assignable_v<World::Detail::Chunk>);
    static_assert(!std::is_move_constructible_v<World::Detail::Chunk>);
    static_assert(!std::is_move_assignable_v<World::Detail::Chunk>);
    try {
        Require(argc == 2, "Expected real block configuration path");
        std::ifstream input(argv[1],std::ios::binary);
        Require(static_cast<bool>(input),"Cannot read block configuration");
        const std::string text{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
        const auto definition = World::BlockDefinition::FromConfig(text);
        auto world = World::VoxelWorld::Create(definition,{424242,2,false});
        World::WorldTestAccess::ResetToAir(*world);
        CheckMissingNeighbors(*world);
        CheckNeighborWrites(*world);
        CheckFaceCounts(*world);
        CheckLargeMesh(*world);
        const auto identity = world->Identity();
        world.reset();
        world.reset();
        world = World::VoxelWorld::Create(definition,{424242,2,false});
        Require(world->Identity() != identity && world->ChunkCount() == 25, "Recreation reused identity or retained freed entries");
        World::WorldTestAccess::ResetToAir(*world);
        Require(world->QueryBlock({0,20,0}).block == BlockConstants::AIR_BLOCK,"Recreated world inherited stale storage");
        std::cout << "Mesh safety, immutable chunk lifetime and explicit-world ownership tests passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
