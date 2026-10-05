#include "shader_program.h"
#include <robin_hood.h>
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

// Internal Structures
struct ShaderVariable
{
	std::string name;
	GLint var_location{};
	uint32 shaderProgramId{};

	bool operator==(const ShaderVariable& other) const
	{
		return other.shaderProgramId == shaderProgramId && other.name == name;
	}
};

struct HashShaderVar
{
	std::size_t operator()(const ShaderVariable& key) const
	{
		// This code was adapted from https://stackoverflow.com/questions/35985960/c-why-is-boosthash-combine-the-best-way-to-combine-hash-values
		std::size_t seed = std::hash<std::string>()(key.name);
		seed ^= std::hash<uint32>()(key.shaderProgramId) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		return seed;
	}
};

// Internal Variables
static auto allShaderVariableLocations = robin_hood::unordered_set<ShaderVariable, HashShaderVar>();

// Forward Declarations
static GLint getVariableLocation(const ShaderProgram& shader, const char* varName);

bool ShaderProgram::CompileAndLink(std::string_view vertexShaderFile, std::string_view fragmentShaderFile)
{
    Shader vertex_shader;
    Shader fragment_shader;
    GLuint program = 0;
    try
    {
        vertex_shader.Compile(ShaderType::Vertex, vertexShaderFile);
        fragment_shader.Compile(ShaderType::Fragment, fragmentShaderFile);
        program = glCreateProgram();
        if (!program)
            throw std::runtime_error("OpenGL could not create a shader program");
        glAttachShader(program, vertex_shader.shaderId);
        glAttachShader(program, fragment_shader.shaderId);
        glLinkProgram(program);

        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE)
        {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::vector<GLchar> log(static_cast<size_t>(std::max(length, 1)), '\0');
            glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error("Shader linking failed: " + std::string(vertexShaderFile) +
                                     " + " + std::string(fragmentShaderFile) + "\n" + log.data());
        }
        glDetachShader(program, vertex_shader.shaderId);
        glDetachShader(program, fragment_shader.shaderId);
        vertex_shader.Destroy();
        fragment_shader.Destroy();
        Destroy();
        programId = program;
    }
    catch (...)
    {
        vertex_shader.Destroy();
        fragment_shader.Destroy();
        if (program)
            glDeleteProgram(program);
        throw;
    }
    std::cout << "Shader compilation and linking succeeded: " << vertexShaderFile
              << " + " << fragmentShaderFile << '\n';
    return true;
}

void ShaderProgram::Destroy()
{
	if (programId)
	{
		glDeleteProgram(programId);
		programId = 0;

        // GL may reuse names after deletion; rebuild cached locations lazily.
        clearAllShaderVariables();
	}

}

void ShaderProgram::Bind() const
{
    if (!programId)
        throw std::logic_error("Cannot bind an uninitialized shader program");
	glUseProgram(programId);
}

void ShaderProgram::Unbind() const
{
	glUseProgram(0);
}

void ShaderProgram::UploadVec4(const char* varName, const glm::vec4& vec4) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform4f(var_location, vec4.x, vec4.y, vec4.z, vec4.w);
}

void ShaderProgram::UploadVec3(const char* varName, const glm::vec3& vec3) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform3f(var_location, vec3.x, vec3.y, vec3.z);
}

void ShaderProgram::UploadVec2(const char* varName, const glm::vec2& vec2) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform2f(var_location, vec2.x, vec2.y);
}

void ShaderProgram::UploadIVec4(const char* varName, const glm::ivec4& vec4) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform4i(var_location, vec4.x, vec4.y, vec4.z, vec4.w);
}

void ShaderProgram::UploadIVec3(const char* varName, const glm::ivec3& vec3) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform3i(var_location, vec3.x, vec3.y, vec3.z);
}

void ShaderProgram::UploadIVec2(const char* varName, const glm::ivec2& vec2) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform2i(var_location, vec2.x, vec2.y);
}

void ShaderProgram::UploadFloat(const char* varName, float value) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform1f(var_location, value);
}

void ShaderProgram::UploadInt(const char* varName, int value) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform1i(var_location, value);
}

void ShaderProgram::UploadUInt(const char* varName, uint32 value) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform1ui(var_location, value);
}

void ShaderProgram::UploadMat4(const char* varName, const glm::mat4& mat4) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniformMatrix4fv(var_location, 1, GL_FALSE, glm::value_ptr(mat4));
}

void ShaderProgram::UploadMat3(const char* varName, const glm::mat3& mat3) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniformMatrix3fv(var_location, 1, GL_FALSE, glm::value_ptr(mat3));
}

void ShaderProgram::UploadIntArray(const char* varName, int length, const int* array) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform1iv(var_location, length, array);
}

void ShaderProgram::UploadBool(const char* varName, bool value) const
{
	int var_location = getVariableLocation(*this, varName);
	glUniform1i(var_location, value ? 1 : 0);
}

void ShaderProgram::clearAllShaderVariables()
{
	allShaderVariableLocations.clear();
}

// Private functions
static GLint getVariableLocation(const ShaderProgram& shader, const char* varName)
{
	ShaderVariable match = {
		varName,
		0,
		shader.programId
	};

	auto iter = allShaderVariableLocations.find(match);
	if (iter != allShaderVariableLocations.end())
	{
		return iter->var_location;
	}

    match.var_location = glGetUniformLocation(shader.programId, varName);
    allShaderVariableLocations.emplace(match);
    return match.var_location;
}
