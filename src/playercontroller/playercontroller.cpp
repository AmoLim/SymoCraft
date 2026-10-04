//
// Created by Amo on 2022/7/5.
//
#include "playercontroller/playercontroller.h"
#include "core.h"
#include "core/constants.h"
#include "core/window.h"
#include "core/ECS/registry.h"
#include "core/ECS/component.h"
#include "core/ECS/Systems/physics_system.h"
#include "world/world.h"
#include "world/chunk_manager.h"
#include "renderer/renderer.h"
#include "playercontroller/player_math.h"

namespace SymoCraft
{
    namespace PlayerController
    {
        void DoRayCast( ECS::Registry &registry, Window &window, bool allow_input)
        {
            auto player_com = registry.GetComponent<Character::PlayerComponent>(World::GetPlayer());
            auto &transform = registry.GetComponent<Transform>(World::GetPlayer());

            RaycastStaticResult res = Physics::RayCastStatic(transform.position + player_com.camera_offset, transform.front, 3.0f);
            if (res.hit)
            {
                //printf("ray hitted\n");
                Renderer::GenerateBlockFrameData(res.block_center);
                if (!allow_input) return;

                if (glfwGetMouseButton((GLFWwindow*)window.window_ptr, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS
                    && Application::block_place_debounce <=0)
                {
                    const int selected = Application::new_block_id;
                    if (selected >= 0 && static_cast<size_t>(selected) < kBlockInventor.size() &&
                        res.hit_normal != glm::vec3(0.0f))
                    {
                        const uint16 block_id = kBlockInventor[selected];
                        const glm::vec3 world_pos = res.block_center + res.hit_normal;
                        bool can_place = world_pos.y >= 0.0f && world_pos.y < k_chunk_height;
                        if (can_place)
                        {
                            const Block existing = ChunkManager::GetBlock(world_pos);
                            can_place = existing != BlockConstants::NULL_BLOCK &&
                                        !get_block(existing.block_id).m_is_solid;
                        }
                        if (can_place && get_block(block_id).m_is_solid)
                        {
                            const auto& hit_box = registry.GetComponent<Physics::HitBox>(World::GetPlayer());
                            const glm::vec3 player_center = transform.position + hit_box.offset;
                            const glm::vec3 block_min = glm::floor(world_pos);
                            can_place = !PlayerMath::Overlaps(player_center - hit_box.size * 0.5f,
                                                             player_center + hit_box.size * 0.5f,
                                                             block_min, block_min + glm::vec3(1.0f));
                        }
                        if (can_place)
                            ChunkManager::SetBlock(world_pos, block_id);
                    }
                    Application::block_place_debounce = Application::kBlockPlaceDebounceTime;
                }
                else
                if (glfwGetMouseButton((GLFWwindow*)window.window_ptr , GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS
                    && Application::block_place_debounce <= 0)
                {
                    static int num_delete;
                    ChunkManager::RemoveBLock(res.block_center);
                    num_delete++;
                    std::cout << "block removed " << num_delete  << std::endl;
                    Application::block_place_debounce = Application::kBlockPlaceDebounceTime;
                }
            }
        }

        void DisplayCurrentBlockName()
        {
            static std::string kBlockName[12] = {"", "Air Block", "Grass", "Sand", "Dirt", "Stone", "Oak Log", "Oak Leaves"
                                                , "Oak Planks", "Water Still", "Birch Plank", "Cobble Stone"};

            if (Application::new_block_id >= 0 &&
                static_cast<size_t>(Application::new_block_id) < kBlockInventor.size())
                std::cout << "Current block is " << kBlockName[kBlockInventor[Application::new_block_id]] << std::endl;

        }

    }
}
