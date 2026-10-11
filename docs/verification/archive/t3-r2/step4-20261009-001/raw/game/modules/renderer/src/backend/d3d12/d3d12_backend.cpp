#include "d3d12_backend.h"

#include "pipeline.h"
#include "native_bridge.h"
#include "symocraft/renderer/error.h"

#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <wrl/client.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <limits>
#include <map>
#include <sstream>
#include <string_view>
#include <tuple>
#include <utility>

namespace SymoCraft::Rendering::Detail {
namespace {
using Microsoft::WRL::ComPtr;
using Clock = std::chrono::steady_clock;
constexpr UINT FrameCount = 3;
constexpr std::size_t ReadySampleCount = 256;
constexpr DXGI_FORMAT StorageFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT ColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT SrgbTextureFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
constexpr DXGI_FORMAT DepthFormat = DXGI_FORMAT_D32_FLOAT;

struct EventHandle {
    HANDLE value = nullptr;
    ~EventHandle() noexcept { Reset(); }
    void Reset() noexcept { if (value) CloseHandle(std::exchange(value, nullptr)); }
};

std::string Hex(HRESULT value) {
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << static_cast<std::uint32_t>(value);
    return stream.str();
}

std::string Utf8(const wchar_t* value) {
    const int size = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (size <= 0) return "<unavailable>";
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), size, nullptr, nullptr);
    result.pop_back();
    return result;
}

D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES value{};
    value.Type = type;
    value.CreationNodeMask = value.VisibleNodeMask = 1;
    return value;
}

D3D12_RESOURCE_DESC Buffer(UINT64 bytes) {
    D3D12_RESOURCE_DESC value{};
    value.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    value.Width = bytes;
    value.Height = 1;
    value.DepthOrArraySize = value.MipLevels = 1;
    value.SampleDesc.Count = 1;
    value.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return value;
}

D3D12_RESOURCE_DESC Texture(Extent2D extent, UINT16 layers, DXGI_FORMAT format,
                           UINT samples = 1, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE) {
    D3D12_RESOURCE_DESC value{};
    value.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    value.Width = extent.width;
    value.Height = extent.height;
    value.DepthOrArraySize = layers;
    value.MipLevels = 1;
    value.Format = format;
    value.SampleDesc.Count = samples;
    value.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    value.Flags = flags;
    return value;
}

D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                                  D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = before;
    barrier.Transition.StateAfter = after;
    return barrier;
}

struct IdLess {
    bool operator()(BackendResourceId a, BackendResourceId b) const noexcept {
        return std::tie(a.owner, a.slot, a.generation) < std::tie(b.owner, b.slot, b.generation);
    }
};

struct MeshResource {
    ComPtr<ID3D12Resource> buffer;
    UINT vertices = 0;
    UINT bytes = 0;
    std::array<double, 3> max_coordinate{};
};

struct TextureResource {
    ComPtr<ID3D12Resource> texture;
    DXGI_FORMAT format = StorageFormat;
    UINT layers = 0;
};

struct FrameContext {
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> commands;
    ComPtr<ID3D12DescriptorHeap> descriptors;
    ComPtr<ID3D12Resource> selection;
    ComPtr<ID3D12Resource> screenshot;
    ComPtr<ID3D12Resource> query_readback;
    std::vector<std::shared_ptr<MeshResource>> mesh_refs;
    std::vector<std::shared_ptr<TextureResource>> texture_refs;
    UINT64 fence = 0;
    std::uint64_t frame_id = 0;
    bool query_pending = false;
};

struct NativeQuarantine {
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12InfoQueue> diagnostics;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12Fence> fence;
    ComPtr<ID3D12DescriptorHeap> rtv_heap, dsv_heap;
    ComPtr<ID3D12QueryHeap> queries;
    D3D12::PipelineSet pipelines;
    std::array<ComPtr<ID3D12Resource>, FrameCount> backbuffers, depth, color;
    std::array<FrameContext, FrameCount> frames;
    std::map<BackendResourceId, std::shared_ptr<MeshResource>, IdLess> meshes;
    std::map<BackendResourceId, std::shared_ptr<TextureResource>, IdLess> textures;
    ComPtr<ID3D12CommandAllocator> upload_allocator;
    ComPtr<ID3D12GraphicsCommandList> upload_commands;
    ComPtr<ID3D12Resource> pending_texture, pending_upload;
    EventHandle completion, latency;
};

float GpuFloat(double value, const char* input) {
    if (!std::isfinite(value) || std::abs(value) > std::numeric_limits<float>::max())
        throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Render",
                          "finite CPU input produces an unrepresentable GPU float", {}, input);
    const auto narrowed = static_cast<float>(value);
    if (!std::isfinite(narrowed))
        throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Render",
                          "GPU float conversion is not finite", {}, input);
    return narrowed;
}

glm::mat4 GpuMatrix(const glm::dmat4& matrix, const char* input) {
    glm::mat4 result{1.0f};
    for (glm::length_t column = 0; column < 4; ++column)
        for (glm::length_t row = 0; row < 4; ++row)
            result[column][row] = GpuFloat(matrix[column][row], input);
    return result;
}

void ValidateGpuPosition(const glm::mat4& matrix, const std::array<double, 3>& max_coordinate,
                         const char* input) {
    // An otherwise finite matrix can overflow float shader products/sums.
    // The absolute bound also excludes cancellation through infinite products.
    for (glm::length_t row = 0; row < 4; ++row) {
        double bound = std::abs(static_cast<double>(matrix[3][row]));
        for (glm::length_t axis = 0; axis < 3; ++axis)
            bound += std::abs(static_cast<double>(matrix[axis][row])) * max_coordinate[axis];
        if (!std::isfinite(bound) || bound > static_cast<double>(std::numeric_limits<float>::max()) * 0.999999)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Render",
                              "finite CPU input may overflow float shader position arithmetic", {}, input);
    }
}

struct CameraMatrices {
    glm::dmat4 projection{1.0};
    glm::dmat4 view{1.0};
};

CameraMatrices CameraMatrix(const CameraParameters& camera, Extent2D extent) {
    const glm::dvec3 forward = glm::normalize(glm::dvec3(camera.forward));
    const glm::dvec3 side = glm::normalize(glm::cross(forward, glm::normalize(glm::dvec3(camera.up))));
    const glm::dvec3 up = glm::cross(side, forward);
    const glm::dvec3 eye{camera.position};
    glm::dmat4 view{1.0};
    for (glm::length_t axis = 0; axis < 3; ++axis) {
        view[axis][0] = side[axis];
        view[axis][1] = up[axis];
        view[axis][2] = -forward[axis];
    }
    // Do not form eye + forward: a finite, distant eye can erase that direction.
    view[3][0] = -glm::dot(side, eye);
    view[3][1] = -glm::dot(up, eye);
    view[3][2] = glm::dot(forward, eye);
    const auto projection = glm::perspectiveRH_ZO(static_cast<double>(camera.vertical_fov_radians),
                                                  extent.AspectRatio(), static_cast<double>(camera.near_plane),
                                                  static_cast<double>(camera.far_plane));
    return {projection, view};
}

class D3D12Backend final : public RenderBackend {
public:
    D3D12Backend(Window& window, const RendererConfig& config) : config_(config) {
        try { Initialize(window); }
        catch (...) { ShutdownNoexcept(); throw; }
    }
    ~D3D12Backend() noexcept override { ShutdownNoexcept(); }

