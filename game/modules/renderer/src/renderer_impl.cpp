#include "renderer_impl.h"

#include "validation.h"
#include "frame_diagnostics.h"

#include <chrono>
#include <limits>

namespace SymoCraft::Rendering::Detail {

RendererState::RendererState(RendererConfig config, std::unique_ptr<RenderBackend> backend)
    : config_(std::move(config)), backend_(std::move(backend)),
      owner_thread_(std::this_thread::get_id()) {
    try {
        ValidateConfig(config_);
        if (!backend_) Throw(RenderErrorCategory::InitializationFailure, "Initialize", "backend is null");
        info_ = backend_->GetInfo();
        if (info_.backend != config_.backend)
            Throw(RenderErrorCategory::InitializationFailure, "Initialize", "backend identity does not match request");
        lifecycle_ = info_.framebuffer_extent.IsSuspended()
                         ? RendererLifecycle::Suspended : RendererLifecycle::Ready;
    } catch (const RenderError&) {
        if (backend_) backend_->ShutdownNoexcept();
        throw;
    } catch (const std::bad_alloc&) {
        if (backend_) backend_->ShutdownNoexcept();
        Throw(RenderErrorCategory::AllocationFailure, "Initialize", "CPU allocation failed");
    } catch (const std::invalid_argument& error) {
        if (backend_) backend_->ShutdownNoexcept();
        throw RenderError(RenderErrorCategory::InvalidInput, BackendName(config_.backend), "Initialize", error.what());
    } catch (const std::exception& error) {
        if (backend_) backend_->ShutdownNoexcept();
        throw RenderError(RenderErrorCategory::InitializationFailure, BackendName(config_.backend), "Initialize", error.what());
    } catch (...) {
        if (backend_) backend_->ShutdownNoexcept();
        Throw(RenderErrorCategory::InitializationFailure, "Initialize", "unknown backend initialization failure");
    }
}

RendererState::~RendererState() noexcept {
    if (backend_ && lifecycle_ != RendererLifecycle::Stopped) backend_->ShutdownNoexcept();
    resources_.Clear();
}

RendererState::CallGuard::CallGuard(RendererState& state, const char* operation, bool allow_inactive)
    : state_(state) {
    if (std::this_thread::get_id() != state_.owner_thread_)
        state_.Throw(RenderErrorCategory::WrongThread, operation, "renderer called from a nonowner thread");
    if (state_.in_call_)
        state_.Throw(RenderErrorCategory::ReentrantCall, operation, "renderer calls cannot be reentrant");
    if (!allow_inactive && (state_.lifecycle_ == RendererLifecycle::Failed ||
                           state_.lifecycle_ == RendererLifecycle::Stopped))
        state_.Throw(RenderErrorCategory::InvalidState, operation, "renderer is failed or stopped");
    state_.in_call_ = true;
}

void RendererState::Throw(RenderErrorCategory category, const char* operation, const char* message) const {
    throw RenderError(category, BackendName(config_.backend), operation, message);
}

void RendererState::RecordFatal(RenderErrorCategory category) noexcept {
    if (category == RenderErrorCategory::DeviceLost || category == RenderErrorCategory::SubmissionFailure ||
        category == RenderErrorCategory::Timeout || category == RenderErrorCategory::InitializationFailure)
        lifecycle_ = RendererLifecycle::Failed;
}

RendererInfo RendererState::ReadAcceptedInfo() {
    try {
        return backend_->GetInfo();
    } catch (...) {
        // The preceding native operation already succeeded. An unavailable
        // snapshot cannot leave stale cached state eligible for another submit.
        RecordFatal(RenderErrorCategory::SubmissionFailure);
        throw;
    }
}

MeshHandle RendererState::CreateMesh(const MeshData& data) {
    return Invoke("CreateMesh", [&] {
        DiagnosticWorkScope diagnostic(FrameWork::MeshCreateTotal);
        auto prepared = resources_.PrepareMesh(data);
        backend_->CreateMesh(prepared.id, prepared.data);
        resources_.Commit(prepared);
        return prepared.handle;
    });
}

void RendererState::UpdateMesh(MeshHandle handle, const MeshData& data) {
    Invoke("UpdateMesh", [&] {
        const auto id = resources_.Id(handle);
        DiagnosticWorkScope diagnostic(FrameWork::MeshUpdateTotal);
        std::optional<std::uint32_t> maximum_layer;
        { DiagnosticWorkScope validation(FrameWork::MeshValidate); maximum_layer = ValidateMesh(data); }
        std::shared_ptr<const MeshData> owned;
        { DiagnosticWorkScope copy(FrameWork::MeshOwnedCopy); owned = std::make_shared<const MeshData>(data); }
        backend_->UpdateMesh(id, owned);
        resources_.ReplaceMesh(handle, std::move(owned), maximum_layer);
    });
}

void RendererState::DestroyMesh(MeshHandle handle) {
    Invoke("DestroyMesh", [&] {
        const auto id = resources_.Id(handle);
        backend_->DestroyMesh(id);
        resources_.DestroyMesh(handle);
    });
}

TextureArrayHandle RendererState::CreateTextureArray(const TextureArrayData& data) {
    return Invoke("CreateTextureArray", [&] {
        auto prepared = resources_.PrepareTexture(data);
        backend_->CreateTextureArray(prepared.id, prepared.data);
        resources_.Commit(prepared);
        return prepared.handle;
    });
}

void RendererState::DestroyTextureArray(TextureArrayHandle handle) {
    Invoke("DestroyTextureArray", [&] {
        const auto id = resources_.Id(handle);
        backend_->DestroyTextureArray(id);
        resources_.DestroyTexture(handle);
    });
}

FrameResult RendererState::Render(const FrameInput& frame) {
    return Invoke("Render", [&] {
        FrameDiagnosticScope diagnostic(config_.backend == Backend::Vulkan || config_.backend == Backend::D3D12,
                                        frame.frame_id, frame.request_screenshot);
        const auto begin = std::chrono::steady_clock::now();
        if (has_frame_ && frame.frame_id <= last_frame_id_)
            throw std::invalid_argument("frame identity must increase strictly after each accepted frame");
        ValidateCamera(frame.camera);
        if (frame.selection) ValidateSelection(*frame.selection);

        PreparedFrame prepared;
        prepared.frame_id = frame.frame_id;
        prepared.camera = frame.camera;
        prepared.selection = frame.selection;
        prepared.request_screenshot = frame.request_screenshot;
        prepared.extent = info_.framebuffer_extent;
        prepared.draws.reserve(frame.draws.size());
        for (const auto& draw : frame.draws) {
            ValidateTransform(draw.object_transform);
            auto mesh = resources_.Mesh(draw.mesh);
            auto texture = resources_.Texture(draw.texture_array);
            {
                DiagnosticWorkScope work(FrameWork::LayerSummaryCheck);
                ValidateMeshTextureLayers(resources_.MeshMaximumLayer(draw.mesh), texture->layers);
            }
            prepared.draws.push_back({resources_.Id(draw.mesh), resources_.Id(draw.texture_array),
                                      std::move(mesh), std::move(texture), draw.object_transform});
        }
        if (frame.selection && !info_.capabilities.selection_boxes)
            Throw(RenderErrorCategory::Unsupported, "Render", "backend does not support selection boxes");
        if (!prepared.draws.empty() && !info_.capabilities.texture_arrays)
            Throw(RenderErrorCategory::Unsupported, "Render", "backend does not support texture arrays");
        if (frame.request_screenshot && !info_.capabilities.screenshots)
            Throw(RenderErrorCategory::Unsupported, "Render", "backend does not support screenshots");

        const auto prepared_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - begin);
        DiagnosticPhase(FramePhase::BackendSetup);
        FrameResult result;
        if (lifecycle_ == RendererLifecycle::Suspended) {
            result.frame_id = frame.frame_id;
            result.status = FrameStatus::Skipped;
            result.skip_reason = FrameSkipReason::ZeroExtent;
        } else {
            result = backend_->Render(prepared);
            DiagnosticPhase(FramePhase::FacadeResult);
            if (result.frame_id != frame.frame_id ||
                (result.status != FrameStatus::Presented && result.status != FrameStatus::Skipped) ||
                (result.status == FrameStatus::Presented && result.skip_reason != FrameSkipReason::None) ||
                (result.status == FrameStatus::Skipped && result.skip_reason != FrameSkipReason::Occluded &&
                 result.skip_reason != FrameSkipReason::ZeroExtent))
                Throw(RenderErrorCategory::SubmissionFailure, "Render", "backend returned inconsistent frame result");
            if (result.statistics.cpu_prepare.count() < 0 || result.statistics.cpu_submit.count() < 0 ||
                result.statistics.present_wait.count() < 0)
                Throw(RenderErrorCategory::SubmissionFailure, "Render", "backend returned negative frame duration");
            if (result.screenshot) {
                const auto& capture = *result.screenshot;
                const auto width = static_cast<std::size_t>(capture.extent.width);
                const auto height = static_cast<std::size_t>(capture.extent.height);
                if (!frame.request_screenshot || result.status != FrameStatus::Presented ||
                    capture.extent.width != prepared.extent.width || capture.extent.height != prepared.extent.height ||
                    (width != 0 && height > std::numeric_limits<std::size_t>::max() / width) ||
                    width * height > std::numeric_limits<std::size_t>::max() / 4 ||
                    capture.pixels.size() != width * height * 4)
                    Throw(RenderErrorCategory::SubmissionFailure, "Render", "backend returned inconsistent screenshot");
            } else if (frame.request_screenshot && result.status == FrameStatus::Presented) {
                Throw(RenderErrorCategory::SubmissionFailure, "Render", "backend omitted requested screenshot");
            }
            if (result.skip_reason == FrameSkipReason::ZeroExtent) {
                auto updated = ReadAcceptedInfo();
                if (updated.backend != config_.backend || !updated.framebuffer_extent.IsSuspended())
                    Throw(RenderErrorCategory::SubmissionFailure, "Render", "backend zero-extent skip disagrees with info");
                info_ = std::move(updated);
                lifecycle_ = RendererLifecycle::Suspended;
            }
            last_backend_frame_id_ = frame.frame_id;
            has_backend_frame_ = true;
        }
        const auto maximum_duration = std::chrono::nanoseconds::max().count();
        if (prepared_time.count() < 0 ||
            result.statistics.cpu_prepare.count() > maximum_duration - prepared_time.count()) {
            RecordFatal(RenderErrorCategory::SubmissionFailure);
            Throw(RenderErrorCategory::SubmissionFailure, "Render", "combined CPU preparation duration overflow");
        }
        result.statistics.cpu_prepare += prepared_time;
        last_frame_id_ = frame.frame_id;
        has_frame_ = true;
        diagnostic.Complete();
        return result;
    });
}

