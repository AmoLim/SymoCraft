#include <random>
#include <algorithm>
#include "chunk.h"
#include <symocraft/world/world.h>
#include <symocraft/world/generation.h>
#include <symocraft/world/constants.h>


namespace SymoCraft
{
    static float g_normal;
    static std::array<std::array<BlockVertex3D, 4>, 6> block_faces{}; // Each block contains 6 faces, which contains 4 vertices

    Chunk::Chunk()
        : m_local_blocks(k_chunk_length * k_chunk_width * k_chunk_height, BlockConstants::AIR_BLOCK)
    {
    }

    Block Chunk::GetLocalBlock(int x, int y, int z) {
        if (y < 0 || y >= k_chunk_height || m_local_blocks.empty())
            return BlockConstants::NULL_BLOCK;
        if (x >= k_chunk_length || x < 0 || z >= k_chunk_width || z < 0) {
            if (x >= k_chunk_length) {
                return front_neighbor ? front_neighbor->GetLocalBlock(x - k_chunk_length, y, z) : BlockConstants::NULL_BLOCK;
            } else if (x < 0) {
                return back_neighbor ? back_neighbor->GetLocalBlock(k_chunk_length + x, y, z) : BlockConstants::NULL_BLOCK;
            }

            if (z >= k_chunk_width) {
                return right_neighbor ? right_neighbor->GetLocalBlock(x, y, z - k_chunk_width) : BlockConstants::NULL_BLOCK;
            } else if (z < 0) {
                return left_neighbor ? left_neighbor->GetLocalBlock(x, y, k_chunk_width + z) : BlockConstants::NULL_BLOCK;
            }
        }
        else if (y >= k_chunk_height || y < 0)
            return BlockConstants::NULL_BLOCK;

        return m_local_blocks[GetLocalBlockIndex(x, y, z)];
    }

    Block Chunk::GetWorldBlock(const glm::vec3 &world_coord) {
        glm::ivec3 localPosition = glm::floor(
                world_coord - glm::vec3(m_chunk_coord.x * 16.0f, 0.0f, m_chunk_coord.y * 16.0f));
        return GetLocalBlock(localPosition.x, localPosition.y, localPosition.z);
    }

    bool Chunk::SetLocalBlock(int x, int y, int z, uint16 block_id) {
        if (y < 0 || y >= k_chunk_height || m_local_blocks.empty())
            return false;
        if (x >= k_chunk_length || x < 0 || z >= k_chunk_width || z < 0)
        {
            if (x >= k_chunk_length) {
                return front_neighbor && front_neighbor->SetLocalBlock(x - k_chunk_length, y, z, block_id);
            } else if (x < 0) {
                return back_neighbor && back_neighbor->SetLocalBlock(k_chunk_length + x, y, z, block_id);
            }

            if (z >= k_chunk_width) {
                return right_neighbor && right_neighbor->SetLocalBlock(x, y, z - k_chunk_width, block_id);
            } else if (z < 0) {
                return left_neighbor && left_neighbor->SetLocalBlock(x, y, k_chunk_width + z, block_id);
            }
        }
        else if (y >= k_chunk_height || y < 0)
            return false;


        int index = GetLocalBlockIndex(x, y, z);
        BlockFormat blockFormat = get_block(block_id);
        m_local_blocks[index].block_id = block_id;
        m_local_blocks[index].SetTransparency(blockFormat.m_is_transparent);
        m_local_blocks[index].SetBlendability(blockFormat.m_is_blendable);
        m_local_blocks[index].SetLightSource(blockFormat.m_is_lightSource);

        UpdateChunkLocalBlocks({x, y, z});
        return true;
    }

    bool Chunk::SetWorldBlock(const glm::vec3 &world_coord, uint16 block_id) {
        glm::ivec3 localPosition = glm::floor(
                world_coord - glm::vec3(m_chunk_coord.x * 16.0f, 0.0f, m_chunk_coord.y * 16.0f));
        return SetLocalBlock(localPosition.x, localPosition.y, localPosition.z, block_id);
    }

