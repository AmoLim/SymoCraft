#include <symocraft/simulation/playercontroller.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/physics_system.h>
#include <symocraft/simulation/player_math.h>
#include <symocraft/world/constants.h>
#include <symocraft/world/chunk_manager.h>
#include <iostream>
#include <string_view>

namespace SymoCraft::PlayerController {
    InteractionResult DoRayCast(ECS::Registry& registry, ECS::EntityId player,
                                const InteractionInput& input, float& placement_debounce)
    {
        const auto& player_com = registry.GetComponent<Character::PlayerComponent>(player);
        const auto& transform = registry.GetComponent<Transform>(player);
        const auto hit = Physics::RayCastStatic(transform.position + player_com.camera_offset, transform.front, 3.0f);
        InteractionResult result;
        if (!hit.hit) return result;
        result.selection = hit.block_center;
        if (!input.allow_input) return result;

        if (input.place && placement_debounce <= 0.0f) {
            if (input.selected_block >= 0 && static_cast<std::size_t>(input.selected_block) < kBlockInventor.size() &&
                hit.hit_normal != glm::vec3(0.0f)) {
                const auto block_id = kBlockInventor[input.selected_block];
                const glm::vec3 position = hit.block_center + hit.hit_normal;
                bool can_place = position.y >= 0.0f && position.y < k_chunk_height;
                if (can_place) {
                    const auto existing = ChunkManager::GetBlock(position);
                    can_place = existing != BlockConstants::NULL_BLOCK && !get_block(existing.block_id).m_is_solid;
                }
                if (can_place && get_block(block_id).m_is_solid) {
                    const auto& hit_box = registry.GetComponent<Physics::HitBox>(player);
                    const glm::vec3 center = transform.position + hit_box.offset;
                    const glm::vec3 block_min = glm::floor(position);
                    can_place = !PlayerMath::Overlaps(center - hit_box.size * 0.5f, center + hit_box.size * 0.5f,
                                                     block_min, block_min + glm::vec3(1.0f));
                }
                if (can_place) result.edit = BlockEdit{position, block_id, false};
            }
            placement_debounce = 0.2f;
        }
        else if (input.remove && placement_debounce <= 0.0f) {
            result.edit = BlockEdit{hit.block_center, BlockConstants::AIR_BLOCK.block_id, true};
            placement_debounce = 0.2f;
        }
        return result;
    }

    void DisplayCurrentBlockName(int selected_block)
    {
        static constexpr std::array<std::string_view, 12> names{
            "", "Air Block", "Grass", "Sand", "Dirt", "Stone", "Oak Log", "Oak Leaves",
            "Oak Planks", "Water Still", "Birch Plank", "Cobble Stone"};
        if (selected_block >= 0 && static_cast<std::size_t>(selected_block) < kBlockInventor.size())
            std::cout << "Current block is " << names[kBlockInventor[selected_block]] << std::endl;
    }
}
