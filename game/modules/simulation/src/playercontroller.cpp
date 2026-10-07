#include <symocraft/simulation/playercontroller.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/physics_system.h>
#include <symocraft/simulation/player_math.h>
#include <symocraft/world/constants.h>
#include <symocraft/world/world.h>
#include <iostream>
#include <string_view>
#include <stdexcept>

namespace SymoCraft::PlayerController {
    InteractionResult DoRayCast(ECS::Registry& registry, const World::VoxelWorld& world, ECS::EntityId player,
                                const InteractionInput& input, float& placement_debounce)
    {
        const auto& player_com = registry.GetComponent<Character::PlayerComponent>(player);
        const auto& transform = registry.GetComponent<Transform>(player);
        const auto hit = Physics::RayCastStatic(world, transform.position + player_com.camera_offset, transform.front, 3.0f);
        InteractionResult result;
        if (!hit.hit) return result;
        result.selection = hit.block_center;
        if (!input.allow_input) return result;

        if (input.place && placement_debounce <= 0.0f) {
            if (input.selected_block >= 0 && static_cast<std::size_t>(input.selected_block) < kBlockInventor.size() &&
                hit.hit_normal != glm::vec3(0.0f)) {
                const auto block_id = kBlockInventor[input.selected_block];
                const glm::vec3 position = hit.block_center + hit.hit_normal;
                const auto coordinate = World::TryToBlockCoord(position);
                const auto selected_rule = world.DescribeBlock(block_id);
                if (!selected_rule) throw std::logic_error("Selected inventory block is not defined in this world");
                bool can_place = coordinate && position.y >= 0.0f && position.y < k_chunk_height;
                if (can_place) {
                    const auto existing = world.QueryBlock(*coordinate);
                    can_place = existing.status == World::BlockQueryStatus::Found;
                    if (can_place) {
                        const auto existing_rule = world.DescribeBlock(existing.block.block_id);
                        if (!existing_rule) throw std::logic_error("World contains an undefined placement target block");
                        can_place = !existing_rule->m_is_solid;
                    }
                }
                if (can_place && selected_rule->m_is_solid) {
                    const auto& hit_box = registry.GetComponent<Physics::HitBox>(player);
                    const glm::vec3 center = transform.position + hit_box.offset;
                    const glm::vec3 block_min = glm::floor(position);
                    can_place = !PlayerMath::Overlaps(center - hit_box.size * 0.5f, center + hit_box.size * 0.5f,
                                                     block_min, block_min + glm::vec3(1.0f));
                }
                if (can_place) result.edit = World::EditRequest{World::EditOperation::Set, *coordinate, block_id};
            }
            placement_debounce = 0.2f;
        }
        else if (input.remove && placement_debounce <= 0.0f) {
            if (const auto coordinate = World::TryToBlockCoord(hit.block_center))
                result.edit = World::EditRequest{World::EditOperation::Remove, *coordinate};
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
