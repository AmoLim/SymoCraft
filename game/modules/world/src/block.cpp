#include <symocraft/world/block_definition.h>
#include <yaml-cpp/yaml.h>
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace SymoCraft::World {
    struct BlockDefinition::Storage {
        std::vector<std::pair<BlockId, BlockDefinitionEntry>> rules;
        std::vector<std::pair<std::string, BlockId>> names;
    };
    BlockDefinition::BlockDefinition(std::unique_ptr<Storage> storage) : storage_(std::move(storage)) {}
    BlockDefinition::~BlockDefinition() = default;
    BlockDefinition::BlockDefinition(BlockDefinition&&) noexcept = default;
    BlockDefinition& BlockDefinition::operator=(BlockDefinition&&) noexcept = default;
    BlockDefinition::BlockDefinition(const BlockDefinition& other)
        : storage_(other.storage_ ? std::make_unique<Storage>(*other.storage_) : nullptr) {}
    BlockDefinition& BlockDefinition::operator=(const BlockDefinition& other)
    {
        if (this != &other) {
            BlockDefinition copy(other);
            storage_.swap(copy.storage_);
        }
        return *this;
    }
    std::optional<BlockDefinitionEntry> BlockDefinition::Find(BlockId id) const
    {
        if (!storage_) return std::nullopt;
        const auto found = std::lower_bound(storage_->rules.begin(), storage_->rules.end(), id,
            [](const auto& pair, BlockId value) { return pair.first < value; });
        if (found == storage_->rules.end() || found->first != id) return std::nullopt;
        return found->second;
    }
    std::optional<BlockId> BlockDefinition::FindId(std::string_view name) const
    {
        if (!storage_) return std::nullopt;
        const auto found = std::lower_bound(storage_->names.begin(), storage_->names.end(), name,
            [](const auto& pair, std::string_view value) { return pair.first < value; });
        if (found == storage_->names.end() || found->first != name) return std::nullopt;
        return found->second;
    }
    BlockDefinition BlockDefinition::FromConfig(std::string_view text)
    {
        try {
            const auto root = YAML::Load(std::string(text));
            if (!root.IsMap() || root.size() == 0)
                throw std::runtime_error("Block configuration must be a nonempty mapping");
            auto storage = std::make_unique<Storage>();
            for (const auto& block : root) {
                const auto name = block.first.as<std::string>();
                const auto node = block.second;
                if (name.empty() || !node.IsMap()) throw std::runtime_error("Invalid block entry: " + name);
                const auto id = node["id"].as<int>();
                if (id <= 0 || id > std::numeric_limits<BlockId>::max())
                    throw std::runtime_error("Invalid block id for " + name);
                for (const auto& pair : storage->rules)
                    if (pair.first == id) throw std::runtime_error("Duplicate block id: " + name);
                for (const auto& pair : storage->names)
                    if (pair.first == name) throw std::runtime_error("Duplicate block name: " + name);
                const auto texture = [&](const char* field) {
                    const int value = node[field].as<int>();
                    if (value < 0 || value > std::numeric_limits<uint16>::max())
                        throw std::runtime_error("Invalid texture layer: " + name + "/" + field);
                    return static_cast<uint16>(value);
                };
                BlockDefinitionEntry entry{texture("top"), texture("side"), texture("bottom"),
                    node["isTransparent"].as<bool>(), node["isSolid"].as<bool>(),
                    node["isBlendable"].IsDefined() && node["isBlendable"].as<bool>(),
                    node["isLightSource"].IsDefined() && node["isLightSource"].as<bool>(),
                    node["light_level"].IsDefined() ? node["light_level"].as<int16>() : int16{0}};
                storage->rules.emplace_back(static_cast<BlockId>(id), entry);
                storage->names.emplace_back(name, static_cast<BlockId>(id));
            }
            std::sort(storage->rules.begin(), storage->rules.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            std::sort(storage->names.begin(), storage->names.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
            BlockDefinition definition(std::move(storage));
            constexpr std::array<std::string_view, 11> required{
                "air_block", "grass_block", "sand", "dirt", "stone", "oak_log", "oak_leaves", "oak_planks", "water_still",
                "birch_planks", "cobblestone"};
            for (std::size_t i = 0; i < required.size(); ++i)
                if (definition.FindId(required[i]) != i + 1)
                    throw std::runtime_error("Missing or incompatible required block: " + std::string(required[i]));
            return definition;
        } catch (const YAML::Exception& error) {
            throw std::runtime_error("Block configuration parse failed: " + std::string(error.what()));
        }
    }
    void BlockDefinition::ValidateTextureLayers(std::size_t count) const
    {
        if (!storage_) throw std::logic_error("Block definition was moved from");
        if (!count) throw std::runtime_error("Texture array has no layers");
        for (const auto& [id, rule] : storage_->rules)
            if (rule.m_top_texture >= count || rule.m_side_texture >= count || rule.m_bottom_texture >= count)
                throw std::runtime_error("Block " + std::to_string(id) + " references a texture layer outside the atlas");
    }
}
