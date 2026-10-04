#pragma once
#include "core.h"
#include "shader_program.h"
#include "camera/camera.h"
#include "renderer/batch.hpp"

namespace SymoCraft{
    struct Framebuffer;

    extern Batch<BlockVertex3D> chunk_batch;
    static uint16 vertex_count;
    static uint16 face_count;

    namespace Renderer
    {
        void Init();
        void Free();
        void Render(RenderStats* stats = nullptr, GpuTimer* timer = nullptr);
        std::size_t AllocatedBufferBytes();

        void ReloadShaders();

        void DrawBatches3D(RenderStats* stats = nullptr, GpuTimer* timer = nullptr);
        void FlushBatches3D(const glm::mat4 &projection_mat, const glm::mat4 &view_mat);

        void ClearBuffers();

        void GenerateBlockFrameData(const glm::vec3 &block_center_coord);

        void ReportStatus();

    }

}
