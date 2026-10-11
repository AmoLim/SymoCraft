#include <symocraft/world/generation.h>
#include <symocraft/world/constants.h>
#include "world_internal.h"
#include <fast_noise_lite/FastNoiseLite.h>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace SymoCraft::Generation {
    struct Generator::NoiseState { std::array<FastNoiseLite, 3> noises; };
    namespace {
        float Remap(float x, float low, float high, float out_low, float out_high)
        { return (x - low) * (out_high - out_low) / (high - low) + out_low; }
        constexpr std::array frequencies{0.00573f, 0.02f, 0.1f};
        constexpr std::array weights{1.0f, 0.2f, 0.03f};
        std::uint32_t Mix(std::uint32_t value)
        {
            value ^= value >> 16; value *= 0x7feb352du;
            value ^= value >> 15; value *= 0x846ca68bu;
            return value ^ (value >> 16);
        }
    }
    Generator::Generator(Settings settings) : settings_(settings)
    {
        if (settings.radius < 2 || settings.radius > 10)
            throw std::invalid_argument("Generation radius must be between 2 and 10");
        noise_ = std::make_unique<NoiseState>();
        for (std::size_t i = 0; i < noise_->noises.size(); ++i) {
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
        float blended = 0.0f, weight_sum = 0.0f;
        for (std::size_t i = 0; i < noise_->noises.size(); ++i) {
            blended += Remap(noise_->noises[i].GetNoise(static_cast<float>(x) / 1.5f, static_cast<float>(z) / 1.5f),
                -1.0f, 1.0f, 0.0f, 1.0f) * weights[i];
            weight_sum += weights[i];
        }
        return Remap(std::pow(std::clamp(blended / weight_sum, 0.0f, 1.0f), 1.19f), 0.0f, 1.0f, min_biome_height, max_biome_height);
    }
    std::mt19937 Generator::VegetationRandom(int x, int z) const
    {
        return std::mt19937{Mix(Mix(settings_.seed ^ 0xa511e9b3u) ^
            Mix(static_cast<std::uint32_t>(x)) ^ Mix(static_cast<std::uint32_t>(z) ^ 0x63d83595u))};
    }
    std::uint32_t RandomSeed() { return static_cast<std::uint32_t>(std::random_device{}()); }
    Data::Value Metadata(const Settings& settings, const std::array<int, 3>& noise_seeds,
        const World::BlockDefinition& definition)
    {
        Data::Value node;
        node["generator_version"] = Version; node["seed"] = settings.seed;
        node["radius"] = settings.radius; node["vegetation"] = settings.vegetation;
        node["chunk_dimensions"].push_back(k_chunk_length);
        node["chunk_dimensions"].push_back(k_chunk_height);
        node["chunk_dimensions"].push_back(k_chunk_width);
        node["noise_type"] = "FastNoiseLite-1.0.1/OpenSimplex2/FBm";
        node["octaves"] = 8; node["lacunarity"] = 1.6f; node["gain"] = 0.5f;
        node["coordinate_scale"] = 1.5f; node["height_exponent"] = 1.19f;
        node["min_height"] = min_biome_height; node["max_height"] = max_biome_height; node["sea_level"] = sea_level;
        for (std::size_t i = 0; i < frequencies.size(); ++i) {
            node["noise_seeds"].push_back(noise_seeds[i]);
            node["frequencies"].push_back(frequencies[i]); node["weights"].push_back(weights[i]);
        }
        node["vegetation_rng"] = "mix32-v1/seed+chunk-x+chunk-z/mt19937/raw-modulo";
        node["vegetation_candidate_extent"] = 10; node["vegetation_roll"] = "raw%100>98";
        node["write_order"] = "chunk-x,chunk-z; all-terrain-before-vegetation; last-write-wins";
        node["digest_schema"] = "fnv1a64-v1/le-coord-i32x2/block-u16x4/chunk-x,z/block-y,x,z";
        for (World::BlockId id = 1; id <= 11; ++id) {
            const auto rule = *definition.Find(id);
            Data::Value block;
            block["id"] = static_cast<int>(id);
            block["textures"].push_back(rule.m_top_texture); block["textures"].push_back(rule.m_side_texture);
            block["textures"].push_back(rule.m_bottom_texture);
            block["transparent"] = rule.m_is_transparent; block["solid"] = rule.m_is_solid;
            block["blendable"] = rule.m_is_blendable; block["light_source"] = rule.m_is_lightSource;
            block["light_level"] = rule.m_light_level;
            node["block_formats"].push_back(block);
        }
        return node;
    }
}

