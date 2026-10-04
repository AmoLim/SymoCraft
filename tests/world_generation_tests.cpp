#include "world/generation.h"
#include "world/test_scene.h"
#include "world/chunk.h"
#include "core/constants.h"
#include <iostream>
#include <stdexcept>

// Only the unused rendering endpoint is replaced. Coordinates, formats, chunks and generation are real.
namespace SymoCraft { Batch<BlockVertex3D> chunk_batch; }

namespace {
    using namespace SymoCraft;
    void Require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    std::vector<std::array<std::uint16_t, 4>> Snapshot(int radius)
    {
        std::vector<std::array<std::uint16_t, 4>> snapshot;
        for (int x = -radius; x <= radius; ++x)
            for (int z = -radius; z <= radius; ++z)
                for (const auto& block : ChunkManager::GetChunk(glm::ivec2{x, z})->m_local_blocks)
                    snapshot.push_back({block.block_id, block.lightLevel, static_cast<std::uint16_t>(block.lightColor), block.bitwise_compressed_data});
        return snapshot;
    }
    std::size_t Trees()
    {
        std::size_t count = 0;
        for (const auto& [coord, chunk] : ChunkManager::GetAllChunks())
            for (const auto& block : chunk.m_local_blocks) count += block.block_id == 6 || block.block_id == 7;
        return count;
    }
    void RequireAirPlayer(const TestScene::Pose& pose)
    {
        const auto low = glm::floor(pose.position - glm::vec3(0.275f, 0.9f, 0.275f) + glm::vec3(0.001f));
        const auto high = glm::floor(pose.position + glm::vec3(0.275f, 0.9f, 0.275f) - glm::vec3(0.001f));
        for (int x = static_cast<int>(low.x); x <= high.x; ++x)
            for (int y = static_cast<int>(low.y); y <= high.y; ++y)
                for (int z = static_cast<int>(low.z); z <= high.z; ++z)
                    Require(!get_block(ChunkManager::GetBlock({x, y, z}).block_id).m_is_solid, "Checkpoint embeds the player in a solid block");
    }
}

int main(int argc, char* argv[])
{
    using namespace SymoCraft;
    try {
        Require(argc == 2, "Expected the real block configuration path");
        LoadBlocks(argv[1]);
        const Generation::Settings settings{424242, 3, true};
        Generation::Build(settings);
        const auto baseline = Generation::BlockDigest();
        const auto content = Snapshot(settings.radius);
        Require(Trees() > 0, "Seed fixture must exercise vegetation, not just terrain");
        Generation::Populate(Generation::Generator(settings));
        Require(Generation::BlockDigest() == baseline && Snapshot(settings.radius) == content, "Repeated generation retained random or block state");
        ChunkManager::FreeAllChunks();
        for (int x = settings.radius; x >= -settings.radius; --x)
            for (int z = settings.radius; z >= -settings.radius; --z) ChunkManager::CreateChunk({x, z});
        Generation::Populate(Generation::Generator(settings));
        Require(Generation::BlockDigest() == baseline && Snapshot(settings.radius) == content, "Creation/map iteration order changed the world");
        const auto generator = Generation::Generator(settings);
        auto first = generator.VegetationRandom(-1, 0);
        auto same = generator.VegetationRandom(-1, 0);
        auto unrelated = generator.VegetationRandom(1, 0);
        for (int i = 0; i < 100; ++i) {
            static_cast<void>(unrelated());
            Require(first() == same(), "Vegetation streams share mutable random state");
        }

        TestScene::Install();
        const auto scene = Generation::BlockDigest();
        Require(scene != baseline, "Regression overlay was not installed");
        for (const auto& pose : TestScene::Checkpoints()) RequireAirPlayer(pose);
        Require(ChunkManager::GetBlock({8, 162, 6}).block_id == 11 && ChunkManager::GetBlock({6, 162, 8}).block_id == 11, "Wall corner fixture missing");
        Require(ChunkManager::GetBlock({-6, 163, 6}).block_id == 8 && ChunkManager::GetBlock({-6, 162, 6}).block_id == 1, "Low-ceiling clearance is wrong");
        Require(ChunkManager::GetBlock({20, 164, 20}).block_id == 5 && ChunkManager::GetBlock({21, 164, 20}).block_id == 1, "Single-block landing target is not isolated");
        Require(World::ToChunkCoords({-0.5f, 161, -0.5f}) == glm::ivec2(-1, -1), "Negative coordinates use the wrong real mapping");
        for (const auto& edit : TestScene::Edits()) {
            auto* chunk = ChunkManager::GetChunk(glm::vec3(edit.position));
            Require(chunk && !chunk->m_is_fringe_chunk, "Edit fixture outside playable chunks");
        }
        TestScene::ApplyEdits();
        const auto edited = Generation::BlockDigest();
        Require(edited != scene, "Edit sequence did not change world content");
        for (std::size_t i = 1; i < TestScene::Edits().size(); i += 2) {
            const auto& edit = TestScene::Edits()[i];
            Require(ChunkManager::GetBlock(glm::vec3(edit.position)).block_id == edit.after, "Ordered boundary edit produced the wrong block");
        }
        Generation::Build(settings);
        TestScene::Install();
        Require(Generation::BlockDigest() == scene, "Scene cannot be rebuilt after editing and clearing");
        TestScene::ApplyEdits();
        Require(Generation::BlockDigest() == edited, "Edit plan is not reproducible");
        Generation::Build({settings.seed + 1, settings.radius, true});
        Require(Generation::BlockDigest() != baseline, "Different seed did not change world content");
        Generation::Build({settings.seed, settings.radius, false});
        Require(Trees() == 0 && Generation::BlockDigest() != baseline, "Vegetation configuration is not respected");
        std::cout << "generator_version=" << Generation::Version << "; seed=" << settings.seed
                  << "; radius=" << settings.radius << "; terrain=" << baseline
                  << "; scene=" << scene << "; edited=" << edited << '\n';
        ChunkManager::FreeAllChunks();
        return 0;
    } catch (const std::exception& error) {
        ChunkManager::FreeAllChunks();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