    RendererInfo GetInfo() const override { return info_; }
    void CreateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override;
    void UpdateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override;
    void DestroyMesh(BackendResourceId id) override;
    void CreateTextureArray(BackendResourceId id, std::shared_ptr<const TextureArrayData> data) override;
    void DestroyTextureArray(BackendResourceId id) override;
    FrameResult Render(const PreparedFrame& input) override;
    void Resize(Extent2D extent) override;
    void SetVSync(bool enabled) override;
    std::vector<GpuFrameSample> PollGpuSamples() override;
    void Shutdown() override;
    void ShutdownNoexcept() noexcept override;

private:
    template<class F> decltype(auto) Operation(const char* name, F&& action) {
        RequireRunning(name);
        irreversible_ = false;
        try { return std::forward<F>(action)(); }
        catch (const RenderError& error) {
            if (error.Category() == RenderErrorCategory::DeviceLost ||
                error.Category() == RenderErrorCategory::Timeout ||
                error.Category() == RenderErrorCategory::SubmissionFailure ||
                error.Category() == RenderErrorCategory::InitializationFailure)
                failed_ = true;
            if (irreversible_ && error.Category() != RenderErrorCategory::DeviceLost &&
                error.Category() != RenderErrorCategory::Timeout &&
                error.Category() != RenderErrorCategory::SubmissionFailure) {
                failed_ = true;
                throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", name,
                                  error.what(), error.NativeCode(), error.Diagnostic());
            }
            throw;
        } catch (const std::bad_alloc&) {
            if (irreversible_) failed_ = true;
            throw RenderError(irreversible_ ? RenderErrorCategory::SubmissionFailure
                                           : RenderErrorCategory::AllocationFailure,
                              "D3D12", name, "CPU allocation failed");
        } catch (const std::exception& error) {
            if (irreversible_) failed_ = true;
            throw RenderError(irreversible_ ? RenderErrorCategory::SubmissionFailure
                                           : RenderErrorCategory::InvalidInput,
                              "D3D12", name, error.what());
        } catch (...) {
            failed_ = true;
            throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", name,
                              "unknown backend failure");
        }
    }

    void Initialize(Window& window);
    void SelectDevice();
    void CreateSwapchain(Extent2D extent);
    void CreateSizeResources(Extent2D extent);
    std::array<ComPtr<ID3D12Resource>, FrameCount> MakeDepth(Extent2D extent);
    std::array<ComPtr<ID3D12Resource>, FrameCount> MakeColor(Extent2D extent);
    void ReleaseSizeResources() noexcept;
    void Check(HRESULT result, const char* operation, bool initializing = false);
    void CheckDebug(const char* operation);
    void RequireRunning(const char* operation) const;
    void WaitFor(UINT64 value, const char* operation);
    void WaitEvent(HANDLE event, const char* operation);
    UINT64 Completed(const char* operation);
    UINT64 Signal(const char* operation);
    Extent2D NativeExtent() const;
    std::shared_ptr<MeshResource> PrepareMesh(const MeshData& data);
    ComPtr<ID3D12Resource> UploadBuffer(const void* bytes, UINT64 size, const char* operation);
    void CollectQuery(FrameContext& frame);
    void PushSample(const GpuFrameSample& sample) noexcept;
    void ClearNative() noexcept;
    bool MayReleaseNative() const noexcept;
    void QuarantineNative() noexcept;

    RendererConfig config_;
    RendererInfo info_;
    HWND window_ = nullptr;
    bool failed_ = false;
    bool stopped_ = false;
    bool quarantined_ = false;
    bool irreversible_ = false;
    bool initialized_ = false;
    bool has_native_work_ = false;
    bool unfenced_work_ = false;
    bool tearing_ = false;
    UINT swap_flags_ = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
    UINT rtv_stride_ = 0;
    UINT dsv_stride_ = 0;
    UINT srv_stride_ = 0;
    UINT64 timestamp_frequency_ = 0;
    UINT64 next_fence_ = 1;
    UINT64 last_submitted_ = 0;
    ComPtr<IDXGIFactory6> factory_;
    ComPtr<IDXGIAdapter1> adapter_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12InfoQueue> diagnostics_;
    ComPtr<ID3D12CommandQueue> queue_;
    ComPtr<IDXGISwapChain3> swapchain_;
    ComPtr<ID3D12Fence> fence_;
    ComPtr<ID3D12DescriptorHeap> rtv_heap_;
    ComPtr<ID3D12DescriptorHeap> dsv_heap_;
    ComPtr<ID3D12QueryHeap> queries_;
    D3D12::PipelineSet pipelines_;
    std::array<ComPtr<ID3D12Resource>, FrameCount> backbuffers_;
    std::array<ComPtr<ID3D12Resource>, FrameCount> depth_;
    std::array<ComPtr<ID3D12Resource>, FrameCount> color_;
    std::array<FrameContext, FrameCount> frames_;
    std::map<BackendResourceId, std::shared_ptr<MeshResource>, IdLess> meshes_;
    std::map<BackendResourceId, std::shared_ptr<TextureResource>, IdLess> textures_;
    std::array<GpuFrameSample, ReadySampleCount> ready_samples_;
    std::size_t ready_head_ = 0;
    std::size_t ready_count_ = 0;
    EventHandle completion_;
    EventHandle latency_;
    // A submitted texture upload remains owned even when its bounded wait fails.
    ComPtr<ID3D12CommandAllocator> upload_allocator_;
    ComPtr<ID3D12GraphicsCommandList> upload_commands_;
    ComPtr<ID3D12Resource> pending_texture_;
    ComPtr<ID3D12Resource> pending_upload_;
    // Preallocated before Initialize; the catastrophic path must not allocate.
    std::unique_ptr<NativeQuarantine> quarantine_ = std::make_unique<NativeQuarantine>();
};

void D3D12Backend::RequireRunning(const char* operation) const {
    if (failed_ || stopped_)
        throw RenderError(RenderErrorCategory::InvalidState, "D3D12", operation,
                          "backend is failed or stopped; new native operations are forbidden");
}

void D3D12Backend::Check(HRESULT result, const char* operation, bool initializing) {
    if (SUCCEEDED(result)) return;
    const HRESULT removed = device_ ? device_->GetDeviceRemovedReason() : S_OK;
    const bool lost = result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET ||
                      result == DXGI_ERROR_DEVICE_HUNG || FAILED(removed);
    const auto category = lost ? RenderErrorCategory::DeviceLost
        : result == E_OUTOFMEMORY && !irreversible_ ? RenderErrorCategory::AllocationFailure
        : initializing || !initialized_ ? RenderErrorCategory::InitializationFailure
        : RenderErrorCategory::SubmissionFailure;
    if (category != RenderErrorCategory::AllocationFailure) failed_ = true;
    throw RenderError(category, "D3D12", operation, "native call failed", Hex(result),
                      "device_removed_reason=" + Hex(removed));
}

void D3D12Backend::CheckDebug(const char* operation) {
    if (!diagnostics_) return;
    bool violation = false;
    std::ostringstream detail;
    const UINT64 count = diagnostics_->GetNumStoredMessagesAllowedByRetrievalFilter();
    for (UINT64 index = 0; index < count; ++index) {
        SIZE_T bytes = 0;
        Check(diagnostics_->GetMessage(index, nullptr, &bytes), "InfoQueue.GetMessage(size)");
        std::vector<std::byte> storage(bytes);
        auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
        Check(diagnostics_->GetMessage(index, message, &bytes), "InfoQueue.GetMessage");
        const bool bad = message->Severity == D3D12_MESSAGE_SEVERITY_WARNING ||
                         message->Severity == D3D12_MESSAGE_SEVERITY_ERROR ||
                         message->Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION;
        if (bad) {
            violation = true;
            detail << "severity=" << message->Severity << " id=" << message->ID << " text="
                   << std::string_view(message->pDescription, message->DescriptionByteLength) << '\n';
        }
    }
    diagnostics_->ClearStoredMessages();
    if (violation) {
        failed_ = true;
        throw RenderError(initialized_ ? RenderErrorCategory::SubmissionFailure
                                      : RenderErrorCategory::InitializationFailure,
                          "D3D12", operation, "Debug Layer warning/error", "InfoQueue", detail.str());
    }
}

