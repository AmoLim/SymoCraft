#include <symocraft/renderer/renderer.h>
#include <symocraft/assets/asset_paths.h>
#include "batch.hpp"
#include "shader_program.h"
#include "texture.h"
#include "graphics_bridge.h"
#include <array>
#include <cstdio>
#include <iostream>
#include <stdexcept>

namespace SymoCraft::Renderer {
    namespace {
        Batch<BlockVertex3D> chunk_batch;
        Batch<LineVertex3D> line_batch;
        ShaderProgram block_shader, line_shader;
        TextureArray texture_array;
        DeviceInfo device;
        int viewport_width{}, viewport_height{};
        constexpr float depth_value = 1.0f;
        constexpr std::array<float, 4> clear_color{0.529f, 0.808f, 0.922f, 1.0f};
        constexpr std::array<glm::vec3, 8> corners{{{0,0,0}, {0,0,1}, {1,0,1}, {1,0,0},
                                                  {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0}}};
        constexpr std::array<unsigned, 24> edges{7,6,6,2,2,3,3,7,7,4,6,5,2,1,3,0,5,4,4,0,0,1,1,5};
#ifndef NDEBUG
        void GLAPIENTRY MessageCallback(GLenum, GLenum type, GLuint id, GLenum severity,
                                         GLsizei, const GLchar* message, const void*) {
            std::fprintf(stderr, "OpenGL diagnostic [type=0x%x severity=0x%x id=%u]: %s\n",
                         type, severity, id, message ? message : "no message");
        }
#endif
    }
    void AttachContext(Window& window) {
        Platform::GraphicsBridge::MakeCurrent(window);
        if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(Platform::GraphicsBridge::GetProcedure)))
            throw std::runtime_error("Failed to load OpenGL functions through GLAD");
        if (!GLAD_GL_VERSION_4_6)
            throw std::runtime_error("OpenGL 4.6 is required by the shaders and renderer");
        const auto* vendor = glGetString(GL_VENDOR);
        const auto* renderer = glGetString(GL_RENDERER);
        const auto* version = glGetString(GL_VERSION);
        if (!vendor || !renderer || !version) throw std::runtime_error("Could not query the current OpenGL device");
        device.vendor = reinterpret_cast<const char*>(vendor);
        device.renderer = reinterpret_cast<const char*>(renderer);
        device.version = reinterpret_cast<const char*>(version);
        glGetIntegerv(GL_SAMPLES, &device.msaa_samples);
        device.nvx_memory_supported = Platform::GraphicsBridge::ExtensionSupported("GL_NVX_gpu_memory_info");
        std::cout << "OpenGL vendor: " << device.vendor << '\n'
                  << "OpenGL renderer: " << device.renderer << '\n'
                  << "OpenGL version: " << device.version << std::endl;
        SetViewport(window.width, window.height);
    }
    void Init() {
        if (!Platform::GraphicsBridge::HasCurrentContext() || !glad_glCreateBuffers)
            throw std::runtime_error("Renderer initialization requires a loaded OpenGL context");
#ifndef NDEBUG
        if (glDebugMessageCallback) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(MessageCallback, nullptr);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
        }
#endif
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        line_batch.SetPrimitiveType(GL_LINES);
        line_batch.SetBatchSize(100);
        ReloadShaders();
        chunk_batch.Init({{0,3,GL_INT,offsetof(BlockVertex3D,pos_coord)},
                          {1,3,GL_FLOAT,offsetof(BlockVertex3D,tex_coord)},
                          {2,1,GL_FLOAT,offsetof(BlockVertex3D,normal)}});
        line_batch.Init({{0,3,GL_FLOAT,offsetof(LineVertex3D,pos_coord)}});
    }
    void Free() {
        texture_array.Destroy();
        chunk_batch.Free(); line_batch.Free();
        block_shader.Destroy(); line_shader.Destroy();
        viewport_width = viewport_height = 0;
#ifndef NDEBUG
        if (Platform::GraphicsBridge::HasCurrentContext() && glDebugMessageCallback)
            glDebugMessageCallback(nullptr, nullptr);
#endif
    }
    const DeviceInfo& Device() { return device; }
    std::size_t AllocatedBufferBytes() { return chunk_batch.AllocatedBytes() + line_batch.AllocatedBytes(); }
    std::uint16_t LoadTextureAtlas(const std::filesystem::path& path) {
        texture_array = texture_array.CreateAtlasSlice(path.string(), true);
        return texture_array.layer_amount;
    }
    void AppendMesh(std::span<const BlockVertex3D> vertices) { chunk_batch.AddVertex(vertices.data(), vertices.size()); }
    void SetSelection(std::optional<glm::vec3> position) {
        if (!position) return;
        const auto center = glm::floor(*position) + glm::vec3(0.5f);
        for (auto index : edges)
            line_batch.AddVertex(LineVertex3D{center + (corners[index] - glm::vec3(0.5f)) * 1.004f});
    }
    void SetViewport(int width, int height) {
        if (width != viewport_width || height != viewport_height) {
            glViewport(0, 0, width, height); viewport_width = width; viewport_height = height;
        }
    }
    void Render(const CameraView& camera, RenderStats* stats, GpuTimer* timer) {
        glBindTextureUnit(0, texture_array.m_texture_Id);
        glClearNamedFramebufferfv(0, GL_COLOR, 0, clear_color.data());
        glClearNamedFramebufferfv(0, GL_DEPTH, 0, &depth_value);
        const auto combined = camera.projection * camera.view;
        block_shader.Bind(); block_shader.UploadMat4("u_combo_mat", combined);
        chunk_batch.Draw(stats, timer); block_shader.Unbind();
        line_shader.Bind(); line_shader.UploadMat4("u_combo_mat", combined);
        line_batch.ReloadData(stats); line_batch.Draw(stats, timer); line_shader.Unbind();
    }
    void Present(Window& window) { Platform::GraphicsBridge::Present(window); }
    void SetVsync(bool enabled) { Platform::GraphicsBridge::SetVsync(enabled); }
    void ReloadShaders() {
        block_shader.Destroy(); line_shader.Destroy();
        block_shader.CompileAndLink(Assets::Resolve("shaders/vs_BlockShader.glsl").string(),
                                    Assets::Resolve("shaders/fs_BlockShader.glsl").string());
        line_shader.CompileAndLink(Assets::Resolve("shaders/vs_FrameShader.glsl").string(),
                                   Assets::Resolve("shaders/fs_FrameShader.glsl").string());
    }
}
