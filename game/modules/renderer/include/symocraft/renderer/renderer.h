#pragma once
#include <symocraft/scene/mesh.h>
#include <symocraft/scene/camera.h>
#include <symocraft/renderer/render_stats.h>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace SymoCraft {
    class Window;
    class GpuTimer;
    namespace Renderer {
        struct DeviceInfo {
            std::string vendor, renderer, version;
            int msaa_samples{};
            bool nvx_memory_supported{};
        };
        struct DeviceMemory {
            std::optional<std::uint64_t> dedicated_kib, available_kib;
        };
        void AttachContext(Window& window);
        void Init();
        void Free();
        const DeviceInfo& Device();
        std::size_t AllocatedBufferBytes();
        std::uint16_t LoadTextureAtlas(const std::filesystem::path& path);
        void AppendMesh(std::span<const BlockVertex3D> vertices);
        void SetSelection(std::optional<glm::vec3> position);
        void SetViewport(int width, int height);
        void Render(const CameraView& camera, RenderStats* stats = nullptr, GpuTimer* timer = nullptr);
        void Present(Window& window);
        void SetVsync(bool enabled);
        void ReloadShaders();
        DeviceMemory SampleDeviceMemory();
        void CaptureFramebuffer(int width, int height, const std::filesystem::path& path);
    }
}
