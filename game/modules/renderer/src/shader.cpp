#include "shader.h"
#include <symocraft/assets/image.h>
#include <algorithm>
#include <vector>

#include <stdexcept>

bool Shader::Compile(ShaderType type, std::string_view shaderFilepath)
{
    const std::string path(shaderFilepath);
    const auto bytes = SymoCraft::Assets::ReadBytes(path);
    if (bytes.empty())
        throw std::runtime_error("Shader is empty or unreadable: " + path);
    const std::string text(bytes.begin(), bytes.end());

    const GLenum shader_type = toGlShaderType(type);
    if (shader_type == GL_INVALID_ENUM)
        throw std::invalid_argument("Unknown shader type for " + path);

    Destroy();
    shaderId = glCreateShader(shader_type);
    if (!shaderId)
        throw std::runtime_error("OpenGL could not create shader: " + path);
    try
    {
        const char* data = text.c_str();
        glShaderSource(shaderId, 1, &data, nullptr);
        glCompileShader(shaderId);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE)
        {
            GLint length = 0;
            glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &length);
            std::vector<GLchar> log(static_cast<size_t>(std::max(length, 1)), '\0');
            glGetShaderInfoLog(shaderId, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error("Shader compilation failed: " + path + "\n" + log.data());
        }
        m_type = type;
    }
    catch (...)
    {
        Destroy();
        throw;
    }
    return true;
}

void Shader::Destroy()
{
	if (shaderId)
	{
		glDeleteShader(shaderId);
		shaderId = 0;
	}
}


GLenum Shader::toGlShaderType(ShaderType type)
{
	switch (type)
	{
	case ShaderType::Vertex:
		return GL_VERTEX_SHADER;
	case ShaderType::Fragment:
		return GL_FRAGMENT_SHADER;
	}
	return GL_INVALID_ENUM;
}