Extent2D D3D12Backend::NativeExtent() const {
    RECT rect{};
    if (!window_ || !IsWindow(window_) || !GetClientRect(window_, &rect))
        throw RenderError(RenderErrorCategory::InvalidState, "D3D12", "NativeExtent",
                          "borrowed native window is unavailable", Hex(HRESULT_FROM_WIN32(GetLastError())));
    if (IsIconic(window_)) return {};
    return {static_cast<std::uint32_t>(std::max<LONG>(0, rect.right - rect.left)),
            static_cast<std::uint32_t>(std::max<LONG>(0, rect.bottom - rect.top))};
}

void D3D12Backend::Initialize(Window& window) {
    if (config_.requested_samples == 0 || config_.requested_samples > 32 ||
        (config_.requested_samples & (config_.requested_samples - 1)) != 0)
        throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Initialize",
                          "D3D12 requires a supported power-of-two MSAA count from 1 to 32");
    if (config_.shader_asset_root.empty())
        throw RenderError(RenderErrorCategory::AssetUnavailable, "D3D12", "Initialize",
                          "shader_asset_root must identify the production DXIL directory");
    window_ = Platform::NativeBridge::GetHandle(window);
    if (!window_ || !IsWindow(window_))
        throw RenderError(RenderErrorCategory::InvalidInput, "D3D12", "Initialize",
                          "a live Native-mode Window is required");
    if (config_.enable_validation) {
        ComPtr<ID3D12Debug> debug;
        Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "D3D12GetDebugInterface", true);
        debug->EnableDebugLayer();
    }
    Check(CreateDXGIFactory2(config_.enable_validation ? DXGI_CREATE_FACTORY_DEBUG : 0,
                            IID_PPV_ARGS(&factory_)), "CreateDXGIFactory2", true);
    SelectDevice();
    if (config_.enable_validation)
        Check(device_.As(&diagnostics_), "QueryInterface(ID3D12InfoQueue)", true);
    BOOL supported = FALSE;
    const HRESULT tearing = factory_->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING,
                                                          &supported, sizeof(supported));
    tearing_ = SUCCEEDED(tearing) && supported;
    if (!config_.vsync && !tearing_)
        throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Initialize",
                          "VSync-off Immediate mode requires actual DXGI tearing support; no mode fallback");
    if (tearing_) swap_flags_ |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
    D3D12_COMMAND_QUEUE_DESC queue_desc{};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Check(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&queue_)), "CreateCommandQueue", true);
    Check(device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_)), "CreateFence", true);
    completion_.value = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!completion_.value) Check(HRESULT_FROM_WIN32(GetLastError()), "CreateEventW", true);

    info_.actual_samples = config_.requested_samples;
    if (info_.actual_samples != 1) {
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS color{ColorFormat, info_.actual_samples, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0};
        D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS depth{DepthFormat, info_.actual_samples, D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE, 0};
        Check(device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &color, sizeof(color)),
              "CheckFeatureSupport(MSAA color)", true);
        Check(device_->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &depth, sizeof(depth)),
              "CheckFeatureSupport(MSAA depth)", true);
        if (!color.NumQualityLevels || !depth.NumQualityLevels)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Initialize",
                              "requested MSAA count is unsupported by color/depth formats; no silent downgrade",
                              {}, "requested_samples=" + std::to_string(config_.requested_samples));
    }
    pipelines_ = D3D12::CreatePipelines(device_.Get(), config_.shader_asset_root, ColorFormat,
                                      DepthFormat, info_.actual_samples);
    D3D12_DESCRIPTOR_HEAP_DESC rtv{};
    rtv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtv.NumDescriptors = FrameCount * 2;
    Check(device_->CreateDescriptorHeap(&rtv, IID_PPV_ARGS(&rtv_heap_)), "CreateDescriptorHeap(RTV)", true);
    D3D12_DESCRIPTOR_HEAP_DESC dsv{};
    dsv.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsv.NumDescriptors = FrameCount;
    Check(device_->CreateDescriptorHeap(&dsv, IID_PPV_ARGS(&dsv_heap_)), "CreateDescriptorHeap(DSV)", true);
    rtv_stride_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    dsv_stride_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    srv_stride_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    const HRESULT timing = queue_->GetTimestampFrequency(&timestamp_frequency_);
    if (FAILED(timing)) {
        const HRESULT removed = device_->GetDeviceRemovedReason();
        if (FAILED(removed)) Check(timing, "GetTimestampFrequency", true);
        timestamp_frequency_ = 0;
    }
    info_.capabilities.gpu_timestamps = timestamp_frequency_ != 0;
    if (timestamp_frequency_) {
        D3D12_QUERY_HEAP_DESC desc{};
        desc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
        desc.Count = FrameCount * 2;
        Check(device_->CreateQueryHeap(&desc, IID_PPV_ARGS(&queries_)), "CreateQueryHeap(timestamp)", true);
    }
    for (auto& frame : frames_) {
        Check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.allocator)),
              "CreateCommandAllocator(frame)", true);
        Check(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.allocator.Get(), nullptr,
                                        IID_PPV_ARGS(&frame.commands)), "CreateCommandList(frame)", true);
        Check(frame.commands->Close(), "Close(initial frame)", true);
        if (timestamp_frequency_) {
            const auto heap = Heap(D3D12_HEAP_TYPE_READBACK);
            const auto desc = Buffer(sizeof(UINT64) * 2);
            Check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                  IID_PPV_ARGS(&frame.query_readback)), "CreateResource(query readback)", true);
        }
    }
    info_.backend = Backend::D3D12;
    info_.validation_enabled = config_.enable_validation;
    info_.actual_present_mode = config_.vsync ? PresentMode::Fifo : PresentMode::Immediate;
    info_.framebuffer_format = "R8G8B8A8_UNORM RTV / swapchain storage (no output sRGB encode)";
    info_.capabilities.texture_arrays = true;
    info_.capabilities.selection_boxes = true;
    info_.capabilities.screenshots = true;
    info_.framebuffer_extent = NativeExtent();
    if (!info_.framebuffer_extent.IsSuspended()) CreateSwapchain(info_.framebuffer_extent);
    CheckDebug("Initialize");
    initialized_ = true;
}