    bool Chunk::RemoveLocalBlock(int x, int y, int z) {
        if (y < 0 || y >= k_chunk_height || m_local_blocks.empty())
            return false;
        if (x >= k_chunk_length || x < 0 || z >= k_chunk_width || z < 0) {
            if (x >= k_chunk_length) {
                return front_neighbor && front_neighbor->RemoveLocalBlock(x - k_chunk_length, y, z);
            } else if (x < 0) {
                return back_neighbor && back_neighbor->RemoveLocalBlock(k_chunk_length + x, y, z);
            }

            if (z >= k_chunk_width) {
                return right_neighbor && right_neighbor->RemoveLocalBlock(x, y, z - k_chunk_width);
            } else if (z < 0) {
                return left_neighbor && left_neighbor->RemoveLocalBlock(x, y, k_chunk_width + z);
            }
        } else if (y >= k_chunk_height || y < 0) {
            return false;
        }

        // Replace the block with an air block
        int index = SymoCraft::Chunk::GetLocalBlockIndex(x, y, z);
        m_local_blocks[index].block_id = BlockConstants::AIR_BLOCK.block_id;
        m_local_blocks[index].SetTransparency(true);
        m_local_blocks[index].SetBlendability(false);
        m_local_blocks[index].SetLightSource(false);

        UpdateChunkLocalBlocks({x, y, z});
        return true;
    }

    bool Chunk::RemoveWorldBlock(const glm::vec3 &world_coord) {
        glm::ivec3 localPosition = glm::floor(
                world_coord - glm::vec3(m_chunk_coord.x * 16.0f, 0.0f, m_chunk_coord.y * 16.0f));
        return RemoveLocalBlock(localPosition.x, localPosition.y, localPosition.z);
    }

    void Chunk::GenerateTerrain(const Generation::Generator& generator) {
        m_local_blocks.assign(k_chunk_width * k_chunk_height * k_chunk_length, BlockConstants::AIR_BLOCK);
        m_vertex_data.clear();
        state = ChunkState::ToBeUpdated;

        int world_x = m_chunk_coord.x * k_chunk_length;
        int world_z = m_chunk_coord.y * k_chunk_width;
        for (int z = 0; z < k_chunk_width; z++) {
            for (int x = 0; x < k_chunk_length; x++) {
                const int max_height = std::clamp(static_cast<int>(generator.Height(x + world_x, z + world_z)), 0, k_chunk_height - 1);
                const int stone_height = std::max(0, max_height - 6);

                for (int y = 0; y < k_chunk_height; y++) {
                    const int block_index = GetLocalBlockIndex(x , y, z);
                    if (m_is_fringe_chunk)
                    {
                        m_local_blocks[block_index].block_id = BlockConstants::AIR_BLOCK.block_id;
                        m_local_blocks[block_index].SetTransparency(true);
                        m_local_blocks[block_index].SetBlendability(false);
                        m_local_blocks[block_index].SetLightSource(false);
                        m_local_blocks[block_index].SetLightColor(glm::ivec3(255, 255, 255));
                        continue;
                    };

                        if (y == 0) {
                            // Bedrock
                            m_local_blocks[block_index].block_id = 5;
                            // Set the first bit of compressed data to false, to let us know
                            // this is not a transparent block
                            m_local_blocks[block_index].SetTransparency(false);
                            m_local_blocks[block_index].SetBlendability(false);
                            m_local_blocks[block_index].SetLightSource(false);
                        } else if (y < stone_height) {
                            // Stone
                            m_local_blocks[block_index].block_id = 5;
                            m_local_blocks[block_index].SetTransparency(false);
                            m_local_blocks[block_index].SetBlendability(false);
                            m_local_blocks[block_index].SetLightSource(false);
                        } else if (y < max_height) {
                            // Dirt
                            m_local_blocks[block_index].block_id = 4;
                            m_local_blocks[block_index].SetTransparency(false);
                            m_local_blocks[block_index].SetBlendability(false);
                            m_local_blocks[block_index].SetLightSource(false);
                        } else if (y == max_height ) {
                            if (max_height < sea_level + 2) {
                                // Sand
                                m_local_blocks[block_index].block_id = 3;
                                m_local_blocks[block_index].SetTransparency(false);
                                m_local_blocks[block_index].SetBlendability(false);
                                m_local_blocks[block_index].SetLightSource(false);
                            } else {
                                // Grass
                                m_local_blocks[block_index].block_id = 2;
                                m_local_blocks[block_index].SetTransparency(false);
                                m_local_blocks[block_index].SetBlendability(false);
                                m_local_blocks[block_index].SetLightSource(false);
                            }
                        } else if (y >= min_biome_height && y < sea_level) {
                            // Water
                            m_local_blocks[block_index].block_id = 9;
                            m_local_blocks[block_index].SetTransparency(false);
                            m_local_blocks[block_index].SetBlendability(true);
                            m_local_blocks[block_index].SetLightSource(false);
                        } else {
                            m_local_blocks[block_index].block_id = BlockConstants::AIR_BLOCK.block_id;
                            m_local_blocks[block_index].SetTransparency(true);
                            m_local_blocks[block_index].SetBlendability(false);
                            m_local_blocks[block_index].SetLightSource(false);
                        }
                }
            }
        }
    }

