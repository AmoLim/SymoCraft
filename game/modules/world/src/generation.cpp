#include <symocraft/world/generation.h>
#include "chunk_store.h"
#include <symocraft/world/block.h>
#include <fast_noise_lite/FastNoiseLite.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <chrono>

namespace SymoCraft::Generation {
    struct Generator::NoiseState {
        // The bundled sampler has a non-const API; this is not a thread-safety contract.
        std::array<FastNoiseLite, 3> noises;
    };

    namespace {
        float Remap(float x, float in_min, float in_max, float out_min, float out_max)
        {
            return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
        }

        constexpr std::array frequencies{0.00573f, 0.02f, 0.1f};
        constexpr std::array weights{1.0f, 0.2f, 0.03f};
        constexpr std::uint32_t vegetation_salt = 0xa511e9b3u;

        std::uint32_t Mix(std::uint32_t value)
        {
            value ^= value >> 16;
            value *= 0x7feb352du;
            value ^= value >> 15;
            value *= 0x846ca68bu;
            return value ^ (value >> 16);
        }

        std::vector<glm::ivec2> SortedCoordinates()
        {
            std::vector<glm::ivec2> coordinates;
            coordinates.reserve(ChunkManager::GetAllChunks().size());
            for (const auto& [coord, chunk] : ChunkManager::GetAllChunks()) coordinates.push_back(coord);
            std::sort(coordinates.begin(), coordinates.end(), [](const auto& a, const auto& b) {
                return a.x != b.x ? a.x < b.x : a.y < b.y;
            });
            return coordinates;
        }
    }

    Generator::Generator(Settings settings) : settings_(settings), noise_(std::make_unique<NoiseState>())
    {
        if (settings.radius < 2 || settings.radius > 10)
            throw std::invalid_argument("Generation radius must be between 2 and 10");
        for (std::size_t i = 0; i < noise_->noises.size(); ++i) {
            // Mask before conversion: no implementation-defined unsigned-to-signed seed conversion.
            noise_seeds_[i] = static_cast<int>(Mix(settings.seed ^ (0x9e3779b9u * (static_cast<std::uint32_t>(i) + 1))) & 0x7fffffffu);
            auto& noise = noise_->noises[i];
            noise.SetSeed(noise_seeds_[i]);
            noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
            noise.SetFractalType(FastNoiseLite::FractalType_FBm);
            noise.SetFractalOctaves(8);
            noise.SetFractalLacunarity(1.6f);
            noise.SetFractalGain(0.5f);
            noise.SetFrequency(frequencies[i]);
        }
    }

    Generator::~Generator() = default;

    float Generator::Height(int x, int z) const
    {
        float blended = 0.0f;
        float weight_sum = 0.0f;
        for (std::size_t i = 0; i < noise_->noises.size(); ++i) {
            blended += Remap(noise_->noises[i].GetNoise(static_cast<float>(x) / 1.5f, static_cast<float>(z) / 1.5f),
                             -1.0f, 1.0f, 0.0f, 1.0f) * weights[i];
            weight_sum += weights[i];
        }
        return Remap(std::pow(std::clamp(blended / weight_sum, 0.0f, 1.0f), 1.19f),
                     0.0f, 1.0f, min_biome_height, max_biome_height);
    }

    std::mt19937 Generator::VegetationRandom(int x, int z) const
    {
        const auto local_seed = Mix(Mix(settings_.seed ^ vegetation_salt) ^
            Mix(static_cast<std::uint32_t>(x)) ^ Mix(static_cast<std::uint32_t>(z) ^ 0x63d83595u));
        return std::mt19937{local_seed};
    }

    std::uint32_t RandomSeed() { return static_cast<std::uint32_t>(std::random_device{}()); }

