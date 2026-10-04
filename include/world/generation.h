#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <fast_noise_lite/FastNoiseLite.h>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

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
        float Height(int x, int z) const;
        std::mt19937 VegetationRandom(int chunk_x, int chunk_z) const;
        const Settings& Config() const { return settings_; }
        const std::array<int, 3>& NoiseSeeds() const { return noise_seeds_; }
    private:
        Settings settings_;
        // Bundled FastNoiseLite 1.0.1 has a non-const sampling API; not a thread-safety contract.
        mutable std::array<FastNoiseLite, 3> noises_;
        std::array<int, 3> noise_seeds_{};
    };

    std::uint32_t RandomSeed();
    struct Timings { double allocation_ms{}, terrain_ms{}, vegetation_ms{}; };
    void Build(const Settings& settings, Timings* timings = nullptr);
    // Existing complete square only; sorts by coordinates, never by map iteration order.
    void Populate(const Generator& generator, Timings* timings = nullptr);
    std::string BlockDigest();
    glm::vec3 FindSpawnPosition();
    YAML::Node Describe(const Settings& settings);
}
