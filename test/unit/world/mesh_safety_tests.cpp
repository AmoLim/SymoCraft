#include "chunk.h"
#include "chunk_store.h"
#include "symocraft/world/constants.h"
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {
    using namespace SymoCraft;

    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    void CheckMissingNeighbors()
    {
        Chunk chunk;
        const std::array<glm::vec3, 8> outside{
            glm::vec3{-1, 10, 0}, glm::vec3{16, 10, 0},
            glm::vec3{0, 10, -1}, glm::vec3{0, 10, 16},
            glm::vec3{0, -1, 0}, glm::vec3{0, 256, 0},
            glm::vec3{16, 256, 16}, glm::vec3{-1, -1, -1}};
        for (const auto& position : outside) {
            Require(chunk.GetWorldBlock(position) == BlockConstants::NULL_BLOCK, "Missing neighbor read must return null.");
            Require(!chunk.SetWorldBlock(position, 2), "Missing neighbor write must fail before indexing.");
            Require(!chunk.RemoveWorldBlock(position), "Missing neighbor removal must fail before indexing.");
        }
        Require(chunk.GetWorldBlock({0, 10, 0}) == BlockConstants::AIR_BLOCK, "Rejected writes changed the local chunk.");
        chunk.Free();
        chunk.Free();
        Require(chunk.GetWorldBlock({0, 0, 0}) == BlockConstants::NULL_BLOCK, "Freed chunk read was not safe.");
        Require(!chunk.SetWorldBlock({0, 0, 0}, 2), "Freed chunk accepted a write.");
        Require(!chunk.RemoveWorldBlock({0, 0, 0}), "Freed chunk accepted a removal.");
    }

    void CheckNeighborWrites()
    {
        Chunk left;
        Chunk right;
        right.m_chunk_coord = {1, 0};
        left.front_neighbor = &right;
        right.back_neighbor = &left;
        left.state = right.state = ChunkState::Updated;
        Require(left.SetWorldBlock({16, 64, 3}, 2), "Cross-chunk write failed.");
        Require(right.GetWorldBlock({16, 64, 3}).block_id == 2, "Cross-chunk write indexed the wrong block.");
        Require(left.state == ChunkState::ToBeUpdated && right.state == ChunkState::ToBeUpdated,
            "Boundary write did not invalidate both meshes.");
        Require(left.RemoveWorldBlock({16, 64, 3}), "Cross-chunk removal failed.");
        Require(right.GetWorldBlock({16, 64, 3}) == BlockConstants::AIR_BLOCK, "Boundary removal did not create air.");
        Require(!left.SetWorldBlock({16, 256, 3}, 2), "Vertical bounds were not checked before neighbor routing.");
    }

    void CheckFaceCounts()
    {
        Chunk chunk;
        chunk.GenerateRenderData();
        Require(chunk.VertexCount() == 0, "Air chunk generated vertices.");
        chunk.SetWorldBlock({8, 80, 8}, 2);
        chunk.GenerateRenderData();
        Require(chunk.VertexCount() == 36, "Isolated cube must contain 36 vertices.");
        chunk.SetWorldBlock({9, 80, 8}, 2);
        chunk.GenerateRenderData();
        Require(chunk.VertexCount() == 60, "Adjacent cubes did not cull their shared face.");
        chunk.RemoveWorldBlock({8, 80, 8});
        chunk.RemoveWorldBlock({9, 80, 8});
        chunk.GenerateRenderData();
        Require(chunk.VertexCount() == 0, "Regeneration retained stale vertices.");
    }

    void CheckLargeMeshAndMove()
    {
        Chunk chunk;
        std::size_t block_count = 0;
        for (int y = 1; y < k_chunk_height - 1; y += 2)
            for (int x = 1; x < k_chunk_length - 1; x += 2)
                for (int z = 1; z < k_chunk_width - 1; z += 2) {
                    Require(chunk.SetWorldBlock({x, y, z}, 2), "Checkerboard insertion failed.");
                    ++block_count;
                }
        chunk.GenerateRenderData();
        const std::size_t expected = block_count * 36;
        Require(expected > UINT16_MAX, "Large-mesh fixture did not cross the old 16-bit limit.");
        Require(chunk.VertexCount() == expected, "Mesh vertex count overflowed or faces were lost.");
        chunk.GenerateRenderData();
        Require(chunk.VertexCount() == expected, "Repeated meshing retained stale data.");

        Chunk moved(std::move(chunk));
        chunk.Free();
        Require(moved.VertexCount() == expected, "Moving a chunk lost or shared mesh storage.");
        moved.GenerateRenderData();
        Require(moved.VertexCount() == expected, "Moved chunk no longer owns its block storage.");
    }

    void CheckManagerLifetime()
    {
        ChunkManager::FreeAllChunks();
        ChunkManager::CreateChunk({0, 0});
        ChunkManager::RearrangeChunkNeighborPointers();
        Chunk* center = ChunkManager::GetChunk(glm::ivec2{0, 0});
        Require(center && center->m_is_fringe_chunk, "Isolated manager chunk was not marked as fringe.");
        for (const auto& direction : INormals2::CardinalDirections)
            ChunkManager::CreateChunk(direction);
        ChunkManager::CreateChunk({0, 0});
        ChunkManager::RearrangeChunkNeighborPointers();
        Require(ChunkManager::GetAllChunks().size() == 5, "Repeated chunk creation replaced or duplicated ownership.");
        Require(center == ChunkManager::GetChunk(glm::ivec2{0, 0}), "Node-map insertion invalidated a chunk pointer.");
        Require(!center->m_is_fringe_chunk, "Neighbor rebuild failed to clear stale fringe state.");
        ChunkManager::FreeAllChunks();
        ChunkManager::FreeAllChunks();
        Require(ChunkManager::GetAllChunks().empty(), "FreeAllChunks retained freed entries.");
        Require(ChunkManager::GetChunk(glm::ivec2{0, 0}) == nullptr, "Freed chunk remained discoverable.");
        ChunkManager::CreateChunk({0, 0});
        Require(ChunkManager::GetBlock({0, 20, 0}) == BlockConstants::AIR_BLOCK, "Recreated chunk inherited stale storage.");
        ChunkManager::FreeAllChunks();
    }
}

int main(int argc, char** argv)
{
    static_assert(!std::is_copy_constructible_v<SymoCraft::Chunk>);
    static_assert(!std::is_copy_assignable_v<SymoCraft::Chunk>);
    static_assert(std::is_nothrow_move_constructible_v<SymoCraft::Chunk>);
    try {
        Require(argc == 2, "Expected the real block configuration path.");
        SymoCraft::LoadBlocks(argv[1]);
        CheckMissingNeighbors();
        CheckNeighborWrites();
        CheckFaceCounts();
        CheckLargeMeshAndMove();
        CheckManagerLifetime();
        std::cout << "Mesh and chunk lifetime tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
