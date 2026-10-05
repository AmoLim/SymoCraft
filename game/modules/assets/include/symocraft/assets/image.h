#pragma once
#include <cstdint>
#include <filesystem>
#include <vector>

namespace SymoCraft::Assets {
    struct Image {
        int width = 0;
        int height = 0;
        int channels = 0;
        std::vector<std::uint8_t> pixels;
    };
    std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path);
    // Preserves the file's channel count with tightly packed bytes; throws on failure.
    Image DecodeImage(const std::filesystem::path& path, bool flip_vertical = false);
}
