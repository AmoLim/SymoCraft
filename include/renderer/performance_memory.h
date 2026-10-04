#pragma once
#include "core/performance.h"
namespace SymoCraft::Performance {
    Memory SampleMemory(double elapsed, bool nvx_supported);
    void CaptureFramebuffer(int width, int height, const std::filesystem::path& path);
}