void D3D12Backend::SelectDevice() {
    std::ostringstream rejected;
    for (UINT index = 0;; ++index) {
        ComPtr<IDXGIAdapter1> candidate;
        const HRESULT enumerated = factory_->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                        IID_PPV_ARGS(&candidate));
        if (enumerated == DXGI_ERROR_NOT_FOUND) break;
        Check(enumerated, "EnumAdapterByGpuPreference", true);
        DXGI_ADAPTER_DESC1 desc{};
        Check(candidate->GetDesc1(&desc), "Adapter.GetDesc1", true);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        ComPtr<ID3D12Device> selected;
        const HRESULT created = D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&selected));
        if (FAILED(created)) {
            rejected << Utf8(desc.Description) << " FL12_0=" << Hex(created) << '\n';
            continue;
        }
        D3D12_FEATURE_DATA_SHADER_MODEL model{D3D_SHADER_MODEL_6_0};
        const HRESULT shader_model = selected->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model));
        if (FAILED(shader_model) || model.HighestShaderModel < D3D_SHADER_MODEL_6_0) {
            rejected << Utf8(desc.Description) << " SM6_0=" << Hex(shader_model) << '\n';
            continue;
        }
        adapter_ = std::move(candidate);
        device_ = std::move(selected);
        const auto actual = device_->GetAdapterLuid();
        if (actual.HighPart != desc.AdapterLuid.HighPart || actual.LowPart != desc.AdapterLuid.LowPart)
            throw RenderError(RenderErrorCategory::InitializationFailure, "D3D12", "SelectDevice",
                              "native device and enumerated adapter LUID differ");
        info_.device_name = Utf8(desc.Description);
        std::ostringstream identity;
        identity << "LUID=" << std::hex << std::setfill('0') << std::setw(8) << static_cast<UINT>(actual.HighPart)
                 << ':' << std::setw(8) << actual.LowPart << " vendor=0x" << desc.VendorId << " device=0x" << desc.DeviceId;
        info_.device_identity = identity.str();
        LARGE_INTEGER driver{};
        Check(adapter_->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver), "CheckInterfaceSupport(driver)", true);
        std::ostringstream version;
        version << HIWORD(driver.HighPart) << '.' << LOWORD(driver.HighPart) << '.'
                << HIWORD(driver.LowPart) << '.' << LOWORD(driver.LowPart);
        info_.driver_version = version.str();
        info_.api_version = "D3D12 FL12_0 / SM6_0 (explicit hardware checks)";
        return;
    }
    throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "SelectDevice",
                      "no hardware adapter supports FL12_0 and SM6_0; software/backend fallback is forbidden",
                      {}, rejected.str());
}

std::array<ComPtr<ID3D12Resource>, FrameCount> D3D12Backend::MakeDepth(Extent2D extent) {
    std::array<ComPtr<ID3D12Resource>, FrameCount> result;
    const auto heap = Heap(D3D12_HEAP_TYPE_DEFAULT);
    const auto desc = Texture(extent, 1, DepthFormat, info_.actual_samples, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_CLEAR_VALUE clear{};
    clear.Format = DepthFormat;
    clear.DepthStencil.Depth = 1.0f;
    for (auto& depth : result)
        Check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                              &clear, IID_PPV_ARGS(&depth)), "CreateResource(depth)");
    return result;
}

std::array<ComPtr<ID3D12Resource>, FrameCount> D3D12Backend::MakeColor(Extent2D extent) {
    std::array<ComPtr<ID3D12Resource>, FrameCount> result;
    if (info_.actual_samples == 1) return result;
    const auto heap = Heap(D3D12_HEAP_TYPE_DEFAULT);
    const auto desc = Texture(extent, 1, StorageFormat, info_.actual_samples, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    D3D12_CLEAR_VALUE clear{};
    clear.Format = ColorFormat;
    clear.Color[0] = 0.529f; clear.Color[1] = 0.808f; clear.Color[2] = 0.922f; clear.Color[3] = 1.0f;
    for (auto& color : result)
        Check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                              &clear, IID_PPV_ARGS(&color)), "CreateResource(MSAA color)");
    return result;
}

void D3D12Backend::CreateSizeResources(Extent2D extent) {
    for (UINT index = 0; index < FrameCount; ++index) {
        Check(swapchain_->GetBuffer(index, IID_PPV_ARGS(&backbuffers_[index])), "Swapchain.GetBuffer");
        D3D12_RENDER_TARGET_VIEW_DESC view{};
        view.Format = ColorFormat;
        view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        auto handle = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
        handle.ptr += index * rtv_stride_;
        device_->CreateRenderTargetView(backbuffers_[index].Get(), &view, handle);
        if (color_[index]) {
            view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
            handle.ptr += FrameCount * rtv_stride_;
            device_->CreateRenderTargetView(color_[index].Get(), &view, handle);
        }
        D3D12_DEPTH_STENCIL_VIEW_DESC depth_view{};
        depth_view.Format = DepthFormat;
        depth_view.ViewDimension = info_.actual_samples == 1 ? D3D12_DSV_DIMENSION_TEXTURE2D : D3D12_DSV_DIMENSION_TEXTURE2DMS;
        auto depth_handle = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        depth_handle.ptr += index * dsv_stride_;
        device_->CreateDepthStencilView(depth_[index].Get(), &depth_view, depth_handle);
    }
    DXGI_SWAP_CHAIN_DESC1 actual{};
    Check(swapchain_->GetDesc1(&actual), "Swapchain.GetDesc1");
    if (actual.Width != extent.width || actual.Height != extent.height || actual.Format != StorageFormat ||
        actual.SampleDesc.Count != 1 || actual.BufferCount != FrameCount ||
        actual.SwapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD || actual.Flags != swap_flags_) {
        failed_ = true;
        throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", "CreateSizeResources",
                          "actual swapchain identity differs from accepted configuration");
    }
}

void D3D12Backend::CreateSwapchain(Extent2D extent) {
    depth_ = MakeDepth(extent);
    color_ = MakeColor(extent);
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = extent.width;
    desc.Height = extent.height;
    desc.Format = StorageFormat;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = FrameCount;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.Flags = swap_flags_;
    ComPtr<IDXGISwapChain1> created;
    Check(factory_->CreateSwapChainForHwnd(queue_.Get(), window_, &desc, nullptr, nullptr, &created),
          "CreateSwapChainForHwnd");
    irreversible_ = true;
    Check(created.As(&swapchain_), "QueryInterface(SwapChain3)");
    Check(factory_->MakeWindowAssociation(window_, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
    Check(swapchain_->SetMaximumFrameLatency(FrameCount - 1), "SetMaximumFrameLatency");
    latency_.value = swapchain_->GetFrameLatencyWaitableObject();
    if (!latency_.value) Check(HRESULT_FROM_WIN32(GetLastError()), "GetFrameLatencyWaitableObject");
    CreateSizeResources(extent);
}

UINT64 D3D12Backend::Completed(const char* operation) {
    const UINT64 value = fence_->GetCompletedValue();
    if (value == std::numeric_limits<UINT64>::max()) {
        failed_ = true;
        throw RenderError(RenderErrorCategory::DeviceLost, "D3D12", operation,
                          "fence reports device removal", Hex(device_->GetDeviceRemovedReason()));
    }
    return value;
}

void D3D12Backend::WaitEvent(HANDLE event, const char* operation) {
    const auto timeout = static_cast<DWORD>(std::min<std::int64_t>(config_.wait_timeout.count(), MAXDWORD - 1));
    const DWORD result = WaitForSingleObject(event, timeout);
    if (result == WAIT_OBJECT_0) return;
    failed_ = true;
    const auto removed = device_ ? device_->GetDeviceRemovedReason() : S_OK;
    throw RenderError(FAILED(removed) ? RenderErrorCategory::DeviceLost
                                    : result == WAIT_TIMEOUT ? RenderErrorCategory::Timeout
                                                             : RenderErrorCategory::SubmissionFailure,
                      "D3D12", operation, "bounded native wait did not complete",
                      result == WAIT_FAILED ? Hex(HRESULT_FROM_WIN32(GetLastError())) : Hex(static_cast<HRESULT>(result)),
                      "wait_timeout_ms=" + std::to_string(timeout) + " device_removed_reason=" + Hex(removed));
}

void D3D12Backend::WaitFor(UINT64 value, const char* operation) {
    if (!value || Completed(operation) >= value) return;
    Check(fence_->SetEventOnCompletion(value, completion_.value), "Fence.SetEventOnCompletion");
    WaitEvent(completion_.value, operation);
    if (Completed(operation) < value) {
        failed_ = true;
        throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", operation,
                          "completed event did not cover the submitted fence");
    }
}

UINT64 D3D12Backend::Signal(const char* operation) {
    if (next_fence_ == std::numeric_limits<UINT64>::max()) {
        failed_ = true;
        throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", operation, "fence identity exhausted");
    }
    const auto value = next_fence_++;
    Check(queue_->Signal(fence_.Get(), value), operation);
    last_submitted_ = value;
    unfenced_work_ = false;
    return value;
}

ComPtr<ID3D12Resource> D3D12Backend::UploadBuffer(const void* bytes, UINT64 size, const char* operation) {
    ComPtr<ID3D12Resource> result;
    if (!size) return result;
    const auto heap = Heap(D3D12_HEAP_TYPE_UPLOAD);
    const auto desc = Buffer(size);
    Check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                          nullptr, IID_PPV_ARGS(&result)), operation);
    void* mapped = nullptr;
    const D3D12_RANGE no_read{0, 0};
    Check(result->Map(0, &no_read, &mapped), "UploadBuffer.Map");
    std::memcpy(mapped, bytes, static_cast<std::size_t>(size));
    const D3D12_RANGE written{0, static_cast<SIZE_T>(size)};
    result->Unmap(0, &written);
    return result;
}

