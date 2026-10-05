#pragma once
#include <symocraft/foundation/types.h>
#include <symocraft/foundation/math.h>
#include <cstddef>
#include <string_view>

namespace SymoCraft
{
    struct BlockFormat {
        uint16 m_top_texture;
        uint16 m_side_texture;
        uint16 m_bottom_texture;
        bool m_is_transparent;
        bool m_is_solid;
        bool m_is_blendable;
        bool m_is_lightSource;
        int16 m_light_level;
    };

    void LoadBlocks(std::string_view block_format_config);
    void ValidateBlockTextures(std::size_t layer_count);
    uint16 get_block_id(std::string_view block_name);
    const BlockFormat& get_block(int block_id);
    const BlockFormat& get_block(std::string_view name);

    class Block {
    public:
        uint16 block_id;
        uint16 lightLevel;
        int16 lightColor;

        // Bit 1 IsTransparent
        // Bit 2 IsBlendable
        // Bit 3 IsLightSource
        uint16 bitwise_compressed_data;

        inline bool operator==(const Block& b) const
        {
            return block_id == b.block_id;
        }

        inline bool operator!=(const Block& b) const
        {
            return block_id != b.block_id;
        }

        inline bool IsTransparent() const
        {
            return (bitwise_compressed_data & (1 << 0));
        }

        inline bool IsBlendable() const
        {
            return (bitwise_compressed_data & (1 << 1));
        }

        inline bool IsLightSource() const
        {
            return (bitwise_compressed_data & (1 << 2));
        }

        inline bool IsLightPassable() const
        {
            return IsLightSource() || IsTransparent();
        }

        inline void SetTransparency(bool is_transparent)
        {
            // Clear the bit
            bitwise_compressed_data &= ~(1 << 0);
            // Set the bit if needed
            bitwise_compressed_data |= is_transparent ? (1 << 0) : 0;
        }

        inline void SetBlendability(bool is_blendable)
        {
            // Clear the bit
            bitwise_compressed_data &= ~(1 << 1);
            // Set the bit if needed
            bitwise_compressed_data |= is_blendable ? (1 << 1) : 0;
        }

        inline void SetLightSource(bool is_lightSource)
        {
            // Clear the bit
            bitwise_compressed_data &= ~(1 << 2);
            // Set the bit if needed
            bitwise_compressed_data |= is_lightSource ? (1 << 2) : 0;
        }

        inline void SetLightColor(const glm::ivec3& color)
        {
            // Convert from number between 0-255 to number between 0-7
            lightColor =
                    (( ((int)((float)color.r / 255.0f) * 7) << 0) & 0x7)  |
                    (( ((int)((float)color.g / 255.0f) * 7) << 3) & 0x38) |
                    (( ((int)((float)color.b / 255.0f) * 7) << 6) & 0x1C0);
        }

        inline glm::ivec3 GetLightColor() const
        {
            // Convert from number between 0-7 to number between 0-255
            return {
                    (int)(((float)((lightColor & 0x7)   >> 0) / 7.0f) * 255.0f),  // R
                    (int)(((float)((lightColor & 0x38)  >> 3) / 7.0f) * 255.0f),  // G
                    (int)(((float)((lightColor & 0x1C0) >> 6) / 7.0f) * 255.0f)   // B
            };
        }

        inline glm::ivec3 GetCompressedLightColor() const
        {
            return {((lightColor & 0x7) >> 0),  // R
                    ((lightColor & 0x38) >> 3), // G
                    ((lightColor & 0x1C0) >> 6) // B
            };
        }
    };
}
