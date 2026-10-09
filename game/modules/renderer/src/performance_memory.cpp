#include <glad/glad.h>
#include <symocraft/renderer/renderer.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <stdexcept>
#include <vector>

namespace SymoCraft::Renderer {
    void CaptureFramebuffer(int width, int height, const std::filesystem::path& path)
    {
        std::vector<unsigned char> pixels(static_cast<std::size_t>(width) * height * 3);
        glReadBuffer(GL_BACK);
        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        stbi_flip_vertically_on_write(1);
        if (!stbi_write_png(path.string().c_str(), width, height, 3, pixels.data(), width * 3))
            throw std::runtime_error("Cannot write benchmark screenshot");
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
