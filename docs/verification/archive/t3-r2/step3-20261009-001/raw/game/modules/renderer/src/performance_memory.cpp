#include <glad/glad.h>
#include <symocraft/renderer/legacy_renderer.h>
#include <symocraft/assets/image.h>
#include <vector>

namespace SymoCraft::Renderer {
    void CaptureFramebuffer(int width, int height, const std::filesystem::path& path)
    {
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        Assets::WritePng(path, width, height, 3, pixels, true);
    }

    DeviceMemory SampleDeviceMemory()
    {
        DeviceMemory result;
        if (Device().nvx_memory_supported) {
            constexpr GLenum dedicated_memory_nvx = 0x9047, current_available_memory_nvx = 0x9049;
            GLint dedicated = 0, available = 0;
            glGetIntegerv(dedicated_memory_nvx, &dedicated);
            glGetIntegerv(current_available_memory_nvx, &available);
            if (dedicated >= 0) result.dedicated_kib = static_cast<std::uint64_t>(dedicated);
            if (available >= 0) result.available_kib = static_cast<std::uint64_t>(available);
        }
        return result;
    }
}