namespace SymoCraft::World {
    void VoxelWorld::Impl::WriteGenerated(BlockCoord position, BlockId id)
    {
        const auto converted_x = position.x / 16 - (position.x % 16 < 0 ? 1 : 0);
        const auto converted_z = position.z / 16 - (position.z % 16 < 0 ? 1 : 0);
        auto* chunk = Lookup({converted_x, converted_z});
        if (!chunk || position.y < 0 || position.y >= 256) return;
        const auto rule = definition.Find(id);
        if (!rule) throw std::logic_error("Generation has unknown block ID");
        auto& block = chunk->blocks[Detail::Chunk::Index(position.x - converted_x * 16, position.y, position.z - converted_z * 16)];
        block.block_id = id;
        block.SetTransparency(rule->m_is_transparent); block.SetBlendability(rule->m_is_blendable); block.SetLightSource(rule->m_is_lightSource);
    }
    void VoxelWorld::Impl::Populate(const Generation::Generator& generator, Generation::Timings* timings)
    {
        const auto terrain_start = std::chrono::steady_clock::now();
        for (const auto& owner : chunks) {
            auto& chunk = *owner;
            const int world_x = chunk.coordinate.x * 16, world_z = chunk.coordinate.y * 16;
            for (int z = 0; z < 16; ++z)
                for (int x = 0; x < 16; ++x) {
                    const int height = std::clamp(static_cast<int>(generator.Height(x + world_x, z + world_z)), 0, 255);
                    const int stone = std::max(0, height - 6);
                    for (int y = 0; y < 256; ++y) {
                        auto& block = chunk.blocks[Detail::Chunk::Index(x,y,z)];
                        if (chunk.fringe) {
                            block.block_id = 1;
                            block.SetTransparency(true); block.SetBlendability(false); block.SetLightSource(false);
                            block.SetLightColor({255,255,255});
                            continue;
                        }
                        BlockId id = 1;
                        if (y == 0 || y < stone) id = 5;
                        else if (y < height) id = 4;
                        else if (y == height) id = height < sea_level + 2 ? BlockId{3} : BlockId{2};
                        else if (y >= min_biome_height && y < sea_level) id = 9;
                        block.block_id = id;
                        block.SetTransparency(id == 1); block.SetBlendability(id == 9); block.SetLightSource(false);
                    }
                }
        }
        const auto vegetation_start = std::chrono::steady_clock::now();
        // All terrain is complete before any cross-chunk vegetation write, in ascending x,z order.
        if (settings.vegetation)
            for (const auto& owner : chunks) {
                const auto& chunk = *owner;
                if (chunk.fringe) continue;
                const int wx = chunk.coordinate.x * 16, wz = chunk.coordinate.y * 16;
                auto random = generator.VegetationRandom(chunk.coordinate.x, chunk.coordinate.y);
                for (int x = 0; x < 10; ++x)
                    for (int z = 0; z < 10; ++z) {
                        if (random() % 100 <= 98) continue;
                        const auto y = static_cast<uint16>(generator.Height(x + wx, z + wz) + 1);
                        if (y <= sea_level + 2) continue;
                        const uint16 trunk_top = static_cast<uint16>((random() % 3) + 3);
                        const uint16 ring_top = trunk_top + 1, ring_bottom = trunk_top - 2;
                        if (y + 1 + ring_top >= 256) continue;
                        for (int t = 0; t <= trunk_top; ++t) WriteGenerated({x + wx, t + y, z + wz}, 6);
                        int leaf_y = ring_bottom + y;
                        for (int ring = 0; ring < 2; ++ring, ++leaf_y)
                            for (int lx = x - 2; lx <= x + 2; ++lx)
                                for (int lz = z - 2; lz <= z + 2; ++lz) {
                                    if ((lx == x-2 || lx == x+2) && (lz == z-2 || lz == z+2) && random() % 5 < 2) continue;
                                    WriteGenerated({lx + wx, leaf_y, lz + wz}, 7);
                                }
                        for (int lx = x-1; lx <= x+1; ++lx)
                            for (int lz = z-1; lz <= z+1; ++lz) {
                                if ((lx == x-1 || lx == x+1) && (lz == z-1 || lz == z+1) && random() % 6 < 1) continue;
                                WriteGenerated({lx + wx, leaf_y, lz + wz}, 7);
                            }
                        ++leaf_y;
                        for (int lx = x-1; lx <= x+1; ++lx)
                            for (int lz = z-1; lz <= z+1; ++lz) {
                                if ((lx == x-1 || lx == x+1) && (lz == z-1 || lz == z+1)) continue;
                                WriteGenerated({lx + wx, leaf_y, lz + wz}, 7);
                            }
                    }
            }
        if (timings) {
            timings->terrain_ms = std::chrono::duration<double,std::milli>(vegetation_start - terrain_start).count();
            timings->vegetation_ms = std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now() - vegetation_start).count();
        }
        noise_seeds = generator.NoiseSeeds();
    }
    Data::Value VoxelWorld::Describe() const
    { return Generation::Metadata(impl_->settings, impl_->noise_seeds, impl_->definition); }
}
