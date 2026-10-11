#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "symocraft/scene/mesh.h"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <vector>

namespace SymoCraft::Experimental::D3D12R1 {

struct Extent { std::uint32_t width{}, height{}; };
struct MeshHandle { std::uint64_t owner{}; std::uint32_t slot{}, generation{}; };
struct Image { Extent extent; std::vector<std::uint8_t> rgba; };
struct FrameResult {
    bool presented{}, skipped{};
    std::uint64_t submittedFence{}, completedFence{};
    std::optional<Image> screenshot;
};
struct RendererStats {
    std::uint64_t submittedFrames{}, lastSubmittedFence{}, completedFence{};
    std::size_t retiredPending{}, retiredCollected{};
    std::uint64_t debugWarnings{}, debugErrors{};
};
struct DeviceInfo {
    std::string name, luid, driver, featureLevel, shaderModel;
    std::string textureFormat, renderTargetFormat;
    std::uint32_t vendorId{}, deviceId{};
    bool debugLayerEnabled{}, softwareAdapter{};
};
struct PresentationInfo {
    Extent extent;
    std::uint32_t format{}, bufferCount{}, sampleCount{}, sampleQuality{}, swapEffect{}, flags{};
    bool windowed{};
};

// This HWND-bearing API belongs only to the isolated R1 experiment.
class ProbeRenderer final {
public:
    ProbeRenderer(HWND window, Extent extent, const std::filesystem::path& shaderDir,
                  std::ostream& log, bool requireDebug = true);
    ~ProbeRenderer() noexcept;
    ProbeRenderer(const ProbeRenderer&) = delete;
    ProbeRenderer& operator=(const ProbeRenderer&) = delete;
    ProbeRenderer(ProbeRenderer&&) = delete;
    ProbeRenderer& operator=(ProbeRenderer&&) = delete;

    // One accepted texture array per probe; replacement is outside this fixture.
    void CreateTexture(std::span<const std::uint8_t> pixels, std::uint32_t width,
                       std::uint32_t height, std::uint32_t layers, bool srgb);
    MeshHandle UploadMesh(const SymoCraft::MeshData& mesh);
    void UpdateMesh(MeshHandle handle, const SymoCraft::MeshData& mesh);
    void DestroyMesh(MeshHandle handle);
    FrameResult Render(MeshHandle handle, bool capture = false, bool present = true);
    void Resize(Extent extent);
    void BeginGpuGate();
    void EndGpuGate();
    // Simulates an unmatched CPU event after completed work, not device loss.
    void SetInjectTimeout(bool enabled);
    RendererStats Stats();
    const DeviceInfo& Info() const;
    PresentationInfo Presentation() const;
    void Shutdown();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace SymoCraft::Experimental::D3D12R1