std::shared_ptr<MeshResource> D3D12Backend::PrepareMesh(const MeshData& data) {
    if (data.vertices.size() > std::numeric_limits<UINT>::max() / sizeof(BlockVertex3D))
        throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "PrepareMesh", "vertex-buffer view exceeds UINT range");
    auto resource = std::make_shared<MeshResource>();
    resource->vertices = static_cast<UINT>(data.vertices.size());
    resource->bytes = resource->vertices * sizeof(BlockVertex3D);
    for (const auto& vertex : data.vertices)
        for (glm::length_t axis = 0; axis < 3; ++axis)
            resource->max_coordinate[axis] = std::max(resource->max_coordinate[axis],
                std::abs(static_cast<double>(static_cast<float>(vertex.pos_coord[axis]))));
    resource->buffer = UploadBuffer(data.vertices.data(), resource->bytes, "CreateResource(mesh upload)");
    CheckDebug("PrepareMesh");
    return resource;
}

void D3D12Backend::CreateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) {
    Operation("CreateMesh", [&] {
        if (meshes_.contains(id)) throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "CreateMesh", "duplicate native mesh identity");
        auto resource = PrepareMesh(*data);
        meshes_.emplace(id, std::move(resource));
    });
}

void D3D12Backend::UpdateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) {
    Operation("UpdateMesh", [&] {
        const auto found = meshes_.find(id);
        if (found == meshes_.end()) throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "UpdateMesh", "unknown native mesh identity");
        auto resource = PrepareMesh(*data);
        found->second = std::move(resource);
    });
}

void D3D12Backend::DestroyMesh(BackendResourceId id) {
    Operation("DestroyMesh", [&] {
        if (!meshes_.erase(id)) throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "DestroyMesh", "unknown native mesh identity");
    });
}

void D3D12Backend::CreateTextureArray(BackendResourceId id, std::shared_ptr<const TextureArrayData> data) {
    Operation("CreateTextureArray", [&] {
        if (textures_.contains(id)) throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "CreateTextureArray", "duplicate native texture identity");
        if (data->width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || data->height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
            data->layers > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "CreateTextureArray", "texture dimensions exceed device limits");
        auto resource = std::make_shared<TextureResource>();
        resource->format = data->color_space == ColorSpace::SRGB ? SrgbTextureFormat : StorageFormat;
        resource->layers = data->layers;
        // Reserve the map node before upload submits so acceptance is no-throw.
        std::map<BackendResourceId, std::shared_ptr<TextureResource>, IdLess> staging;
        staging.emplace(id, resource);
        const auto desc = Texture({data->width, data->height}, static_cast<UINT16>(data->layers), resource->format);
        const auto default_heap = Heap(D3D12_HEAP_TYPE_DEFAULT);
        Check(device_->CreateCommittedResource(&default_heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST,
                                              nullptr, IID_PPV_ARGS(&resource->texture)), "CreateResource(texture)");
        std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(data->layers);
        std::vector<UINT> rows(data->layers);
        std::vector<UINT64> row_sizes(data->layers);
        UINT64 upload_bytes = 0;
        device_->GetCopyableFootprints(&desc, 0, data->layers, 0, footprints.data(), rows.data(), row_sizes.data(), &upload_bytes);
        if (upload_bytes > std::numeric_limits<SIZE_T>::max())
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "CreateTextureArray", "upload footprint exceeds host address range");
        const auto upload_heap = Heap(D3D12_HEAP_TYPE_UPLOAD);
        const auto upload_desc = Buffer(upload_bytes);
        ComPtr<ID3D12Resource> upload;
        Check(device_->CreateCommittedResource(&upload_heap, D3D12_HEAP_FLAG_NONE, &upload_desc, D3D12_RESOURCE_STATE_GENERIC_READ,
                                              nullptr, IID_PPV_ARGS(&upload)), "CreateResource(texture upload)");
        void* mapped = nullptr;
        const D3D12_RANGE no_read{0, 0};
        Check(upload->Map(0, &no_read, &mapped), "TextureUpload.Map");
        for (UINT layer = 0; layer < data->layers; ++layer)
            for (UINT row = 0; row < data->height; ++row)
                std::memcpy(static_cast<std::byte*>(mapped) + footprints[layer].Offset + row * footprints[layer].Footprint.RowPitch,
                            data->pixels.data() + (static_cast<std::size_t>(layer) * data->height + row) * data->width * 4,
                            static_cast<std::size_t>(data->width) * 4);
        const D3D12_RANGE written{0, static_cast<SIZE_T>(upload_bytes)};
        upload->Unmap(0, &written);
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> commands;
        Check(device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "CreateCommandAllocator(texture upload)");
        Check(device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                        IID_PPV_ARGS(&commands)), "CreateCommandList(texture upload)");
        for (UINT layer = 0; layer < data->layers; ++layer) {
            D3D12_TEXTURE_COPY_LOCATION destination{};
            destination.pResource = resource->texture.Get();
            destination.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            destination.SubresourceIndex = layer;
            D3D12_TEXTURE_COPY_LOCATION source{};
            source.pResource = upload.Get();
            source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            source.PlacedFootprint = footprints[layer];
            commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
        }
        const auto barrier = Transition(resource->texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commands->ResourceBarrier(1, &barrier);
        Check(commands->Close(), "Close(texture upload)");
        CheckDebug("TextureUpload.Record");
        pending_texture_ = resource->texture;
        pending_upload_ = upload;
        upload_allocator_ = allocator;
        upload_commands_ = commands;
        irreversible_ = true;
        ID3D12CommandList* lists[]{commands.Get()};
        has_native_work_ = unfenced_work_ = true;
        queue_->ExecuteCommandLists(1, lists);
        const auto submitted = Signal("Signal(texture upload)");
        WaitFor(submitted, "TextureUpload.Wait");
        CheckDebug("TextureUpload.Complete");
        textures_.insert(staging.extract(id));
        pending_texture_.Reset(); pending_upload_.Reset();
        upload_commands_.Reset(); upload_allocator_.Reset();
    });
}

void D3D12Backend::DestroyTextureArray(BackendResourceId id) {
    Operation("DestroyTextureArray", [&] {
        if (!textures_.erase(id)) throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "DestroyTextureArray", "unknown native texture identity");
    });
}

void D3D12Backend::PushSample(const GpuFrameSample& sample) noexcept {
    if (ready_count_ == ReadySampleCount) {
        // A consumer that never polls cannot grow backend storage without bound.
        OutputDebugStringA("Symocraft D3D12: GPU sample ring overflow; oldest completed identity discarded.\n");
        ready_head_ = (ready_head_ + 1) % ReadySampleCount;
        --ready_count_;
    }
    ready_samples_[(ready_head_ + ready_count_) % ReadySampleCount] = sample;
    ++ready_count_;
}

