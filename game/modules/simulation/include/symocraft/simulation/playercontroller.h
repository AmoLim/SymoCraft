#pragma once

#include <symocraft/foundation/math.h>
#include <symocraft/ecs/registry.h>
#include <array>
#include <optional>

namespace SymoCraft::PlayerController {
    inline constexpr std::array<uint16, 8> kBlockInventor = {2, 3, 4, 5, 6, 7, 10, 11};
    struct InteractionInput {
        bool allow_input = true;
        bool place{}, remove{};
        int selected_block{};
    };
    struct BlockEdit {
        glm::vec3 position;
        uint16 block_id;
        bool remove;
    };
    struct InteractionResult {
        std::optional<glm::vec3> selection;
        std::optional<BlockEdit> edit;
    };
    // Returns values, never edits the world. App commits the request in the same frame.
    InteractionResult DoRayCast(ECS::Registry& registry, ECS::EntityId player,
                                const InteractionInput& input, float& placement_debounce);
    void DisplayCurrentBlockName(int selected_block);
}
