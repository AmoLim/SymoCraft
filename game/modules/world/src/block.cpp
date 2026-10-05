#include <symocraft/world/block.h>
#include <robin_hood.h>
#include <yaml-cpp/yaml.h>
#include <array>
#include <string>
#include <limits>
#include <stdexcept>

namespace SymoCraft{
    static robin_hood::unordered_flat_map<uint16, BlockFormat> block_format_map;
    static robin_hood::unordered_flat_map<std::string , uint16> name_to_id_map;

    uint16 get_block_id(std::string_view block_name)
    {
        const auto& iter = name_to_id_map.find(std::string(block_name));
        if (iter == name_to_id_map.end())
        {
            return 0;
        }
        return iter->second;
    }

    const BlockFormat& get_block(int block_id)
    {
        static const BlockFormat empty{0, 0, 0, true, false, false, false, 0};
        if (block_id < 0 || block_id > std::numeric_limits<uint16>::max())
            return empty;
        const auto found = block_format_map.find(static_cast<uint16>(block_id));
        return found != block_format_map.end() ? found->second : empty;
    }


    const BlockFormat& get_block(std::string_view name)
    {
        return get_block(get_block_id(name));
    }

    void LoadBlocks(std::string_view block_format_config)
    {
        YAML::Node block_formats = YAML::LoadFile(std::string(block_format_config));
        if (!block_formats.IsMap() || block_formats.size() == 0)
            throw std::runtime_error("Block configuration must be a nonempty mapping");
        decltype(block_format_map) formats;
        decltype(name_to_id_map) names;

        for (auto block : block_formats)
        {
            const auto name = block.first.as<std::string>();
            if (!block.second.IsMap() || name.empty())
                throw std::runtime_error("Invalid block entry: " + name);
            int id = block.second["id"].as<int>();
            if (id <= 0 || id > std::numeric_limits<uint16>::max())
                throw std::runtime_error("Invalid block id for " + name);
            if (formats.contains(static_cast<uint16>(id)) || names.contains(name))
                throw std::runtime_error("Duplicate block id or name: " + name);
            names[name] = static_cast<uint16>(id);

            auto side_texture =  block.second["side"].as<uint16>();
            auto top_texture= block.second["top"].as<uint16>();
            auto bottom_texture = block.second["bottom"].as<uint16>();
            bool isTransparent = block.second["isTransparent"].as<bool>();
            bool isSolid = block.second["isSolid"].as<bool>();
            bool isBlendable = block.second["isBlendable"].IsDefined() && block.second["isBlendable"].as<bool>();
            bool isLightSource = block.second["isLightSource"].IsDefined() && block.second["isLightSource"].as<bool>();
            int16 lightLevel = block.second["light_level"].IsDefined() ? block.second["light_level"].as<int16>() : 0;

            formats[static_cast<uint16>(id)] = BlockFormat{
                    top_texture, side_texture, bottom_texture,
                    isTransparent, isSolid, isBlendable,
                    isLightSource, lightLevel };
        }
        // Terrain generation and the existing hotbar use these fixed IDs.
        constexpr std::array<std::string_view, 11> required_names{
            "air_block", "grass_block", "sand", "dirt", "stone", "oak_log", "oak_leaves", "oak_planks", "water_still",
            "birch_planks", "cobblestone"
        };
        for (std::size_t index = 0; index < required_names.size(); ++index)
        {
            const auto found = names.find(std::string(required_names[index]));
            if (found == names.end() || found->second != index + 1)
                throw std::runtime_error("Missing or incompatible required block: " + std::string(required_names[index]));
        }
        block_format_map = std::move(formats);
        name_to_id_map = std::move(names);
    }

    void ValidateBlockTextures(std::size_t layer_count)
    {
        if (layer_count == 0)
            throw std::runtime_error("Texture array has no layers");
        for (const auto& [id, format] : block_format_map)
        {
            if (format.m_top_texture >= layer_count || format.m_side_texture >= layer_count ||
                format.m_bottom_texture >= layer_count)
                throw std::runtime_error("Block " + std::to_string(id) + " references a texture layer outside the atlas");
        }
    }
}
