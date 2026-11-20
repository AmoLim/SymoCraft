#pragma once

#include <symocraft/world/block.h>
#include <memory>
#include <optional>
#include <cstdint>
#include <string>

namespace SymoCraft::World {
    using BlockId = uint16;
    struct HarvestRule {
        double base_seconds;
        std::string resource_id;
        std::uint64_t quantity;
    };
    struct BlockDefinitionEntry {
        uint16 m_top_texture;
        uint16 m_side_texture;
        uint16 m_bottom_texture;
        bool m_is_transparent;
        bool m_is_solid;
        bool m_is_blendable;
        bool m_is_lightSource;
        int16 m_light_level;
        std::optional<HarvestRule> harvest;
    };
    class BlockDefinition {
    public:
        static BlockDefinition FromConfig(std::string_view text);
        ~BlockDefinition();
        BlockDefinition(const BlockDefinition&);
        BlockDefinition& operator=(const BlockDefinition&);
        BlockDefinition(BlockDefinition&&) noexcept;
        BlockDefinition& operator=(BlockDefinition&&) noexcept;
        std::optional<BlockDefinitionEntry> Find(BlockId id) const;
        std::optional<BlockId> FindId(std::string_view name) const;
        void ValidateTextureLayers(std::size_t layer_count) const;
    private:
        struct Storage;
        explicit BlockDefinition(std::unique_ptr<Storage> storage);
        std::unique_ptr<Storage> storage_;
    };
}