    void Populate(const Generator& generator, Timings* timings)
    {
        const int radius = generator.Config().radius;
        const auto coordinates = SortedCoordinates();
        const auto side = static_cast<std::size_t>(radius * 2 + 1);
        if (coordinates.size() != side * side) throw std::invalid_argument("Generation requires a complete square of chunks");
        for (int x = -radius; x <= radius; ++x)
            for (int z = -radius; z <= radius; ++z)
                if (!ChunkManager::GetChunk(glm::ivec2{x, z})) throw std::invalid_argument("Generation square has a missing chunk");
        ChunkManager::RearrangeChunkNeighborPointers();
        const auto terrain_start = std::chrono::steady_clock::now();
        for (const auto coord : coordinates) ChunkManager::GetChunk(coord)->GenerateTerrain(generator);
        const auto vegetation_start = std::chrono::steady_clock::now();
        // Cross-chunk overlaps use last-write-wins in ascending chunk x,z / local x,z order.
        if (generator.Config().vegetation)
            for (const auto coord : coordinates) ChunkManager::GetChunk(coord)->GenerateVegetation(generator);
        if (timings) {
            timings->terrain_ms = std::chrono::duration<double, std::milli>(vegetation_start - terrain_start).count();
            timings->vegetation_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - vegetation_start).count();
        }
    }

    void Build(const Settings& settings, Timings* timings)
    {
        const Generator generator(settings); // Validate before touching the existing world.
        const auto start = std::chrono::steady_clock::now();
        ChunkManager::FreeAllChunks();
        for (int x = -settings.radius; x <= settings.radius; ++x)
            for (int z = -settings.radius; z <= settings.radius; ++z) ChunkManager::CreateChunk({x, z});
        if (timings) timings->allocation_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        Populate(generator, timings);
    }

    std::string BlockDigest()
    {
        std::uint64_t hash = 14695981039346656037ull;
        const auto append = [&hash](std::uint32_t value, unsigned bytes) {
            for (unsigned i = 0; i < bytes; ++i) {
                hash ^= (value >> (i * 8)) & 0xffu;
                hash *= 1099511628211ull;
            }
        };
        for (const auto coord : SortedCoordinates()) {
            append(static_cast<std::uint32_t>(coord.x), 4);
            append(static_cast<std::uint32_t>(coord.y), 4);
            const auto& blocks = ChunkManager::GetChunk(coord)->m_local_blocks;
            if (blocks.size() != k_chunk_length * k_chunk_width * k_chunk_height)
                throw std::logic_error("Cannot digest incomplete chunk storage");
            // Fixed y,x,z order; fields serialized explicitly, never C++ object padding.
            for (const auto& block : blocks) {
                append(block.block_id, 2);
                append(block.lightLevel, 2);
                append(static_cast<std::uint16_t>(block.lightColor), 2);
                append(block.bitwise_compressed_data, 2);
            }
        }
        std::ostringstream out;
        out << "fnv1a64:" << std::hex << std::setfill('0') << std::setw(16) << hash;
        return out.str();
    }

    glm::vec3 FindSpawnPosition()
    {
        for (int radius = 0; radius <= 64; radius += 4)
            for (int x = -radius; x <= radius; x += 4)
                for (int z = -radius; z <= radius; z += 4) {
                    if (std::max(std::abs(x), std::abs(z)) != radius) continue;
                    for (int y = k_chunk_height - 4; y >= 0; --y) {
                        const auto block = ChunkManager::GetBlock({x, y, z});
                        if (block.block_id == 9) break;
                        if (!get_block(block.block_id).m_is_solid) continue;
                        if (block.block_id >= 2 && block.block_id <= 5) return {x + 0.5f, y + 1.95f, z + 0.5f};
                        break;
                    }
                }
        throw std::runtime_error("Cannot find a safe player spawn near the world center");
    }

    Data::Value Describe(const Settings& settings)
    {
        const Generator generator(settings);
        Data::Value node;
        node["generator_version"] = Version;
        node["seed"] = settings.seed;
        node["radius"] = settings.radius;
        node["vegetation"] = settings.vegetation;
        node["chunk_dimensions"].push_back(k_chunk_length);
        node["chunk_dimensions"].push_back(k_chunk_height);
        node["chunk_dimensions"].push_back(k_chunk_width);
        node["noise_type"] = "FastNoiseLite-1.0.1/OpenSimplex2/FBm";
        node["octaves"] = 8;
        node["lacunarity"] = 1.6f;
        node["gain"] = 0.5f;
        node["coordinate_scale"] = 1.5f;
        node["height_exponent"] = 1.19f;
        node["min_height"] = min_biome_height;
        node["max_height"] = max_biome_height;
        node["sea_level"] = sea_level;
        for (std::size_t i = 0; i < frequencies.size(); ++i) {
            node["noise_seeds"].push_back(generator.NoiseSeeds()[i]);
            node["frequencies"].push_back(frequencies[i]);
            node["weights"].push_back(weights[i]);
        }
        node["vegetation_rng"] = "mix32-v1/seed+chunk-x+chunk-z/mt19937/raw-modulo";
        node["vegetation_candidate_extent"] = 10;
        node["vegetation_roll"] = "raw%100>98";
        node["write_order"] = "chunk-x,chunk-z; all-terrain-before-vegetation; last-write-wins";
        node["digest_schema"] = "fnv1a64-v1/le-coord-i32x2/block-u16x4/chunk-x,z/block-y,x,z";
        for (int id = 1; id <= 11; ++id) {
            const auto& format = get_block(id);
            Data::Value block;
            block["id"] = id;
            block["textures"].push_back(format.m_top_texture);
            block["textures"].push_back(format.m_side_texture);
            block["textures"].push_back(format.m_bottom_texture);
            block["transparent"] = format.m_is_transparent;
            block["solid"] = format.m_is_solid;
            block["blendable"] = format.m_is_blendable;
            block["light_source"] = format.m_is_lightSource;
            block["light_level"] = format.m_light_level;
            node["block_formats"].push_back(block);
        }
        return node;
    }
}