void D3D12Backend::CollectQuery(FrameContext& frame) {
    if (!frame.query_pending || Completed("CollectGpuQuery") < frame.fence) return;
    const D3D12_RANGE read{0, sizeof(UINT64) * 2};
    void* mapped = nullptr;
    Check(frame.query_readback->Map(0, &read, &mapped), "GpuQuery.Readback.Map");
    std::array<UINT64, 2> ticks{};
    std::memcpy(ticks.data(), mapped, sizeof(ticks));
    const D3D12_RANGE no_write{0, 0};
    frame.query_readback->Unmap(0, &no_write);
    GpuFrameSample sample;
    sample.frame_id = frame.frame_id;
    if (ticks[1] >= ticks[0]) {
        const long double nanoseconds = static_cast<long double>(ticks[1] - ticks[0]) * 1'000'000'000.0L / timestamp_frequency_;
        if (nanoseconds <= std::numeric_limits<std::chrono::nanoseconds::rep>::max()) {
            sample.valid = true;
            sample.gpu_draw_time = std::chrono::nanoseconds(static_cast<std::chrono::nanoseconds::rep>(nanoseconds));
            sample.missing_reason = GpuSampleMissingReason::None;
        }
    }
    PushSample(sample);
    frame.query_pending = false;
}

FrameResult D3D12Backend::Render(const PreparedFrame& input) {
    return Operation("Render", [&] {
        const auto prepare_begin = Clock::now();
        FrameResult result;
        result.frame_id = input.frame_id;
        const auto native = NativeExtent();
        if (native.IsSuspended()) {
            info_.framebuffer_extent = native;
            result.skip_reason = FrameSkipReason::ZeroExtent;
            PushSample({input.frame_id, false, std::nullopt, GpuSampleMissingReason::Discarded});
            return result;
        }
        if (!swapchain_ || input.extent.width != info_.framebuffer_extent.width || input.extent.height != info_.framebuffer_extent.height ||
            native.width != input.extent.width || native.height != input.extent.height)
            throw RenderError(RenderErrorCategory::InvalidState, "D3D12", "Render", "native extent changed; Resize must accept it before Render");
        if (input.draws.size() > D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_1)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Render", "frame exceeds shader-visible descriptor capacity");
        const UINT index = swapchain_->GetCurrentBackBufferIndex();
        if (index >= FrameCount) throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", "Render", "invalid backbuffer index");
        auto& frame = frames_[index];
        WaitFor(frame.fence, "FrameContext.Wait");
        CollectQuery(frame);
        Check(frame.allocator->Reset(), "DiscardFrame.Allocator.Reset");
        Check(frame.commands->Reset(frame.allocator.Get(), nullptr), "DiscardFrame.Commands.Reset");
        Check(frame.commands->Close(), "DiscardFrame.Commands.Close");
        frame.mesh_refs.clear(); frame.texture_refs.clear();
        frame.descriptors.Reset(); frame.selection.Reset(); frame.screenshot.Reset();
        frame.mesh_refs.reserve(input.draws.size()); frame.texture_refs.reserve(input.draws.size());
        std::vector<glm::mat4> transforms;
        transforms.reserve(input.draws.size());
        const auto camera = CameraMatrix(input.camera, input.extent);
        for (const auto& draw : input.draws) {
            const auto mesh = meshes_.find(draw.mesh);
            const auto texture = textures_.find(draw.texture);
            if (mesh == meshes_.end() || texture == textures_.end())
                throw RenderError(RenderErrorCategory::InvalidHandle, "D3D12", "Render", "prepared resource has no accepted native version");
            frame.mesh_refs.push_back(mesh->second);
            frame.texture_refs.push_back(texture->second);
            // Cancel matching distant eye/model translations before projection
            // adds its near-plane term; VP * model can lose that term entirely.
            const auto matrix = GpuMatrix(camera.projection * (camera.view * glm::dmat4(draw.object_transform)),
                                          "projection * (view * object_transform)");
            if (mesh->second->vertices) ValidateGpuPosition(matrix, mesh->second->max_coordinate, "mesh clip position");
            transforms.push_back(matrix);
        }
        if (!input.draws.empty()) {
            D3D12_DESCRIPTOR_HEAP_DESC heap{};
            heap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            heap.NumDescriptors = static_cast<UINT>(input.draws.size());
            heap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            Check(device_->CreateDescriptorHeap(&heap, IID_PPV_ARGS(&frame.descriptors)), "CreateDescriptorHeap(frame SRV)");
            auto handle = frame.descriptors->GetCPUDescriptorHandleForHeapStart();
            for (const auto& texture : frame.texture_refs) {
                D3D12_SHADER_RESOURCE_VIEW_DESC view_desc{};
                view_desc.Format = texture->format;
                view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                view_desc.Texture2DArray.MipLevels = 1;
                view_desc.Texture2DArray.ArraySize = texture->layers;
                device_->CreateShaderResourceView(texture->texture.Get(), &view_desc, handle);
                handle.ptr += srv_stride_;
            }
        }
        glm::mat4 selection_camera{1.0f};
        if (input.selection) {
            selection_camera = GpuMatrix(camera.projection * camera.view, "selection camera");
            constexpr std::array<glm::vec3, 8> corners{{{0,0,0}, {0,0,1}, {1,0,1}, {1,0,0},
                                                       {0,1,0}, {0,1,1}, {1,1,1}, {1,1,0}}};
            constexpr std::array<UINT, 24> edges{7,6,6,2,2,3,3,7,7,4,6,5,2,1,3,0,5,4,4,0,0,1,1,5};
            std::array<LineVertex3D, 24> vertices;
            const glm::dvec3 minimum{input.selection->minimum};
            const glm::dvec3 maximum{input.selection->maximum};
            const auto center = (minimum + maximum) * 0.5;
            const auto size = maximum - minimum;
            std::array<double, 3> max_coordinate{};
            for (std::size_t vertex = 0; vertex < edges.size(); ++vertex) {
                const auto position = center + (glm::dvec3(corners[edges[vertex]]) - glm::dvec3(0.5)) * size * 1.004;
                for (glm::length_t axis = 0; axis < 3; ++axis) {
                    vertices[vertex].pos_coord[axis] = GpuFloat(position[axis], "selection expanded position");
                    max_coordinate[axis] = std::max(max_coordinate[axis],
                                                  std::abs(static_cast<double>(vertices[vertex].pos_coord[axis])));
                }
            }
            ValidateGpuPosition(selection_camera, max_coordinate, "selection clip position");
            frame.selection = UploadBuffer(vertices.data(), sizeof(vertices), "CreateResource(selection upload)");
            result.statistics.upload_bytes += sizeof(vertices);
        }
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT capture_footprint{};
        UINT64 capture_bytes = 0;
        if (input.request_screenshot) {
            Screenshot screenshot;
            screenshot.extent = input.extent;
            screenshot.pixels.resize(static_cast<std::size_t>(input.extent.width) * input.extent.height * 4);
            result.screenshot = std::move(screenshot);
            const auto desc = backbuffers_[index]->GetDesc();
            device_->GetCopyableFootprints(&desc, 0, 1, 0, &capture_footprint, nullptr, nullptr, &capture_bytes);
            const auto heap = Heap(D3D12_HEAP_TYPE_READBACK);
            const auto buffer = Buffer(capture_bytes);
            Check(device_->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr, IID_PPV_ARGS(&frame.screenshot)), "CreateResource(screenshot readback)");
        }
        Check(frame.allocator->Reset(), "FrameAllocator.Reset");
        Check(frame.commands->Reset(frame.allocator.Get(), pipelines_.mesh.Get()), "FrameCommands.Reset");
        auto* commands = frame.commands.Get();
        auto* buffer = backbuffers_[index].Get();
        const bool msaa = info_.actual_samples > 1;
        const auto backbuffer_state = msaa ? D3D12_RESOURCE_STATE_RESOLVE_DEST : D3D12_RESOURCE_STATE_RENDER_TARGET;
        const auto before = Transition(buffer, D3D12_RESOURCE_STATE_PRESENT, backbuffer_state);
        commands->ResourceBarrier(1, &before);
        auto rtv = rtv_heap_->GetCPUDescriptorHandleForHeapStart();
        rtv.ptr += (index + (msaa ? FrameCount : 0)) * rtv_stride_;
        auto dsv = dsv_heap_->GetCPUDescriptorHandleForHeapStart();
        dsv.ptr += index * dsv_stride_;
        commands->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
        constexpr float clear[]{0.529f, 0.808f, 0.922f, 1.0f};
        commands->ClearRenderTargetView(rtv, clear, 0, nullptr);
        commands->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        const D3D12_VIEWPORT viewport{0, 0, static_cast<float>(input.extent.width), static_cast<float>(input.extent.height), 0, 1};
        const D3D12_RECT scissor{0, 0, static_cast<LONG>(input.extent.width), static_cast<LONG>(input.extent.height)};
        commands->RSSetViewports(1, &viewport);
        commands->RSSetScissorRects(1, &scissor);
        commands->SetGraphicsRootSignature(pipelines_.root_signature.Get());
        if (frame.descriptors) {
            ID3D12DescriptorHeap* heaps[]{frame.descriptors.Get()};
            commands->SetDescriptorHeaps(1, heaps);
        }
        commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        if (timestamp_frequency_) commands->EndQuery(queries_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2);
        for (std::size_t draw = 0; draw < frame.mesh_refs.size(); ++draw) {
            const auto& mesh = *frame.mesh_refs[draw];
            if (!mesh.vertices) continue;
            const D3D12_VERTEX_BUFFER_VIEW vertices{mesh.buffer->GetGPUVirtualAddress(), mesh.bytes, sizeof(BlockVertex3D)};
            auto descriptor = frame.descriptors->GetGPUDescriptorHandleForHeapStart();
            descriptor.ptr += draw * srv_stride_;
            commands->SetGraphicsRoot32BitConstants(0, 16, glm::value_ptr(transforms[draw]), 0);
            commands->SetGraphicsRootDescriptorTable(1, descriptor);
            commands->IASetVertexBuffers(0, 1, &vertices);
            commands->DrawInstanced(mesh.vertices, 1, 0, 0);
            ++result.statistics.draw_calls;
            result.statistics.drawn_vertices += mesh.vertices;
        }
        if (frame.selection) {
            commands->SetPipelineState(pipelines_.selection.Get());
            commands->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
            const D3D12_VERTEX_BUFFER_VIEW vertices{frame.selection->GetGPUVirtualAddress(), sizeof(LineVertex3D) * 24, sizeof(LineVertex3D)};
            commands->SetGraphicsRoot32BitConstants(0, 16, glm::value_ptr(selection_camera), 0);
            commands->IASetVertexBuffers(0, 1, &vertices);
            commands->DrawInstanced(24, 1, 0, 0);
            ++result.statistics.draw_calls;
            result.statistics.drawn_vertices += 24;
        }
        if (timestamp_frequency_) {
            commands->EndQuery(queries_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2 + 1);
            commands->ResolveQueryData(queries_.Get(), D3D12_QUERY_TYPE_TIMESTAMP, index * 2, 2, frame.query_readback.Get(), 0);
        }
        if (msaa) {
            const auto resolve = Transition(color_[index].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
            commands->ResourceBarrier(1, &resolve);
            commands->ResolveSubresource(buffer, 0, color_[index].Get(), 0, StorageFormat);
            const auto restore = Transition(color_[index].Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commands->ResourceBarrier(1, &restore);
        }
        auto final_state = backbuffer_state;
        if (input.request_screenshot) {
            const auto copy = Transition(buffer, final_state, D3D12_RESOURCE_STATE_COPY_SOURCE);
            commands->ResourceBarrier(1, &copy);
            D3D12_TEXTURE_COPY_LOCATION source{};
            source.pResource = buffer;
            source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
            D3D12_TEXTURE_COPY_LOCATION destination{};
            destination.pResource = frame.screenshot.Get();
            destination.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
            destination.PlacedFootprint = capture_footprint;
            commands->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
            final_state = D3D12_RESOURCE_STATE_COPY_SOURCE;
        }
        const auto present = Transition(buffer, final_state, D3D12_RESOURCE_STATE_PRESENT);
        commands->ResourceBarrier(1, &present);
        Check(commands->Close(), "Close(frame)");
        CheckDebug("Render.Record");
        result.statistics.cpu_prepare = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - prepare_begin);
        const auto latency_begin = Clock::now();
        WaitEvent(latency_.value, "PresentationLatency.Wait");
        const auto latency_wait = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - latency_begin);
        const auto submit_begin = Clock::now();
        irreversible_ = true;
        ID3D12CommandList* lists[]{commands};
        has_native_work_ = unfenced_work_ = true;
        queue_->ExecuteCommandLists(1, lists);
        // Establish draw retirement even if Present or the subsequent Signal fails.
        frame.fence = Signal("Signal(frame draws)");
        result.statistics.cpu_submit = std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - submit_begin);
        const auto present_begin = Clock::now();
        DXGI_PRESENT_PARAMETERS parameters{};
        const UINT flags = DXGI_PRESENT_DO_NOT_WAIT | (!config_.vsync && tearing_ ? DXGI_PRESENT_ALLOW_TEARING : 0);
        unfenced_work_ = true;
        const HRESULT presented = swapchain_->Present1(config_.vsync ? 1 : 0, flags, &parameters);
        // Signal even a failed Present so queued draw ownership has a known fence.
        frame.fence = Signal("Signal(frame including Present)");
        frame.frame_id = input.frame_id;
        frame.query_pending = timestamp_frequency_ != 0;
        result.statistics.present_wait = latency_wait +
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - present_begin);
        Check(presented, "Present1(nonblocking)");
        if (presented == DXGI_STATUS_OCCLUDED) {
            result.skip_reason = FrameSkipReason::Occluded;
            result.screenshot.reset();
        } else {
            result.status = FrameStatus::Presented;
            result.skip_reason = FrameSkipReason::None;
        }
        if (!timestamp_frequency_) PushSample({input.frame_id, false, std::nullopt, GpuSampleMissingReason::Unsupported});
        if (result.screenshot) {
            WaitFor(frame.fence, "Screenshot.Wait");
            const D3D12_RANGE read{static_cast<SIZE_T>(capture_footprint.Offset), static_cast<SIZE_T>(capture_bytes)};
            void* mapped = nullptr;
            Check(frame.screenshot->Map(0, &read, &mapped), "Screenshot.Readback.Map");
            for (UINT row = 0; row < input.extent.height; ++row)
                std::memcpy(result.screenshot->pixels.data() + static_cast<std::size_t>(row) * input.extent.width * 4,
                            static_cast<std::byte*>(mapped) + capture_footprint.Offset + row * capture_footprint.Footprint.RowPitch,
                            static_cast<std::size_t>(input.extent.width) * 4);
            const D3D12_RANGE no_write{0, 0};
            frame.screenshot->Unmap(0, &no_write);
        }
        CheckDebug("Render.Submitted");
        return result;
    });
}

