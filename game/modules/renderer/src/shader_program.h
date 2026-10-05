#ifndef SHADER_PROGRAM_H
#define SHADER_PROGRAM_H

#include <symocraft/foundation/types.h>
#include <symocraft/foundation/math.h>
#include "shader.h"

struct ShaderProgram
{
	uint32 programId{};

    ShaderProgram() = default;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram& operator=(const ShaderProgram&) = delete;

	bool CompileAndLink(std::string_view vertexShaderFile, std::string_view fragmentShaderFile);
	void Bind() const;
	void Unbind() const;
	void Destroy();

	void UploadVec4(const char* varName, const glm::vec4& vec4) const;
	void UploadVec3(const char* varName, const glm::vec3& vec3) const;
	void UploadVec2(const char* varName, const glm::vec2& vec2) const;
	void UploadIVec4(const char* varName, const glm::ivec4& vec4) const;
	void UploadIVec3(const char* varName, const glm::ivec3& vec3) const;
	void UploadIVec2(const char* varName, const glm::ivec2& vec2) const;
	void UploadFloat(const char* varName, float value) const;
	void UploadInt(const char* varName, int value) const;
	void UploadIntArray(const char* varName, int length, const int* array) const;
	void UploadUInt(const char* varName, uint32 value) const;
	void UploadBool(const char* varName, bool value) const;
		 
	void UploadMat4(const char* varName, const glm::mat4& mat4) const;
	void UploadMat3(const char* varName, const glm::mat3& mat3) const;

	static void clearAllShaderVariables();
};

#endif
