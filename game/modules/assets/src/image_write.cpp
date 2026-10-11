#include "symocraft/assets/image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_WRITE_STATIC
#define STBIWDEF static inline
#define STBI_WRITE_NO_STDIO
#include <stb/stb_image_write.h>
#include <algorithm>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace SymoCraft::Assets {
    void WritePng(const std::filesystem::path& path, int width, int height, int channels,
                  std::span<const std::uint8_t> pixels, bool flip_vertical)
    {
        if (width <= 0 || height <= 0 || channels < 1 || channels > 4)
            throw std::invalid_argument("PNG dimensions must be positive with 1..4 channels");
        const auto encoder_max = static_cast<std::size_t>(std::numeric_limits<int>::max());
        if (static_cast<std::size_t>(width) > encoder_max / static_cast<std::size_t>(channels))
            throw std::invalid_argument("PNG row stride is too large for the encoder");
        const auto row_bytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(channels);
        const auto rows = static_cast<std::size_t>(height);
        // stb uses signed int row/filter lengths and doubles its encoded output capacity.
        if (rows > (encoder_max / 4) / (row_bytes + 1))
            throw std::invalid_argument("PNG image is too large for the encoder");
        const auto byte_count = row_bytes * rows;
        if (pixels.size() != byte_count)
            throw std::invalid_argument("PNG pixel bytes do not match the tightly packed dimensions");

        std::vector<std::uint8_t> flipped;
        if (flip_vertical) {
            flipped.resize(byte_count);
            for (std::size_t row = 0; row < rows; ++row)
                std::copy_n(pixels.data() + row * row_bytes, row_bytes,
                            flipped.data() + (rows - row - 1) * row_bytes);
            pixels = flipped;
        }
        std::ofstream file(path, std::ios::binary | std::ios::trunc);
        if (!file) throw std::runtime_error("Cannot write PNG: " + path.string());
        const auto write = [](void* context, void* data, int size) {
            static_cast<std::ofstream*>(context)->write(static_cast<const char*>(data), size);
        };
        const auto encoded = stbi_write_png_to_func(write, &file, width, height, channels,
                                                    pixels.data(), static_cast<int>(row_bytes));
        file.close();
        if (!encoded || !file)
            throw std::runtime_error("Cannot encode or complete PNG: " + path.string());
    }
}
