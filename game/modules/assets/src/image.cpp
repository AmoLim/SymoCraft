#include "symocraft/assets/image.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace SymoCraft::Assets {
    std::vector<std::uint8_t> ReadBytes(const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) throw std::runtime_error("Cannot read asset: " + path.string());
        const auto length = file.tellg();
        if (length < 0 || static_cast<std::uintmax_t>(length) > std::numeric_limits<std::size_t>::max())
            throw std::runtime_error("Invalid asset length: " + path.string());
        std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
        file.seekg(0);
        if (!bytes.empty() && !file.read(reinterpret_cast<char*>(bytes.data()), length))
            throw std::runtime_error("Incomplete asset read: " + path.string());
        return bytes;
    }
    Image DecodeImage(const std::filesystem::path& path, bool flip_vertical) {
        const auto bytes = ReadBytes(path);
        if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("Image input is too large: " + path.string());
        stbi_set_flip_vertically_on_load_thread(flip_vertical ? 1 : 0);
        Image image;
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> decoded(
            stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                                  &image.width, &image.height, &image.channels, 0),
            &stbi_image_free);
        if (!decoded) {
            const char* reason = stbi_failure_reason();
            throw std::runtime_error("Cannot decode image: " + path.string() + ": " +
                                     (reason ? reason : "unknown decoder error"));
        }
        const auto length = static_cast<std::size_t>(image.width) * image.height * image.channels;
        image.pixels.assign(decoded.get(), decoded.get() + length);
        return image;
    }
}
