#pragma once

#include "symocraft/renderer/types.h"

#include <memory>

namespace SymoCraft::Rendering::Detail {

struct BackendResourceId {
    std::uint64_t owner = 0;
    std::size_t slot = 0;
    std::uint64_t generation = 0;
    friend bool operator==(const BackendResourceId&, const BackendResourceId&) = default;
};

struct PreparedDraw {
    BackendResourceId mesh;
    BackendResourceId texture;
    std::shared_ptr<const MeshData> mesh_data;
    std::shared_ptr<const TextureArrayData> texture_data;
    glm::mat4 object_transform{1.0f};
};

struct PreparedFrame {
    std::uint64_t frame_id = 0;
    CameraParameters camera;
    std::vector<PreparedDraw> draws;
    std::optional<SelectionBox> selection;
    bool request_screenshot = false;
    Extent2D extent;
};

class RenderBackend {
public:
    // For every operation, recoverable errors preserve the previous accepted
    // native state; a partially applied native transition must report a fatal
    // error rather than allowing facade retries against stale cached state.
    // Concrete backend destructors also own bounded RAII cleanup, including
    // factory/constructor failures before RendererState can take ownership.
    virtual ~RenderBackend() = default;
    virtual RendererInfo GetInfo() const = 0;

    // Resource operations publish atomically on success. Backends retain old native
    // versions until their last submitted use completes, including after destroy.
    virtual void CreateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) = 0;
    virtual void UpdateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) = 0;
    virtual void DestroyMesh(BackendResourceId id) = 0;
    virtual void CreateTextureArray(BackendResourceId id,
                                    std::shared_ptr<const TextureArrayData> data) = 0;
    virtual void DestroyTextureArray(BackendResourceId id) = 0;

    // All frame input has been validated and owns its resource snapshots. Render
    // is the sole draw/present operation; no facade callbacks or borrowed spans.
    // Recoverable errors must leave no partially accepted frame or native submit;
    // after native work has been submitted, an unrecoverable error is reported as
    // DeviceLost, SubmissionFailure, or Timeout so the facade cannot retry it.
    // If a native extent changes to zero between frame boundaries, a ZeroExtent
    // skip must agree with the suspended actual extent returned by GetInfo.
    virtual FrameResult Render(const PreparedFrame& frame) = 0;
    // A zero extent suspends presentation and never creates a zero-size swapchain.
    virtual void Resize(Extent2D extent) = 0;
    virtual void SetVSync(bool enabled) = 0;
    // Only completed, previously accepted backend Render identities may appear.
    // The backend owns exact query/frame association (including gaps in IDs),
    // bounded pending-query storage, and discarded/unsupported sample reporting.
    // Poll never waits, fabricates an identity, or invents a zero duration.
    virtual std::vector<GpuFrameSample> PollGpuSamples() = 0;
    // Both shutdown paths must bound native waits using the configured timeout.
    // The fallback records diagnostics and never throws, even after device loss.
    virtual void Shutdown() = 0;
    virtual void ShutdownNoexcept() noexcept = 0;
};

} // namespace SymoCraft::Rendering::Detail
