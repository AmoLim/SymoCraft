#include "renderer/renderer.h"
#include "core/application.h"
#include "core/asset_paths.h"
#include "core/window.h"
#include "world/block.h"
#include "core/constants.h"
#include <stdexcept>

namespace SymoCraft{
    Batch<BlockVertex3D> chunk_batch;
    Batch<LineVertex3D> line_batch;

    namespace Renderer {

        static ShaderProgram block_shader;
        static ShaderProgram line3D_shader;
        static Camera *camera;

        static glm::mat4 g_projection_mat;
        static glm::mat4 g_view_mat;
        static glm::mat4 g_combo_mat;
        static float g_normal;

        constexpr float depth_value = 1.0f;
        constexpr std::array<float, 4> clear_color = {0.529f, 0.808f, 0.922f, 1.0f};

        // Internal functions
#ifndef NDEBUG
        static void GLAPIENTRY messageCallback(GLenum, GLenum type, GLuint id, GLenum severity,
                                               GLsizei, const GLchar *message, const void*)
        {
            std::fprintf(stderr, "OpenGL diagnostic [type=0x%x severity=0x%x id=%u]: %s\n",
                         type, severity, id, message ? message : "no message");
        }
#endif

        void Init() {
            if (!glfwGetCurrentContext() || !glad_glCreateBuffers)
                throw std::runtime_error("Renderer initialization requires a loaded OpenGL context");
            camera = Application::GetCamera();

#ifndef NDEBUG
            if (glDebugMessageCallback)
            {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(messageCallback, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION,
                                      0, nullptr, GL_FALSE);
            }
#endif

            // Enable render parameters
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);


            line_batch.SetPrimitiveType(GL_LINES);
            line_batch.SetBatchSize(100);
            // Initialize shaders
            block_shader.CompileAndLink(Assets::Resolve("shaders/vs_BlockShader.glsl").string(),
                                        Assets::Resolve("shaders/fs_BlockShader.glsl").string());

            line3D_shader.CompileAndLink(Assets::Resolve("shaders/vs_FrameShader.glsl").string(),
                                         Assets::Resolve("shaders/fs_FrameShader.glsl").string());

            // Initialize batches
            chunk_batch.Init({
                                     {0, 3,   GL_INT, offsetof(BlockVertex3D, pos_coord)},
                                     {1, 3, GL_FLOAT, offsetof(BlockVertex3D, tex_coord)},
                                     {2, 1, GL_FLOAT, offsetof(BlockVertex3D, normal   )}});

            line_batch.Init({
                                    {0, 3, GL_FLOAT, offsetof(LineVertex3D, pos_coord)}});


            try
            {
                LoadBlocks(Assets::Resolve("configs/blockFormats.yaml").string());
            }
            catch (const std::exception& error)
            {
                throw std::runtime_error(std::string("Failed to load block configuration: ") + error.what());
            }
        }

        void Free() {
            chunk_batch.Free();
            line_batch.Free();
            block_shader.Destroy();
            line3D_shader.Destroy();
            camera = nullptr;
#ifndef NDEBUG
            if (glfwGetCurrentContext() && glDebugMessageCallback)
                glDebugMessageCallback(nullptr, nullptr);
#endif
        }

        std::size_t AllocatedBufferBytes() { return chunk_batch.AllocatedBytes() + line_batch.AllocatedBytes(); }

        void Render(RenderStats* stats, GpuTimer* timer) {
            ClearBuffers();

            DrawBatches3D(stats, timer);
        }

        void ReloadShaders() {
            block_shader.Destroy();
            line3D_shader.Destroy();

            block_shader.CompileAndLink(Assets::Resolve("shaders/vs_BlockShader.glsl").string(),
                                        Assets::Resolve("shaders/fs_BlockShader.glsl").string());
            line3D_shader.CompileAndLink(Assets::Resolve("shaders/vs_FrameShader.glsl").string(),
                                         Assets::Resolve("shaders/fs_FrameShader.glsl").string());
        }

        void DrawBatches3D(RenderStats* stats, GpuTimer* timer) {
            g_projection_mat = camera->GetCameraProjMat();
            g_view_mat = camera->GetCameraViewMat();
            g_combo_mat = g_projection_mat * g_view_mat;

            block_shader.Bind();
            block_shader.UploadMat4("u_combo_mat", g_combo_mat);
            chunk_batch.Draw(stats, timer);
            block_shader.Unbind();

            line3D_shader.Bind();
            line3D_shader.UploadMat4("u_combo_mat", g_combo_mat);
            line_batch.ReloadData(stats);
            line_batch.Draw(stats, timer);
            line3D_shader.Unbind();
        }

        void FlushBatches3D(const glm::mat4 &projection_mat, const glm::mat4 &view_mat) {
            block_shader.Bind();

            g_combo_mat = projection_mat * view_mat;
            block_shader.UploadMat4("u_combo_mat", g_combo_mat);
            chunk_batch.Draw();

            block_shader.Unbind();
        }


        void ClearBuffers() {
            glClearNamedFramebufferfv(0, GL_COLOR, 0, clear_color.data());
            glClearNamedFramebufferfv(0, GL_DEPTH, 0, &depth_value);
        }


        // =========================================================
        // Draw 3D Functions
        // =========================================================

        static std::array<LineVertex3D, 24> frame_vertices{}; // Each block contains 6 faces, which contains 4 vertices

        // Generate render data for the ray cast block
        void GenerateBlockFrameData(const glm::vec3 &block_center_coord) {
            const glm::vec3 center = glm::floor(block_center_coord) + glm::vec3(0.5f);
            size_t index = 0;
            for (auto &vertex: frame_vertices) {
                // Expand around this block's center, not around the world origin.
                const glm::vec3 corner = BlockConstants::pos_coords[BlockConstants::frame_indices[index]];
                vertex.pos_coord = center + (corner - glm::vec3(0.5f)) * 1.004f;
                line_batch.AddVertex(vertex);
                index++;
            }
        }

        void ReportStatus() {
            if (face_count % 10000 == 0)
                AmoLogger_Log("%d vertices, %d faces in total loaded\n", vertex_count, face_count);
        }
    }
}
