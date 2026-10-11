#include "symocraft/assets/image.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    struct Fixture {
        std::filesystem::path directory;
        Fixture()
            : directory(std::filesystem::temp_directory_path() /
                ("symocraft-image-write-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())))
        {
            if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Expected a new image test directory");
        }
        ~Fixture() { std::error_code error; std::filesystem::remove_all(directory, error); }
    };
    template<typename Action>
    void Reject(Action action)
    {
        try { action(); } catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("Invalid PNG input accepted");
    }
}

int main()
{
    using namespace SymoCraft::Assets;
    try {
        const Fixture fixture;
        for (int channels : {1, 2, 3, 4}) {
            constexpr int width = 3, height = 2;
            const auto row_bytes = static_cast<std::size_t>(width) * channels;
            std::vector<std::uint8_t> pixels(row_bytes * height);
            for (std::size_t i = 0; i < pixels.size(); ++i)
                pixels[i] = static_cast<std::uint8_t>((i * 37 + channels * 13) % 256);
            const auto original = pixels;
            const auto normal_path = fixture.directory / ("top-left-" + std::to_string(channels) + ".png");
            WritePng(normal_path, width, height, channels, pixels);
            const auto normal = DecodeImage(normal_path);
            Require(normal.width == width && normal.height == height && normal.channels == channels,
                    "PNG writer changed dimensions or channels");
            Require(normal.pixels == original, "PNG writer changed tightly packed pixel or alpha bytes");

            const auto flipped_path = fixture.directory / ("bottom-left-" + std::to_string(channels) + ".png");
            WritePng(flipped_path, width, height, channels, pixels, true);
            auto expected = original;
            std::copy_n(original.data() + row_bytes, row_bytes, expected.data());
            std::copy_n(original.data(), row_bytes, expected.data() + row_bytes);
            Require(DecodeImage(flipped_path).pixels == expected, "PNG writer did not flip rows exactly once");
            Require(pixels == original, "PNG writer modified caller-owned pixels");

            const auto after_path = fixture.directory / ("after-flip-" + std::to_string(channels) + ".png");
            WritePng(after_path, width, height, channels, pixels);
            Require(DecodeImage(after_path).pixels == original, "PNG flip state leaked into a later call");
        }

        const auto invalid_path = fixture.directory / "invalid.png";
        const std::vector<std::uint8_t> pixels{17, 42, 91, 255};
        Reject([&] { WritePng(invalid_path, 0, 1, 4, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 0, 4, pixels); });
        Reject([&] { WritePng(invalid_path, -1, 1, 4, pixels); });
        Reject([&] { WritePng(invalid_path, 1, -1, 4, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 1, 0, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 1, -1, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 1, 5, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 1, 3, pixels); });
        Reject([&] { WritePng(invalid_path, 2, 1, 4, pixels); });
        Reject([&] { WritePng(invalid_path, 1, 1, 4, {}); });
        Reject([&] { WritePng(invalid_path, std::numeric_limits<int>::max(), 1, 4, pixels); });
        Reject([&] { WritePng(invalid_path, 1, std::numeric_limits<int>::max(), 4, pixels); });
        Reject([&] { WritePng(invalid_path, 16384, 16384, 4, pixels); });
        Require(!std::filesystem::exists(invalid_path), "Invalid PNG input created an output file");

        for (const auto path : {fixture.directory, fixture.directory / "missing-parent" / "image.png"}) {
            bool rejected = false;
            try { WritePng(path, 1, 1, 4, pixels); }
            catch (const std::runtime_error&) { rejected = true; }
            Require(rejected, "PNG writer did not report an output file failure");
        }
        std::cout << "CPU PNG packing, channels, flip isolation and error contracts passed.\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
