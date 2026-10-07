#include "symocraft/world/generation.h"
#include "symocraft/world/test_scene.h"
#include "symocraft/world/benchmark_workload.h"
#include "world_test_access.h"
#include "symocraft/world/constants.h"
#include <iostream>
#include <fstream>
#include <stdexcept>

namespace {
    using namespace SymoCraft;
    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    std::vector<std::array<std::uint16_t, 4>> Snapshot(const World::VoxelWorld& world)
    {
        std::vector<std::array<std::uint16_t, 4>> snapshot;
        for (const auto& chunk : World::WorldTestAccess::Chunks(world))
                for (const auto& block : chunk->blocks)
                    snapshot.push_back({block.block_id, block.lightLevel, static_cast<std::uint16_t>(block.lightColor), block.bitwise_compressed_data});
        return snapshot;
    }
    std::size_t Trees(const World::VoxelWorld& world)
    {
        std::size_t count = 0;
        for (const auto& chunk : World::WorldTestAccess::Chunks(world))
            for (const auto& block : chunk->blocks) count += block.block_id == 6 || block.block_id == 7;
        return count;
    }
    void RequireAirPlayer(const World::VoxelWorld& world, const TestScene::Pose& pose)
    {
        const auto low = glm::floor(pose.position - glm::vec3(0.275f, 0.9f, 0.275f) + glm::vec3(0.001f));
        const auto high = glm::floor(pose.position + glm::vec3(0.275f, 0.9f, 0.275f) - glm::vec3(0.001f));
        for (int x = static_cast<int>(low.x); x <= high.x; ++x)
            for (int y = static_cast<int>(low.y); y <= high.y; ++y)
                for (int z = static_cast<int>(low.z); z <= high.z; ++z)
                    Require(!world.DescribeBlock(world.QueryBlock({x, y, z}).block.block_id)->m_is_solid, "Checkpoint embeds the player in a solid block");
    }
}

int main(int argc, char* argv[])
{
    using namespace SymoCraft;
    try {
        Require(argc == 2, "Expected the real block configuration path");
        std::ifstream input(argv[1], std::ios::binary);
        Require(static_cast<bool>(input), "Cannot read block configuration");
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const auto definition = World::BlockDefinition::FromConfig(text);
        const Generation::Settings settings{424242, 3, true};
        auto world = World::VoxelWorld::Create(definition, settings);
        const auto baseline = world->Digest();
        const auto content = Snapshot(*world);
        Require(Trees(*world) > 0, "Seed fixture must exercise vegetation, not just terrain");
        world = World::VoxelWorld::Create(definition, settings);
        Require(world->Digest() == baseline && Snapshot(*world) == content, "Repeated generation retained random or block state");
        // The new factory fixes construction order; compare with the frozen old-map digest.
        Require(baseline == "fnv1a64:d88ef364525f7355", "Generated world differs from the pre-T1 production baseline");
        const auto generator = Generation::Generator(settings);
        auto first = generator.VegetationRandom(-1, 0);
        auto same = generator.VegetationRandom(-1, 0);
        auto unrelated = generator.VegetationRandom(1, 0);
        for (int i = 0; i < 100; ++i) {
            static_cast<void>(unrelated());
            Require(first() == same(), "Vegetation streams share mutable random state");
        }

        TestScene::Install(*world);
        const auto scene = world->Digest();
        Require(scene != baseline, "Regression overlay was not installed");
        Require(scene == "fnv1a64:945aefc4a381a591", "Regression scene differs from the pre-T1 production baseline");
        for (const auto& pose : TestScene::Checkpoints()) RequireAirPlayer(*world, pose);
        Require(world->QueryBlock({8, 162, 6}).block.block_id == 11 && world->QueryBlock({6, 162, 8}).block.block_id == 11, "Wall corner fixture missing");
        Require(world->QueryBlock({-6, 163, 6}).block.block_id == 8 && world->QueryBlock({-6, 162, 6}).block.block_id == 1, "Low-ceiling clearance is wrong");
        Require(world->QueryBlock({20, 164, 20}).block.block_id == 5 && world->QueryBlock({21, 164, 20}).block.block_id == 1, "Single-block landing target is not isolated");
        Require(World::TryToBlockCoord({-0.5f, 161, -0.5f}) == glm::ivec3(-1, 161, -1), "Negative coordinates use the wrong real mapping");
        for (const auto& edit : TestScene::Edits()) {
            const auto& chunk = World::WorldTestAccess::Chunk(*world, {edit.position.x / 16 - (edit.position.x % 16 < 0), edit.position.z / 16 - (edit.position.z % 16 < 0)});
            Require(!chunk.fringe, "Edit fixture outside playable chunks");
        }
        TestScene::ApplyEdits(*world);
        const auto edited = world->Digest();
        Require(edited != scene, "Edit sequence did not change world content");
        Require(edited == "fnv1a64:97eb7834bae64491", "Edited scene differs from the pre-T1 production baseline");
        for (std::size_t i = 1; i < TestScene::Edits().size(); i += 2) {
            const auto& edit = TestScene::Edits()[i];
            Require(world->QueryBlock(edit.position).block.block_id == edit.after, "Ordered boundary edit produced the wrong block");
        }
        world = World::VoxelWorld::Create(definition, settings);
        TestScene::Install(*world);
        Require(world->Digest() == scene, "Scene cannot be rebuilt after editing and clearing");
        TestScene::ApplyEdits(*world);
        Require(world->Digest() == edited, "Edit plan is not reproducible");
        Require(Benchmark::DueEdits(0.249) == 0 && Benchmark::DueEdits(0.25) == 1 && Benchmark::DueEdits(8) == 32, "Edit wall-clock schedule is wrong");
        TestScene::Install(*world);
        for (std::uint64_t i = 0; i < 64; ++i) {
            const auto edit = Benchmark::CycleEdit(i);
            Require(world->QueryBlock(edit.position).block.block_id == edit.before, "Cyclic edit precondition is wrong");
            Require(world->TryEdit({World::EditOperation::Set, edit.position, edit.after}).Accepted(), "Cyclic edit write failed");
            if (i == 15 || i == 47) Require(world->Digest() == edited, "Forward edit cycle changed");
            if (i == 31 || i == 63) Require(world->Digest() == scene, "Reverse edit cycle did not restore fixture");
        }
        world = World::VoxelWorld::Create(definition, {settings.seed + 1, settings.radius, true});
        Require(world->Digest() != baseline, "Different seed did not change world content");
        world = World::VoxelWorld::Create(definition, {settings.seed, settings.radius, false});
        Require(Trees(*world) == 0 && world->Digest() != baseline, "Vegetation configuration is not respected");
        std::cout << "generator_version=" << Generation::Version << "; seed=" << settings.seed
                  << "; radius=" << settings.radius << "; terrain=" << baseline
                  << "; scene=" << scene << "; edited=" << edited << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
