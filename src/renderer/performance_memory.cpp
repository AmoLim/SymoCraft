#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <glad/glad.h>
#include "renderer/performance_memory.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>
#include <stdexcept>

namespace SymoCraft::Performance {
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

    Memory SampleMemory(double elapsed, bool nvx_supported)
    {
        Memory result;
        result.elapsed = elapsed;
        PROCESS_MEMORY_COUNTERS_EX process{};
        process.cb = sizeof(process);
        if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&process), sizeof(process))) {
            result.working_set_bytes = process.WorkingSetSize;
            result.private_bytes = process.PrivateUsage;
        }
        if (nvx_supported) {
            constexpr GLenum dedicated_memory_nvx = 0x9047, current_available_memory_nvx = 0x9049;
            GLint dedicated = 0, available = 0;
            glGetIntegerv(dedicated_memory_nvx, &dedicated);
            glGetIntegerv(current_available_memory_nvx, &available);
            if (dedicated >= 0) result.device_dedicated_kib = static_cast<std::uint64_t>(dedicated);
            if (available >= 0) result.device_available_kib = static_cast<std::uint64_t>(available);
        }
        return result;
    }
}
