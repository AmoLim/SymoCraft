#include "shader_program.h"

#include <set>
#include <iostream>
#include <stdexcept>

namespace
{
    struct Calls
    {
        GLuint next_id{1};
        int shader_attempts{};
        int program_attempts{};
        int links{};
        int fail_shader_attempt{};
        int fail_compile_attempt{};
        bool fail_program_creation{};
        bool fail_link{};
        std::set<GLuint> shaders;
        std::set<GLuint> programs;
    } calls;

    void Require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }

    template<class Function>
    void ExpectFailure(Function&& function, std::string_view diagnostic)
    {
        try
        {
            function();
        }
        catch (const std::runtime_error& error)
        {
            Require(std::string_view(error.what()).find(diagnostic) != std::string_view::npos,
                    "Failure did not identify the expected shader or operation.");
            return;
        }
        throw std::runtime_error("Shader loading unexpectedly succeeded.");
    }

    GLuint APIENTRY CreateShader(GLenum)
    {
        if (++calls.shader_attempts == calls.fail_shader_attempt)
            return 0;
        const GLuint id = calls.next_id++;
        calls.shaders.insert(id);
        return id;
    }
    void APIENTRY DeleteShader(GLuint id)
    {
        Require(calls.shaders.erase(id) == 1, "Invalid or duplicate shader deletion.");
    }
    void APIENTRY ShaderSource(GLuint, GLsizei count, const GLchar* const* text, const GLint*)
    {
        Require(count == 1 && text && text[0] && text[0][0], "Empty shader source reached OpenGL.");
    }
    void APIENTRY CompileShader(GLuint) {}
    void APIENTRY GetShader(GLuint, GLenum name, GLint* result)
    {
        *result = name == GL_COMPILE_STATUS && calls.shader_attempts != calls.fail_compile_attempt
            ? GL_TRUE : 0;
    }
    // An empty driver log must still produce a safe and useful diagnostic.
    void APIENTRY GetLog(GLuint, GLsizei size, GLsizei*, GLchar* log)
    {
        Require(size > 0 && log, "Shader error log did not have valid storage.");
        log[0] = '\0';
    }
    GLuint APIENTRY CreateProgram()
    {
        ++calls.program_attempts;
        if (calls.fail_program_creation)
            return 0;
        const GLuint id = calls.next_id++;
        calls.programs.insert(id);
        return id;
    }
    void APIENTRY DeleteProgram(GLuint id)
    {
        Require(calls.programs.erase(id) == 1, "Invalid or duplicate program deletion.");
    }
    void APIENTRY AttachShader(GLuint, GLuint) {}
    void APIENTRY DetachShader(GLuint, GLuint) {}
    void APIENTRY LinkProgram(GLuint) { ++calls.links; }
    void APIENTRY GetProgram(GLuint, GLenum name, GLint* result)
    {
        *result = name == GL_LINK_STATUS && !calls.fail_link ? GL_TRUE : 0;
    }

    void InstallGLStubs()
    {
        glad_glCreateShader = CreateShader;
        glad_glDeleteShader = DeleteShader;
        glad_glShaderSource = ShaderSource;
        glad_glCompileShader = CompileShader;
        glad_glGetShaderiv = GetShader;
        glad_glGetShaderInfoLog = GetLog;
        glad_glCreateProgram = CreateProgram;
        glad_glDeleteProgram = DeleteProgram;
        glad_glAttachShader = AttachShader;
        glad_glDetachShader = DetachShader;
        glad_glLinkProgram = LinkProgram;
        glad_glGetProgramiv = GetProgram;
        glad_glGetProgramInfoLog = GetLog;
    }

    void CheckFiles(const std::string& vertex, const std::string& fragment, const std::string& empty)
    {
        // A regular file cannot also be a directory, so this path cannot exist.
        const std::string missing = vertex + "/missing.glsl";
        Shader shader;
        ShaderProgram program;
        shader.Destroy();
        program.Destroy();

        ExpectFailure([&] { shader.Compile(ShaderType::Vertex, missing); }, missing);
        ExpectFailure([&] { shader.Compile(ShaderType::Vertex, empty); }, empty);
        ExpectFailure([&] { program.CompileAndLink(missing, fragment); }, missing);
        Require(calls.shader_attempts == 0 && calls.program_attempts == 0 && calls.links == 0,
                "Missing or empty vertex source reached OpenGL.");

        ExpectFailure([&] { program.CompileAndLink(vertex, missing); }, missing);
        ExpectFailure([&] { program.CompileAndLink(vertex, empty); }, empty);
        Require(calls.shaders.empty() && calls.program_attempts == 0 && calls.links == 0,
                "Fragment loading failure leaked its vertex shader or reached linking.");

        const std::string extended = vertex + ".outside-the-view";
        Require(shader.Compile(ShaderType::Vertex, std::string_view(extended.data(), vertex.size())),
                "Shader path string_view was read past its length.");
        shader.Destroy();
        shader.Destroy();
        Require(calls.shaders.empty(), "Shader cleanup leaked an object.");
    }

    void CheckDriverFailures(const std::string& vertex, const std::string& fragment)
    {
        for (int stage = 1; stage <= 2; ++stage)
        {
            for (bool fail_creation : {false, true})
            {
                calls = {};
                calls.fail_shader_attempt = fail_creation ? stage : 0;
                calls.fail_compile_attempt = fail_creation ? 0 : stage;
                ShaderProgram program;
                ExpectFailure([&] { program.CompileAndLink(vertex, fragment); }, stage == 1 ? vertex : fragment);
                program.Destroy();
                Require(calls.shaders.empty() && calls.programs.empty() && calls.program_attempts == 0,
                        "Shader failure leaked an object or allocated a program.");
            }
        }

        for (bool fail_creation : {false, true})
        {
            calls = {};
            calls.fail_program_creation = fail_creation;
            calls.fail_link = !fail_creation;
            ShaderProgram program;
            ExpectFailure([&] { program.CompileAndLink(vertex, fragment); },
                          fail_creation ? "could not create" : "linking failed");
            program.Destroy();
            Require(calls.shaders.empty() && calls.programs.empty(), "Program failure leaked GL objects.");
        }
    }

    void CheckReplacement(const std::string& vertex, const std::string& fragment)
    {
        calls = {};
        ShaderProgram program;
        Require(program.CompileAndLink(vertex, fragment), "Valid shader files could not be loaded.");
        const GLuint original = program.programId;
        Require(calls.links == 1 && calls.shaders.empty() && calls.programs.size() == 1,
                "Successful linking did not clean up the intermediate shaders.");

        calls.fail_link = true;
        ExpectFailure([&] { program.CompileAndLink(vertex, fragment); }, "linking failed");
        Require(program.programId == original && calls.programs.contains(original) &&
                calls.programs.size() == 1 && calls.shaders.empty(),
                "Failed replacement destroyed the old program or leaked the new one.");

        calls.fail_link = false;
        Require(program.CompileAndLink(vertex, fragment), "Successful program replacement failed.");
        Require(program.programId != original && !calls.programs.contains(original) &&
                calls.programs.size() == 1 && calls.shaders.empty(),
                "Successful replacement did not release the old program.");
        program.Destroy();
        program.Destroy();
        Require(calls.programs.empty(), "Program cleanup leaked an object.");
    }
}

int main(int argc, char* argv[])
{
    try
    {
        Require(argc == 4, "Expected vertex, fragment, and empty shader paths.");
        InstallGLStubs();
        CheckFiles(argv[1], argv[2], argv[3]);
        CheckDriverFailures(argv[1], argv[2]);
        CheckReplacement(argv[1], argv[2]);
        std::cout << "Shader loading and failure cleanup checks passed without an OpenGL context.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
