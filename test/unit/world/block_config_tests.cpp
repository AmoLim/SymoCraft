#include "symocraft/world/block_definition.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
            throw std::runtime_error("Expected a block configuration path");
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) throw std::runtime_error("Cannot read block configuration");
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const auto definition = SymoCraft::World::BlockDefinition::FromConfig(text);
        definition.ValidateTextureLayers(64);
        if (!definition.Find(2)->m_is_solid || definition.Find(1)->m_is_solid)
            throw std::runtime_error("Solid/air configuration mismatch");
        if (!definition.Find(7)->m_is_blendable)
            throw std::runtime_error("isBlendable was not parsed");
        const std::string padded_name = "grass_block_extra";
        if (definition.FindId(std::string_view(padded_name.data(), 11)) != 2)
            throw std::runtime_error("Block name lookup read beyond its string_view");

        bool rejected = false;
        try { definition.ValidateTextureLayers(10); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected)
            throw std::runtime_error("Out-of-range texture layers were accepted");

        rejected = false;
        try { definition.ValidateTextureLayers(0); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Zero texture layers were accepted");
        if (definition.Find(0) || definition.Find(65535) || definition.FindId("absent"))
            throw std::runtime_error("Unknown definition silently became air");
        const auto valid = YAML::Load(text);
        for (int test = 0; test < 10; ++test)
        {
            auto node = YAML::Clone(valid);
            if (test == 0) node = YAML::Node(YAML::NodeType::Sequence);
            if (test == 1) node["grass_block"]["id"] = 1;
            if (test == 2) node.remove("water_still");
            if (test == 3) node["grass_block"]["id"] = -1;
            if (test == 4) node.remove("birch_planks");
            if (test == 5) node.remove("cobblestone");
            if (test == 6) node["grass_block"]["id"] = 0;
            if (test == 7) node["grass_block"]["id"] = 65536;
            if (test == 8) node["grass_block"]["isSolid"] = "not-a-bool";
            if (test == 9) node["grass_block"]["side"] = -1;
            rejected = false;
            try { static_cast<void>(SymoCraft::World::BlockDefinition::FromConfig(YAML::Dump(node))); }
            catch (const std::exception&) { rejected = true; }
            if (!rejected)
                throw std::runtime_error("Malformed block configuration was accepted");
            if (definition.FindId("grass_block") != 2)
                throw std::runtime_error("Failed configuration replaced the last valid state");
        }
        rejected = false;
        try { static_cast<void>(SymoCraft::World::BlockDefinition::FromConfig(text + "\ngrass_block:\n  id: 12\n")); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected) throw std::runtime_error("Duplicate definition name accepted");
        auto copied = definition;
        auto rule = copied.Find(2).value();
        rule.m_is_solid = false;
        if (!copied.Find(2)->m_is_solid || !definition.Find(2)->m_is_solid)
            throw std::runtime_error("Rule result borrowed definition storage");
        std::cout << "Block definition validation and value-ownership contracts passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Block configuration test failed: " << error.what() << '\n';
        return 1;
    }
}
