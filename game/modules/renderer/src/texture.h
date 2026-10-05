#ifndef TEXTURE_H
#define TEXTURE_H
#include <symocraft/foundation/types.h>
#include <glad/glad.h>
#include <string>
#include <string_view>

using layer_t = int;

class Texture
{
public:
    Texture() = default;
    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    Texture CreateRegularTexture(std::string_view filepath, bool pixelated);
    void Destroy() noexcept;

    std::string m_filepath;
    int m_width{};
    int m_height{};
    int m_channel_amount{};
    GLint m_texture_format{};
    GLint m_internal_format{};
    uint32 m_texture_Id{};
};

class TextureArray : public Texture
{
public:
    TextureArray CreateAtlasSlice(std::string_view filepath, bool pixelated);

    uint16 layer_amount{};
};

class CubeMap : public Texture
{
public:
    CubeMap CreateCubeMap(std::string_view filepath);
};

#endif
