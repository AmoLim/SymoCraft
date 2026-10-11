#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <memory>
#include <symocraft/foundation/document.h>
#include <symocraft/foundation/math.h>

namespace SymoCraft::Generation {
    inline constexpr unsigned Version = 1;
    struct Settings {
        std::uint32_t seed = 0;
        int radius = 10;
        bool vegetation = true;
    };

    class Generator {
    public:
        explicit Generator(Settings settings);
        ~Generator();
        Generator(const Generator&) = delete;
        Generator& operator=(const Generator&) = delete;
        Generator(Generator&&) = delete;
        Generator& operator=(Generator&&) = delete;
        float Height(int x, int z) const;
        std::mt19937 VegetationRandom(int chunk_x, int chunk_z) const;
        const Settings& Config() const { return settings_; }
        const std::array<int, 3>& NoiseSeeds() const { return noise_seeds_; }
    private:
        Settings settings_;
        struct NoiseState;
        std::unique_ptr<NoiseState> noise_;
        std::array<int, 3> noise_seeds_{};
    };

    std::uint32_t RandomSeed();
    struct Timings { double allocation_ms{}, terrain_ms{}, vegetation_ms{}; };
}
