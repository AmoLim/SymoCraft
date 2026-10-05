#ifndef SHADER_H
#define SHADER_H

#include <symocraft/foundation/types.h>
#include <glad/glad.h>
#include <string_view>


enum class ShaderType : uint8
{
	Vertex,
	Fragment,
};

struct Shader
{
	uint32 shaderId{};
	ShaderType m_type{};

    Shader() = default;
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

	bool Compile(ShaderType type, std::string_view shaderFilepath);
	void Destroy();

	static GLenum toGlShaderType(ShaderType type);
};

#endif