    void Chunk::GenerateVegetation(const Generation::Generator& generator)
    {
           if (m_is_fringe_chunk || m_local_blocks.empty())
               return;
           const int worldChunkX = m_chunk_coord.x * 16;
           const int worldChunkZ = m_chunk_coord.y * 16;
           auto mt = generator.VegetationRandom(m_chunk_coord.x, m_chunk_coord.y);

           const int vegetation_length = 10;
           const int vegetation_width = 10;
           for (int x = 0; x < vegetation_length; x++)
           {
               for (int z = 0; z < vegetation_width; z++)
               {
                   // Generate trees at random
                   if (mt() % 100 > 98)
                   {
                       auto y = static_cast<uint16>(generator.Height(x + worldChunkX, z + worldChunkZ) + 1);

                       if (y > sea_level + 2)
                       {
                           // Set tree attributes
                           uint16 top_trunk_y = (mt() % 3) + 3;
                           uint16 top_ring_y = top_trunk_y + 1;
                           uint16 bottom_ring_y = top_trunk_y - 2;

                           // Start generating
                           if (y + 1 + top_ring_y < k_chunk_height) {
                               // Generate trunks
                               for (int trunk_y = 0; trunk_y <= top_trunk_y; trunk_y++)
                                   SetLocalBlock(x, trunk_y + y, z, 6);


                               int leaf_y = bottom_ring_y + y;
                               int leaf_radius = 2;
                               // Generate the bottom two rings
                               for (int loop_count = 1; loop_count <= 2; loop_count++)
                               {
                                   for (int leaf_x = x - leaf_radius; leaf_x <= x + leaf_radius; leaf_x++)
                                   {
                                       for (int leaf_z = z - leaf_radius; leaf_z <= z + leaf_radius; leaf_z++)
                                       {
                                           // The leaves at the four corners is generated randomly
                                           if ( (leaf_x == x - leaf_radius || leaf_x == x + leaf_radius)
                                            && (leaf_z == z - leaf_radius || leaf_z == z + leaf_radius) )
                                           {
                                               bool flag = mt() % 5 < 2;
                                               if (flag)
                                                   continue;
                                           }

                                           SetLocalBlock(leaf_x, leaf_y, leaf_z, 7);
                                       }
                                   }
                                   leaf_y++;
                               }


                               // Generate the second ring
                               leaf_radius = 1;
                               for (int leaf_x = x - leaf_radius; leaf_x <= x + leaf_radius; leaf_x++)
                               {
                                   for (int leaf_z = z - leaf_radius; leaf_z <= z + leaf_radius; leaf_z++)
                                   {
                                       // The leaves at the four corners is generated randomly
                                       if ((leaf_x == x - leaf_radius || leaf_x == x + leaf_radius)
                                           && (leaf_z == z - leaf_radius || leaf_z == z + leaf_radius))
                                       {
                                           bool flag = mt() % 6 < 1;
                                           if (flag)
                                               continue;
                                       }

                                       SetLocalBlock(leaf_x, leaf_y, leaf_z, 7);
                                   }

                               }

                               // Generate the top ring
                               leaf_y++;
                               for (int leaf_x = x - leaf_radius; leaf_x <= x + leaf_radius; leaf_x++)
                               {
                                   for (int leaf_z = z - leaf_radius; leaf_z <= z + leaf_radius; leaf_z++)
                                   {
                                       // The leaves at the four corners is skipped
                                       if ( (leaf_x == x - leaf_radius || leaf_x == x + leaf_radius)
                                            && (leaf_z == z - leaf_radius || leaf_z == z + leaf_radius) )
                                           continue;

                                       SetLocalBlock(leaf_x,leaf_y,leaf_z, 7);
                                   }
                               }
                           }
                       }
                   }
               }
           }
    }

