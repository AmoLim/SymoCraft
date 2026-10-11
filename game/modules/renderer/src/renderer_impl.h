#pragma once

#include "backend/backend_factory.h"
#include "resource_registry.h"
#include "symocraft/renderer/error.h"

#include <functional>
#include <thread>
#include <type_traits>
#include <utility>

namespace SymoCraft::Rendering::Detail {

enum class RendererLifecycle { Ready, Suspended, Failed, Stopped };

class RendererState final {
public:
    RendererState(RendererConfig config, std::unique_ptr<RenderBackend> backend);
    ~RendererState() noexcept;
    RendererState(const RendererState&) = delete;
    RendererState& operator=(const RendererState&) = delete;

    MeshHandle CreateMesh(const MeshData& data);
    void UpdateMesh(MeshHandle handle, const MeshData& data);
    void DestroyMesh(MeshHandle handle);
    TextureArrayHandle CreateTextureArray(const TextureArrayData& data);
    void DestroyTextureArray(TextureArrayHandle handle);
    FrameResult Render(const FrameInput& frame);
    void Resize(Extent2D extent);
    void SetVSync(bool enabled);
    RendererInfo GetInfo() const;
    std::vector<GpuFrameSample> PollGpuSamples();
    void Shutdown();

private:
    class CallGuard final {
    public:
        CallGuard(RendererState& state, const char* operation, bool allow_inactive);
        ~CallGuard() { state_.in_call_ = false; }
    private:
        RendererState& state_;
    };

    [[noreturn]] void Throw(RenderErrorCategory category, const char* operation,
                            const char* message) const;
    void RecordFatal(RenderErrorCategory category) noexcept;
    RendererInfo ReadAcceptedInfo();

    template <class Function>
    decltype(auto) Invoke(const char* operation, Function&& function, bool allow_inactive = false) {
        try {
            CallGuard guard(*this, operation, allow_inactive);
            return std::invoke(std::forward<Function>(function));
        } catch (const ResourceRegistryError& error) {
            throw RenderError(error.category, BackendName(config_.backend), operation, error.what());
        } catch (const RenderError& error) {
            RecordFatal(error.Category());
            throw;
        } catch (const std::bad_alloc&) {
            Throw(RenderErrorCategory::AllocationFailure, operation, "CPU allocation failed");
        } catch (const std::length_error&) {
            Throw(RenderErrorCategory::AllocationFailure, operation, "CPU container capacity exceeded");
        } catch (const std::invalid_argument& error) {
            throw RenderError(RenderErrorCategory::InvalidInput, BackendName(config_.backend),
                              operation, error.what());
        } catch (const std::exception& error) {
            RecordFatal(RenderErrorCategory::SubmissionFailure);
            throw RenderError(RenderErrorCategory::SubmissionFailure, BackendName(config_.backend),
                              operation, error.what());
        } catch (...) {
            RecordFatal(RenderErrorCategory::SubmissionFailure);
            Throw(RenderErrorCategory::SubmissionFailure, operation, "unknown backend failure");
        }
    }

    RendererConfig config_;
    std::unique_ptr<RenderBackend> backend_;
    ResourceRegistry resources_;
    RendererInfo info_;
    const std::thread::id owner_thread_;
    mutable RendererLifecycle lifecycle_ = RendererLifecycle::Ready;
    mutable bool in_call_ = false;
    bool has_frame_ = false;
    std::uint64_t last_frame_id_ = 0;
    bool has_backend_frame_ = false;
    std::uint64_t last_backend_frame_id_ = 0;
};

} // namespace SymoCraft::Rendering::Detail