void RendererState::Resize(Extent2D extent) {
    Invoke("Resize", [&] {
        if (extent.width == info_.framebuffer_extent.width && extent.height == info_.framebuffer_extent.height)
            return;
        backend_->Resize(extent);
        auto updated = ReadAcceptedInfo();
        if (updated.backend != config_.backend || updated.framebuffer_extent.width != extent.width ||
            updated.framebuffer_extent.height != extent.height)
            Throw(RenderErrorCategory::SubmissionFailure, "Resize", "backend returned inconsistent extent or identity");
        info_ = std::move(updated);
        lifecycle_ = extent.IsSuspended() ? RendererLifecycle::Suspended : RendererLifecycle::Ready;
    });
}

void RendererState::SetVSync(bool enabled) {
    Invoke("SetVSync", [&] {
        if (enabled == config_.vsync) return;
        backend_->SetVSync(enabled);
        auto updated = ReadAcceptedInfo();
        if (updated.backend != config_.backend)
            Throw(RenderErrorCategory::SubmissionFailure, "SetVSync", "backend identity changed");
        info_ = std::move(updated);
        lifecycle_ = info_.framebuffer_extent.IsSuspended()
                         ? RendererLifecycle::Suspended : RendererLifecycle::Ready;
        config_.vsync = enabled;
    });
}