    void Chunk::Free()
    {
        std::vector<Block>().swap(m_local_blocks);
        std::vector<BlockVertex3D>().swap(m_vertex_data);
        front_neighbor = back_neighbor = left_neighbor = right_neighbor = nullptr;
        state = ChunkState::None;
    }

    void Chunk::GenerateRenderData()
    {
        m_vertex_data.clear();
        if(m_is_fringe_chunk || m_local_blocks.empty())
        {
            state = ChunkState::Updated;
            return;
        }

        const int kWorldChunkX = m_chunk_coord.x * 16;
        const int kWorldChunkZ = m_chunk_coord.y * 16;

        for (int y = 0; y < k_chunk_height; y++)
        {
            for (int x = 0; x < k_chunk_length; x++)
            {
                for (int z = 0; z < k_chunk_width; z++)
                {

                    // 36 Vertices per cube
                    const Block &block = GetLocalBlock(x, y, z);

                    if (block == BlockConstants::NULL_BLOCK || block == BlockConstants::AIR_BLOCK) {
                        continue;
                    }

                    const BlockFormat &block_format = get_block(block.block_id);

                    // The order of coordinates is FRONT, RIGHT, BACK, LEFT, TOP, BOTTOM neighbor_blocks to check
                    const int neighbor_block_x_coords[6] = {x + 1,     x, x - 1,     x,     x,     x};
                    const int neighbor_block_y_coords[6] = {    y,     y,     y,     y, y + 1, y - 1};
                    const int neighbor_block_z_coords[6] = {    z, z + 1,     z, z - 1,     z,     z};

                    // The 6 neighbor blocks that the target block is facing
                    Block neighbor_blocks[6];

                    uint16 i;

                    for (i = 0; auto &neighbor_block: neighbor_blocks) {
                        neighbor_block = GetLocalBlock(neighbor_block_x_coords[i], neighbor_block_y_coords[i], neighbor_block_z_coords[i]);
                        i++;
                    }

                    // Only add the faces that are not culled by other neighbor_blocks
                    // Use the 6 blocks to iterate through the 6 faces
                    for (i = 0; auto &neighbor_block: neighbor_blocks)
                    {
                        // If neighbor block is not null and is transparent
                        if (neighbor_block != BlockConstants::NULL_BLOCK && neighbor_block.IsTransparent())
                        {
                            //If the face aren't culled, calculate its 4 vertices
                            for( int j = 0; j < 4; j++)
                            {
                                block_faces[i][j].pos_coord = (glm::ivec3(x + kWorldChunkX, y, z + kWorldChunkZ) +
                                        BlockConstants::pos_coords[BlockConstants::vertex_indices[i * 4 + j]]);
                                block_faces[i][j].tex_coord = {BlockConstants::tex_coords[j % 4], // Set uv coords
                                                                (i * 4 + j >= 16) ? ((i * 4 + j >= 20)
                                                                 ? // Set layer i, sides first, the top second, the bottom last
                                                                 block_format.m_bottom_texture
                                                                 : block_format.m_top_texture) // if 16 <= i < 20, assign top_tex
                                                                 : block_format.m_side_texture}; // if i < 16, assign side_tex
                                block_faces[i][j].normal = g_normal;
                            }


                            // vector grows before insertion; no fixed buffer or 16-bit counter.
                            m_vertex_data.insert(m_vertex_data.end(), {
                                block_faces[i][0], block_faces[i][1], block_faces[i][2],
                                block_faces[i][0], block_faces[i][2], block_faces[i][3]});
                        }
                        i++;
                    }
                }
            }
        }

        state = ChunkState::Updated;
    }

    void Chunk::UpdateChunkLocalBlocks(const glm::vec3& block_local_coord)
    {
        state = ChunkState::ToBeUpdated;

        if (block_local_coord.x == 0)
        {
            if (back_neighbor)
                back_neighbor->state = ChunkState::ToBeUpdated;
        }
        else if (block_local_coord.x == 15)
        {
            if (front_neighbor)
                front_neighbor->state = ChunkState::ToBeUpdated;
        }

        if (block_local_coord.z == 0)
        {
            if (left_neighbor)
                left_neighbor->state = ChunkState::ToBeUpdated;
        }
        else if (block_local_coord.z == 15)
        {
            if (right_neighbor)
                right_neighbor->state = ChunkState::ToBeUpdated;
        }
    }
}