void D3D12Backend::ReleaseSizeResources() noexcept {
    for (auto& frame : frames_) {
        frame.mesh_refs.clear(); frame.texture_refs.clear();
        frame.descriptors.Reset(); frame.selection.Reset(); frame.screenshot.Reset();
    }
    for (auto& resource : backbuffers_) resource.Reset();
    for (auto& resource : depth_) resource.Reset();
    for (auto& resource : color_) resource.Reset();
}

void D3D12Backend::Resize(Extent2D extent) {
    Operation("Resize", [&] {
        if (extent.IsSuspended()) { info_.framebuffer_extent = extent; return; }
        if (extent.width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || extent.height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "Resize", "framebuffer dimensions exceed device limits");
        if (!swapchain_) {
            CreateSwapchain(extent);
        } else {
            // Allocate replacements before touching the last accepted swapchain.
            auto depth = MakeDepth(extent);
            auto color = MakeColor(extent);
            WaitFor(last_submitted_, "Resize.Wait");
            for (auto& frame : frames_) CollectQuery(frame);
            for (auto& frame : frames_) {
                Check(frame.allocator->Reset(), "Resize.Allocator.Reset");
                Check(frame.commands->Reset(frame.allocator.Get(), nullptr), "Resize.Commands.Reset");
                Check(frame.commands->Close(), "Resize.Commands.Close");
            }
            irreversible_ = true;
            ReleaseSizeResources();
            depth_ = std::move(depth); color_ = std::move(color);
            Check(swapchain_->ResizeBuffers(FrameCount, extent.width, extent.height, StorageFormat, swap_flags_), "ResizeBuffers");
            CreateSizeResources(extent);
        }
        CheckDebug("Resize");
        info_.framebuffer_extent = extent;
    });
}

