#include "symocraft/assets/image.h"
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    struct Fixture {
        std::filesystem::path path;
        ~Fixture() { std::error_code error; std::filesystem::remove(path, error); }
    };
}
int main(int argc, char** argv) {
    using namespace SymoCraft::Assets;
    try {
        Require(argc == 2, "Expected atlas path");
        const auto original = DecodeImage(argv[1], false);
        const auto flipped = DecodeImage(argv[1], true);
        Require(original.width > 0 && original.height > 0 && (original.channels == 3 || original.channels == 4), "Atlas image contract failed");
        Require(original.pixels.size() == static_cast<std::size_t>(original.width) * original.height * original.channels, "Unexpected pixel packing");
        const auto row = static_cast<std::size_t>(original.width) * original.channels;
        Require(std::equal(original.pixels.begin(), original.pixels.begin() + row,
                           flipped.pixels.end() - row), "Vertical flip changed pixel data");
        Require(DecodeImage(argv[1], false).pixels == original.pixels, "Decode flip state leaked into later call");
        const auto stamp = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        for (int channels : {1, 3}) {
            Fixture fixture{std::filesystem::current_path() / ("image-contract-" + stamp + '-' + std::to_string(channels) + ".pnm")};
            {
                std::ofstream file(fixture.path, std::ios::binary);
                file << (channels == 1 ? "P5\n1 1\n255\n" : "P6\n1 1\n255\n");
                for (int i = 0; i < channels; ++i) file.put(static_cast<char>(17 + i));
            }
            const auto decoded = DecodeImage(fixture.path);
            Require(decoded.channels == channels && decoded.pixels.size() == channels,
                    "Decoder changed source channels or GPU format semantics");
        }
        bool rejected = false;
        try { DecodeImage(std::filesystem::path(argv[1]).parent_path() / "missing-image-for-test.png"); }
        catch (const std::runtime_error&) { rejected = true; }
        Require(rejected, "Missing image did not report failure");
        std::cout << "CPU image decoding, ownership and error contracts passed.\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