RendererInfo RendererState::GetInfo() const {
    return const_cast<RendererState*>(this)->Invoke("GetInfo", [&] { return info_; }, true);
}

std::vector<GpuFrameSample> RendererState::PollGpuSamples() {
    return Invoke("PollGpuSamples", [&] {
        auto samples = backend_->PollGpuSamples();
        for (const auto& sample : samples) {
            if (!has_backend_frame_ || sample.frame_id > last_backend_frame_id_ ||
                sample.valid != sample.gpu_draw_time.has_value() ||
                (sample.missing_reason != GpuSampleMissingReason::None &&
                 sample.missing_reason != GpuSampleMissingReason::Unsupported &&
                 sample.missing_reason != GpuSampleMissingReason::Discarded &&
                 sample.missing_reason != GpuSampleMissingReason::DeviceFailure) ||
                (sample.valid && (sample.missing_reason != GpuSampleMissingReason::None ||
                                 sample.gpu_draw_time->count() < 0)) ||
                (!sample.valid && sample.missing_reason == GpuSampleMissingReason::None))
                Throw(RenderErrorCategory::SubmissionFailure, "PollGpuSamples", "backend returned inconsistent GPU sample");
        }
        for (const auto& sample : samples)
            if (sample.missing_reason == GpuSampleMissingReason::DeviceFailure)
                RecordFatal(RenderErrorCategory::DeviceLost);
        return samples;
    });
}

void RendererState::Shutdown() {
    Invoke("Shutdown", [&] {
        if (lifecycle_ == RendererLifecycle::Stopped) return;
        lifecycle_ = RendererLifecycle::Failed;
        backend_->Shutdown();
        resources_.Clear();
        lifecycle_ = RendererLifecycle::Stopped;
    }, true);
}

} // namespace SymoCraft::Rendering::Detail