void D3D12Backend::SetVSync(bool enabled) {
    Operation("SetVSync", [&] {
        if (!enabled && !tearing_)
            throw RenderError(RenderErrorCategory::Unsupported, "D3D12", "SetVSync",
                              "VSync-off Immediate mode requires actual DXGI tearing support; no mode fallback");
        CheckDebug("SetVSync");
        config_.vsync = enabled;
        info_.actual_present_mode = enabled ? PresentMode::Fifo : PresentMode::Immediate;
    });
}

std::vector<GpuFrameSample> D3D12Backend::PollGpuSamples() {
    return Operation("PollGpuSamples", [&] {
        for (auto& frame : frames_) CollectQuery(frame);
        CheckDebug("PollGpuSamples");
        std::vector<GpuFrameSample> result;
        result.reserve(ready_count_);
        for (std::size_t offset = 0; offset < ready_count_; ++offset)
            result.push_back(ready_samples_[(ready_head_ + offset) % ReadySampleCount]);
        std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
            return left.frame_id < right.frame_id;
        });
        ready_head_ = ready_count_ = 0;
        return result;
    });
}

void D3D12Backend::ClearNative() noexcept {
    meshes_.clear(); textures_.clear();
    ReleaseSizeResources();
    for (auto& frame : frames_) {
        frame.query_readback.Reset(); frame.commands.Reset(); frame.allocator.Reset();
        frame.query_pending = false;
    }
    pending_texture_.Reset(); pending_upload_.Reset();
    upload_commands_.Reset(); upload_allocator_.Reset();
    queries_.Reset(); pipelines_ = {};
    rtv_heap_.Reset(); dsv_heap_.Reset();
    latency_.Reset(); swapchain_.Reset();
    queue_.Reset(); fence_.Reset(); completion_.Reset();
    diagnostics_.Reset(); device_.Reset(); adapter_.Reset(); factory_.Reset();
    stopped_ = true;
}

bool D3D12Backend::MayReleaseNative() const noexcept {
    if (!has_native_work_ || !device_) return true;
    if (FAILED(device_->GetDeviceRemovedReason())) return true;
    if (unfenced_work_ || !fence_) return false;
    const auto completed = fence_->GetCompletedValue();
    return completed == std::numeric_limits<UINT64>::max() || completed >= last_submitted_;
}

void D3D12Backend::QuarantineNative() noexcept {
    auto& holder = *quarantine_;
    holder.factory.Swap(factory_); holder.adapter.Swap(adapter_); holder.device.Swap(device_);
    holder.diagnostics.Swap(diagnostics_); holder.queue.Swap(queue_); holder.swapchain.Swap(swapchain_);
    holder.fence.Swap(fence_); holder.rtv_heap.Swap(rtv_heap_); holder.dsv_heap.Swap(dsv_heap_);
    holder.queries.Swap(queries_);
    holder.pipelines.root_signature.Swap(pipelines_.root_signature);
    holder.pipelines.mesh.Swap(pipelines_.mesh);
    holder.pipelines.selection.Swap(pipelines_.selection);
    holder.backbuffers.swap(backbuffers_); holder.depth.swap(depth_); holder.color.swap(color_);
    holder.frames.swap(frames_); holder.meshes.swap(meshes_); holder.textures.swap(textures_);
    holder.upload_allocator.Swap(upload_allocator_); holder.upload_commands.Swap(upload_commands_);
    holder.pending_texture.Swap(pending_texture_); holder.pending_upload.Swap(pending_upload_);
    std::swap(holder.completion.value, completion_.value);
    std::swap(holder.latency.value, latency_.value);
    // Without completion/removal proof it is unsafe to run native destructors.
    // This terminal emergency owner is intentionally retained until process exit.
    // It is not successful cleanup, and no subsequent native work is permitted.
    static_cast<void>(quarantine_.release());
    quarantined_ = true;
    stopped_ = true;
    constexpr const char* diagnostic =
        "Symocraft D3D12 terminal cleanup: live GPU work lacks completion proof; "
        "all native ownership quarantined until process exit (not normal reclamation).\n";
    OutputDebugStringA(diagnostic);
    std::fputs(diagnostic, stderr);
}

void D3D12Backend::Shutdown() {
    if (quarantined_)
        throw RenderError(RenderErrorCategory::SubmissionFailure, "D3D12", "Shutdown",
                          "prior cleanup failed; terminal native ownership remains quarantined until process exit");
    if (stopped_) return;
    try {
        // A prior Execute or Present may have succeeded before its Signal failed.
        // Signal only already-queued work here; never submit another command list.
        if (queue_ && fence_ && initialized_) Signal("Signal(shutdown queued work)");
        if (fence_ && last_submitted_) WaitFor(last_submitted_, "Shutdown.Wait");
        if (diagnostics_) CheckDebug("Shutdown");
    } catch (...) {
        failed_ = true;
        if (MayReleaseNative()) ClearNative();
        else QuarantineNative();
        throw;
    }
    ClearNative();
}

void D3D12Backend::ShutdownNoexcept() noexcept {
    if (stopped_) return;
    try { Shutdown(); }
    catch (const RenderError& error) {
        OutputDebugStringA("Symocraft D3D12 bounded cleanup failure: ");
        OutputDebugStringA(error.what()); OutputDebugStringA(" ");
        OutputDebugStringA(error.NativeCode().c_str()); OutputDebugStringA(" ");
        OutputDebugStringA(error.Diagnostic().c_str()); OutputDebugStringA("\n");
    } catch (...) {
        OutputDebugStringA("Symocraft D3D12: unknown bounded cleanup failure.\n");
        if (!stopped_) {
            if (MayReleaseNative()) ClearNative();
            else QuarantineNative();
        }
    }
}

} // namespace

std::unique_ptr<RenderBackend> CreateD3D12Backend(Window& window, const RendererConfig& config) {
    return std::make_unique<D3D12Backend>(window, config);
}

} // namespace SymoCraft::Rendering::Detail
