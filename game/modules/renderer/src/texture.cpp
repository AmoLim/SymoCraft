#include "texture.h"
#include <symocraft/assets/image.h>

#include <memory>
#include <stdexcept>
#include <utility>

namespace
{
    void CheckTextureError(const std::string& operation)
    {
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR)
            throw std::runtime_error(operation + ": OpenGL error " + std::to_string(error));
    }
}

Texture::~Texture()
{
    Destroy();
}

Texture::Texture(Texture&& other) noexcept
{
    *this = std::move(other);
}

Texture& Texture::operator=(Texture&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        m_filepath = std::move(other.m_filepath);
        m_width = std::exchange(other.m_width, 0);
        m_height = std::exchange(other.m_height, 0);
        m_channel_amount = std::exchange(other.m_channel_amount, 0);
        m_texture_format = std::exchange(other.m_texture_format, 0);
        m_internal_format = std::exchange(other.m_internal_format, 0);
        m_texture_Id = std::exchange(other.m_texture_Id, 0);
    }
    return *this;
}

void Texture::Destroy() noexcept
{
    if (m_texture_Id)
    {
        glDeleteTextures(1, &m_texture_Id);
        m_texture_Id = 0;
    }
}

Texture Texture::CreateRegularTexture(std::string_view filepath, bool pixelated)
{
    Texture res;
    res.m_filepath = filepath;
    const auto image = SymoCraft::Assets::DecodeImage(res.m_filepath, true);
    res.m_width = image.width;
    res.m_height = image.height;
    res.m_channel_amount = image.channels;
    if (res.m_width <= 0 || res.m_height <= 0)
        throw std::runtime_error("Texture has invalid dimensions: " + res.m_filepath);
    GLint max_size = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
    if (res.m_width > max_size || res.m_height > max_size)
        throw std::runtime_error("Texture dimensions exceed the GPU limit: " + res.m_filepath);

    res.m_texture_format = GL_RGBA;
    res.m_internal_format = GL_RGBA8;
    if (res.m_channel_amount == 3)
    {
        res.m_texture_format = GL_RGB;
        res.m_internal_format = GL_RGB8;
    }
    else if (res.m_channel_amount != 4)
    {
        throw std::runtime_error("Texture must have RGB or RGBA channels: " + res.m_filepath);
    }

    // Generate and bind the texture object
    glCreateTextures(GL_TEXTURE_2D, 1, &res.m_texture_Id);
    if (!res.m_texture_Id)
        throw std::runtime_error("OpenGL could not create texture: " + res.m_filepath);

    glTextureParameteri(res.m_texture_Id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(res.m_texture_Id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(res.m_texture_Id, GL_TEXTURE_MIN_FILTER, pixelated ? GL_NEAREST : GL_LINEAR);
    glTextureParameteri(res.m_texture_Id, GL_TEXTURE_MAG_FILTER, pixelated ? GL_NEAREST : GL_LINEAR);

    glTextureStorage2D(res.m_texture_Id, 1, res.m_internal_format, res.m_width, res.m_height);
    CheckTextureError("Allocating texture " + res.m_filepath);
    GLint unpack_alignment = 4;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTextureSubImage2D(res.m_texture_Id, 0, 0, 0, res.m_width, res.m_height,
                       res.m_texture_format, GL_UNSIGNED_BYTE, image.pixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);
    CheckTextureError("Uploading texture " + res.m_filepath);

    return res;
}

TextureArray TextureArray::CreateAtlasSlice(std::string_view filepath, bool pixelated)
{
    // Set up
    Texture tex_atlas = CreateRegularTexture( filepath, pixelated );

    constexpr int tile_width = 64, tile_height = 64;
    if (tex_atlas.m_width % tile_width != 0 || tex_atlas.m_height % tile_height != 0)
        throw std::runtime_error("Texture atlas dimensions must be multiples of 64: " + tex_atlas.m_filepath);
    const int tile_columns = tex_atlas.m_width / tile_width;
    const int tile_rows = tex_atlas.m_height / tile_height;
    const int64_t tile_quantity = static_cast<int64_t>(tile_columns) * tile_rows;
    GLint max_layers = 0;
    glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &max_layers);
    if (tile_quantity <= 0 || tile_quantity > max_layers || tile_quantity > UINT16_MAX)
        throw std::runtime_error("Texture atlas layer count exceeds the supported range: " + tex_atlas.m_filepath);


    // Create texture array to store slices
    TextureArray tile_set;
    tile_set.m_filepath = tex_atlas.m_filepath;
    tile_set.m_width = tile_width;
    tile_set.m_height = tile_height;
    tile_set.m_channel_amount = tex_atlas.m_channel_amount;
    tile_set.m_texture_format = tex_atlas.m_texture_format;
    tile_set.m_internal_format = tex_atlas.m_internal_format;
    tile_set.layer_amount = static_cast<uint16>(tile_quantity);
    glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &tile_set.m_texture_Id);
    if (!tile_set.m_texture_Id)
        throw std::runtime_error("OpenGL could not create texture array: " + tex_atlas.m_filepath);
    glTextureStorage3D(tile_set.m_texture_Id, 1, tex_atlas.m_internal_format,
                       tile_width, tile_height, static_cast<GLsizei>(tile_quantity));
    CheckTextureError("Allocating texture array " + tile_set.m_filepath);

    for (GLsizei i = 0; i < tile_quantity; ++i)
    {
        // For "glCopyImageSubData", (0, 0, 0) is at the bottom left
        GLint x_coord = (i % tile_columns) * tile_width, // Whenever (i % tile_columns) == 0, reset to the first column
              y_coord = (i / tile_columns) * tile_height;  // Whenever (i / tile_columns)++, switch to the next row(upward)
        glCopyImageSubData(tex_atlas.m_texture_Id, GL_TEXTURE_2D,
                           0, x_coord, y_coord, 0,  // Source texture atlas mipmap level, x coord, y coord, z coord(null)
                           tile_set.m_texture_Id, GL_TEXTURE_2D_ARRAY,
                           0, 0, 0, i,  // Destiny texture array mipmap level, x coord, y coord, z coord(layers)
                           tile_width, tile_height, 1);
    }
    tex_atlas.Destroy();

    //Misc
    glTextureParameteri(tile_set.m_texture_Id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(tile_set.m_texture_Id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(tile_set.m_texture_Id, GL_TEXTURE_MIN_FILTER, pixelated ? GL_NEAREST : GL_LINEAR);
    glTextureParameteri(tile_set.m_texture_Id, GL_TEXTURE_MAG_FILTER, pixelated ? GL_NEAREST : GL_LINEAR);
    CheckTextureError("Creating texture atlas array " + tile_set.m_filepath);

    return tile_set;
}
