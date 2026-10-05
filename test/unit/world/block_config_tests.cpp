#include "symocraft/world/block.h"
#include <yaml-cpp/yaml.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    fs::path fixture;
    try
    {
        if (argc != 2)
            throw std::runtime_error("Expected a block configuration path");
        SymoCraft::LoadBlocks(argv[1]);
        SymoCraft::ValidateBlockTextures(64);
        if (!SymoCraft::get_block(2).m_is_solid || SymoCraft::get_block(1).m_is_solid)
            throw std::runtime_error("Solid/air configuration mismatch");
        if (!SymoCraft::get_block(7).m_is_blendable)
            throw std::runtime_error("isBlendable was not parsed");
        const std::string padded_name = "grass_block_extra";
        if (SymoCraft::get_block_id(std::string_view(padded_name.data(), 11)) != 2)
            throw std::runtime_error("Block name lookup read beyond its string_view");

        bool rejected = false;
        try { SymoCraft::ValidateBlockTextures(10); }
        catch (const std::exception&) { rejected = true; }
        if (!rejected)
            throw std::runtime_error("Out-of-range texture layers were accepted");

        const auto parent = fs::temp_directory_path();
        fixture = parent / ("symocraft-block-config-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".yaml");
        if (fs::exists(fixture))
            throw std::runtime_error("Refusing to overwrite an existing fixture");
        const auto valid = YAML::LoadFile(argv[1]);
        for (int test = 0; test < 6; ++test)
        {
            auto node = YAML::Clone(valid);
            if (test == 0) node = YAML::Node(YAML::NodeType::Sequence);
            if (test == 1) node["grass_block"]["id"] = 1;
            if (test == 2) node.remove("water_still");
            if (test == 3) node["grass_block"]["id"] = -1;
            if (test == 4) node.remove("birch_planks");
            if (test == 5) node.remove("cobblestone");
            std::ofstream output(fixture, std::ios::binary | std::ios::trunc);
            output << YAML::Dump(node);
            output.close();
            if (!output)
                throw std::runtime_error("Cannot write block configuration fixture");
            rejected = false;
            try { SymoCraft::LoadBlocks(fixture.string()); }
            catch (const std::exception&) { rejected = true; }
            if (!rejected)
                throw std::runtime_error("Malformed block configuration was accepted");
            if (SymoCraft::get_block_id("grass_block") != 2)
                throw std::runtime_error("Failed configuration replaced the last valid state");
        }
        fs::remove(fixture);
        std::cout << "Block configuration contracts passed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Block configuration test failed: " << error.what() << '\n';
        // Retain a failing fixture for diagnosis; never delete a preexisting file.
        return 1;
    }
}
