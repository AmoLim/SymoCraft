#include "renderer_probe.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace SymoCraft::Experimental::D3D12R1 {
namespace {
using Microsoft::WRL::ComPtr;
constexpr UINT FrameCount = 3;
constexpr DWORD WaitLimitMs = 2500;
constexpr DXGI_FORMAT BackbufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
constexpr DXGI_FORMAT RenderTargetFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
std::atomic<std::uint64_t> NextOwner{1};

template<class Writer> void CleanupLog(std::ostream& stream, Writer&& write) noexcept {
    try { write(stream); }
    catch (...) {}
}

struct EventHandle {
    HANDLE value{};
    ~EventHandle() noexcept { if (value) CloseHandle(value); }
};

std::string Hex(HRESULT value) {
    std::ostringstream result;
    result << "0x" << std::hex << std::uppercase << static_cast<std::uint32_t>(value);
    return result.str();
}
std::string Utf8(const wchar_t* value) {
    const int count = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (count <= 0) return "<unavailable>";
    std::string result(static_cast<std::size_t>(count), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), count, nullptr, nullptr);
    result.pop_back();
    return result;
}
std::vector<char> ReadShader(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("D3D12 shader asset missing: " + path.string());
    const auto size = input.tellg();
    if (size <= 0 || size > 64 * 1024 * 1024)
        throw std::runtime_error("D3D12 invalid shader asset size: " + path.string());
    std::vector<char> result(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(result.data(), static_cast<std::streamsize>(result.size())))
        throw std::runtime_error("D3D12 shader asset read failed: " + path.string());
    return result;
}
D3D12_HEAP_PROPERTIES Heap(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES result{};
    result.Type = type;
    result.CreationNodeMask = result.VisibleNodeMask = 1;
    return result;
}
D3D12_RESOURCE_DESC BufferDescription(UINT64 bytes) {
    D3D12_RESOURCE_DESC result{};
    result.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    result.Width = bytes;
    result.Height = 1;
    result.DepthOrArraySize = result.MipLevels = 1;
    result.SampleDesc.Count = 1;
    result.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    return result;
}
D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES before,
                                  D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER result{};
    result.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    result.Transition.pResource = resource;
    result.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    result.Transition.StateBefore = before;
    result.Transition.StateAfter = after;
    return result;
}
} // namespace

struct ProbeRenderer::Impl {
    enum class State { Ready, Suspended, Failed, Stopped };
    struct Frame {
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        UINT64 fence{};
    };
    struct Mesh {
        ComPtr<ID3D12Resource> resource;
        UINT vertices{}, bytes{}, generation{1};
        UINT64 lastUse{};
        bool live{};
    };
    struct Retired {
        ComPtr<ID3D12Resource> resource;
        UINT64 fence{};
        std::string reason;
    };

    std::ostream& log;
    HWND window{};
    Extent extent{};
    State state{State::Ready};
    DeviceInfo info;
    const UINT64 owner{NextOwner.fetch_add(1)};
    ComPtr<IDXGIFactory6> factory;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device> device;
    ComPtr<ID3D12InfoQueue> diagnostics;
    ComPtr<ID3D12CommandQueue> queue;
    ComPtr<IDXGISwapChain3> swapchain;
    ComPtr<ID3D12DescriptorHeap> rtvHeap, srvHeap;
    ComPtr<ID3D12RootSignature> rootSignature;
    ComPtr<ID3D12PipelineState> pipeline;
    ComPtr<ID3D12Fence> fence, gateFence;
    std::array<ComPtr<ID3D12Resource>, FrameCount> backbuffers;
    std::array<Frame, FrameCount> frames;
    ComPtr<ID3D12Resource> texture;
    std::vector<Mesh> meshes;
    std::vector<Retired> retired;
    UINT rtvStride{}, layerCount{};
    UINT64 nextFence{1}, lastSubmitted{}, frameSerial{}, gateValue{}, finalCompleted{};
    std::size_t collected{};
    UINT64 warningCount{}, errorCount{};
    EventHandle completionEvent;
    bool gateActive{}, injectTimeout{};

    Impl(HWND hwnd, Extent dimensions, const std::filesystem::path& shaderDir,
         std::ostream& output, bool requireDebug)
        : log(output), window(hwnd), extent(dimensions) {
        if (!requireDebug) throw std::invalid_argument("D3D12 R1 requires the Debug Layer; disabling it is rejected");
        if (!hwnd || !IsWindow(hwnd)) throw std::invalid_argument("D3D12 R1 requires a live native HWND");
        if (!extent.width || !extent.height) throw std::invalid_argument("D3D12 R1 initial extent must be nonzero");
        ComPtr<ID3D12Debug> debug;
        Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)), "D3D12GetDebugInterface (mandatory; install Graphics Tools if unavailable)");
        debug->EnableDebugLayer();
        info.debugLayerEnabled = true;
        Check(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&factory)), "CreateDXGIFactory2(debug)");
        SelectDevice();
        Check(device.As(&diagnostics), "ID3D12InfoQueue (mandatory)");
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        Check(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)), "CreateCommandQueue");
        Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "CreateFence");
        Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gateFence)), "CreateFence(gate)");
        completionEvent.value = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        if (!completionEvent.value) Check(HRESULT_FROM_WIN32(GetLastError()), "CreateEventW");
        D3D12_DESCRIPTOR_HEAP_DESC rtvDesc{};
        rtvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvDesc.NumDescriptors = FrameCount;
        Check(device->CreateDescriptorHeap(&rtvDesc, IID_PPV_ARGS(&rtvHeap)), "CreateDescriptorHeap(RTV)");
        rtvStride = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_DESCRIPTOR_HEAP_DESC srvDesc{};
        srvDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvDesc.NumDescriptors = 1;
        srvDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Check(device->CreateDescriptorHeap(&srvDesc, IID_PPV_ARGS(&srvHeap)), "CreateDescriptorHeap(SRV)");
        CreatePipeline(shaderDir);
        CreateSwapchain();
        for (auto& frame : frames) {
            Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame.allocator)), "CreateCommandAllocator(frame)");
            Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, frame.allocator.Get(), pipeline.Get(),
                                            IID_PPV_ARGS(&frame.list)), "CreateCommandList(frame)");
            Check(frame.list->Close(), "Close(initial frame list)");
        }
        info.renderTargetFormat = "R8G8B8A8_UNORM_SRGB (swapchain storage R8G8B8A8_UNORM)";
        log << "renderer=D3D12 window_mode=native debug_layer=enabled hardware_required=true "
               "minimum_feature_level=12_0 minimum_shader_model=6_0 frame_ring=3 bounded_wait_ms=" << WaitLimitMs << '\n';
        log << "fixture_clip_mapping=x/2-1,1-y/2 z=0.5 world_camera_validation=false "
               "vertex_stride=28 position=SINT3@0 uv_layer=FLOAT3@12 normal=FLOAT@24 "
               "sampler=point_clamp alpha_clip=0.01 clear_linear_rgba=0,0,0,1\n";
        CheckDebug("initialization");
    }

    void Check(HRESULT result, const char* operation) {
        if (SUCCEEDED(result)) return;
        state = State::Failed;
        const HRESULT removal = device ? device->GetDeviceRemovedReason() : S_OK;
        CleanupLog(log, [&](auto& output) {
            output << "error backend=D3D12 operation=" << operation << " hresult=" << Hex(result)
                   << " removed_reason=" << Hex(removal) << '\n';
        });
        throw std::runtime_error(std::string("D3D12 ") + operation + " failed: " + Hex(result));
    }

    void CheckDebug(const char* operation) {
        if (!diagnostics) return;
        bool failed = false;
        const UINT64 count = diagnostics->GetNumStoredMessagesAllowedByRetrievalFilter();
        for (UINT64 index = 0; index < count; ++index) {
            SIZE_T bytes{};
            Check(diagnostics->GetMessage(index, nullptr, &bytes), "GetMessage(size)");
            std::vector<std::byte> storage(bytes);
            auto* message = reinterpret_cast<D3D12_MESSAGE*>(storage.data());
            Check(diagnostics->GetMessage(index, message, &bytes), "GetMessage");
            const bool warning = message->Severity == D3D12_MESSAGE_SEVERITY_WARNING;
            const bool error = message->Severity == D3D12_MESSAGE_SEVERITY_ERROR ||
                               message->Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION;
            warningCount += warning;
            errorCount += error;
            failed |= warning || error;
            log << "debug_message operation=" << operation << " severity=" << message->Severity
                << " id=" << message->ID << " text=" << message->pDescription << '\n';
        }
        diagnostics->ClearStoredMessages();
        if (failed) {
            state = State::Failed;
            throw std::runtime_error(std::string("D3D12 Debug Layer warning/error during ") + operation);
        }
    }

    void RequireRunning() const {
        if (state == State::Failed || state == State::Stopped)
            throw std::runtime_error("D3D12 renderer is failed/stopped; new resource/submission operations rejected");
    }

    void SelectDevice() {
        for (UINT index = 0;; ++index) {
            ComPtr<IDXGIAdapter1> candidate;
            const HRESULT result = factory->EnumAdapterByGpuPreference(index, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
                                                                        IID_PPV_ARGS(&candidate));
            if (result == DXGI_ERROR_NOT_FOUND) break;
            Check(result, "EnumAdapterByGpuPreference");
            DXGI_ADAPTER_DESC1 desc{};
            Check(candidate->GetDesc1(&desc), "GetDesc1");
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                log << "adapter_rejected reason=software name=" << Utf8(desc.Description) << '\n';
                continue;
            }
            ComPtr<ID3D12Device> candidateDevice;
            const HRESULT creation = D3D12CreateDevice(candidate.Get(), D3D_FEATURE_LEVEL_12_0,
                                                      IID_PPV_ARGS(&candidateDevice));
            if (FAILED(creation)) {
                log << "adapter_rejected reason=feature_level_12_0 name=" << Utf8(desc.Description)
                    << " hresult=" << Hex(creation) << '\n';
                continue;
            }
            D3D12_FEATURE_DATA_SHADER_MODEL model{D3D_SHADER_MODEL_6_0};
            const HRESULT sm = candidateDevice->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &model, sizeof(model));
            if (FAILED(sm) || model.HighestShaderModel < D3D_SHADER_MODEL_6_0) {
                log << "adapter_rejected reason=shader_model_6_0 name=" << Utf8(desc.Description) << '\n';
                continue;
            }
            adapter = std::move(candidate);
            device = std::move(candidateDevice);
            const auto actualLuid = device->GetAdapterLuid();
            if (actualLuid.HighPart != desc.AdapterLuid.HighPart || actualLuid.LowPart != desc.AdapterLuid.LowPart)
                throw std::runtime_error("D3D12 device adapter identity differs from the DXGI adapter");
            info.name = Utf8(desc.Description);
            info.vendorId = desc.VendorId;
            info.deviceId = desc.DeviceId;
            info.softwareAdapter = false;
            info.featureLevel = "12_0 (requested and created)";
            info.shaderModel = "6_0 (explicit CheckFeatureSupport)";
            std::ostringstream luid;
            luid << std::hex << std::setfill('0') << std::setw(8) << static_cast<UINT>(actualLuid.HighPart)
                 << ':' << std::setw(8) << actualLuid.LowPart;
            info.luid = luid.str();
            LARGE_INTEGER driver{};
            Check(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &driver), "CheckInterfaceSupport(IDXGIDevice driver package version)");
            std::ostringstream version;
            version << HIWORD(driver.HighPart) << '.' << LOWORD(driver.HighPart) << '.'
                    << HIWORD(driver.LowPart) << '.' << LOWORD(driver.LowPart);
            info.driver = version.str();
            log << "adapter=" << info.name << " luid=" << info.luid << " driver=" << info.driver
                << " dedicated_video_bytes=" << desc.DedicatedVideoMemory << " shader_model=" << info.shaderModel << '\n';
            return;
        }
        throw std::runtime_error("D3D12 R1: no hardware adapter supports FL12_0 and SM6_0; WARP/OpenGL fallback forbidden");
    }

    void CreatePipeline(const std::filesystem::path& shaderDir) {
        const auto vertexShader = ReadShader(shaderDir / "probe-vs.cso");
        const auto pixelShader = ReadShader(shaderDir / "probe-ps.cso");
        D3D12_DESCRIPTOR_RANGE range{};
        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        range.NumDescriptors = 1;
        range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
        D3D12_ROOT_PARAMETER parameter{};
        parameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        parameter.DescriptorTable.NumDescriptorRanges = 1;
        parameter.DescriptorTable.pDescriptorRanges = &range;
        parameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_STATIC_SAMPLER_DESC sampler{};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = sampler.AddressV = sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        sampler.MaxAnisotropy = 1;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_ROOT_SIGNATURE_DESC root{};
        root.NumParameters = 1;
        root.pParameters = &parameter;
        root.NumStaticSamplers = 1;
        root.pStaticSamplers = &sampler;
        root.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
        ComPtr<ID3DBlob> serialized, errors;
        const HRESULT serialization = D3D12SerializeRootSignature(&root, D3D_ROOT_SIGNATURE_VERSION_1,
                                                                 &serialized, &errors);
        if (errors) log << "root_signature_diagnostic=" << static_cast<const char*>(errors->GetBufferPointer()) << '\n';
        Check(serialization, "D3D12SerializeRootSignature");
        Check(device->CreateRootSignature(0, serialized->GetBufferPointer(), serialized->GetBufferSize(),
                                         IID_PPV_ARGS(&rootSignature)), "CreateRootSignature");
        const D3D12_INPUT_ELEMENT_DESC input[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_SINT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };
        static_assert(offsetof(SymoCraft::BlockVertex3D, pos_coord) == 0);
        static_assert(offsetof(SymoCraft::BlockVertex3D, tex_coord) == 12);
        static_assert(offsetof(SymoCraft::BlockVertex3D, normal) == 24);
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        desc.pRootSignature = rootSignature.Get();
        desc.VS = {vertexShader.data(), vertexShader.size()};
        desc.PS = {pixelShader.data(), pixelShader.size()};
        desc.InputLayout = {input, static_cast<UINT>(std::size(input))};
        desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        desc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        desc.SampleMask = UINT_MAX;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        desc.RasterizerState.DepthClipEnable = TRUE;
        desc.DepthStencilState.DepthEnable = FALSE;
        desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
        desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.DepthStencilState.StencilEnable = FALSE;
        desc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        desc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        desc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        desc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        desc.DepthStencilState.BackFace = desc.DepthStencilState.FrontFace;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = RenderTargetFormat;
        desc.SampleDesc.Count = 1;
        Check(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)), "CreateGraphicsPipelineState(DXC SM6 assets)");
        log << "shader_vs=" << (shaderDir / "probe-vs.cso").string() << " shader_ps="
            << (shaderDir / "probe-ps.cso").string() << " shader_runtime_compilation=false\n";
    }

    void CreateSwapchain() {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        desc.Width = extent.width;
        desc.Height = extent.height;
        desc.Format = BackbufferFormat;
        desc.SampleDesc.Count = 1;
        desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        desc.BufferCount = FrameCount;
        desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        ComPtr<IDXGISwapChain1> created;
        Check(factory->CreateSwapChainForHwnd(queue.Get(), window, &desc, nullptr, nullptr, &created), "CreateSwapChainForHwnd(native)");
        Check(created.As(&swapchain), "IDXGISwapChain3");
        Check(factory->MakeWindowAssociation(window, DXGI_MWA_NO_ALT_ENTER), "MakeWindowAssociation");
        CreateBackbuffers();
    }

    void CreateBackbuffers() {
        auto handle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        D3D12_RENDER_TARGET_VIEW_DESC view{};
        view.Format = RenderTargetFormat;
        view.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        for (UINT index = 0; index < FrameCount; ++index) {
            Check(swapchain->GetBuffer(index, IID_PPV_ARGS(&backbuffers[index])), "GetBuffer");
            device->CreateRenderTargetView(backbuffers[index].Get(), &view, handle);
            handle.ptr += rtvStride;
        }
        log << "present_resources extent=" << extent.width << 'x' << extent.height
            << " buffers=3 rtv=R8G8B8A8_UNORM_SRGB storage=R8G8B8A8_UNORM\n";
        const auto actual = Presentation();
        if (actual.extent.width != extent.width || actual.extent.height != extent.height ||
            actual.format != BackbufferFormat || actual.bufferCount != FrameCount ||
            actual.sampleCount != 1 || actual.sampleQuality != 0 ||
            actual.swapEffect != DXGI_SWAP_EFFECT_FLIP_DISCARD || !actual.windowed)
            throw std::runtime_error("Actual DXGI presentation configuration differs from the R1 fixture request");
        log << "actual_swapchain extent=" << actual.extent.width << 'x' << actual.extent.height
            << " format=" << actual.format << " buffers=" << actual.bufferCount
            << " samples=" << actual.sampleCount << " quality=" << actual.sampleQuality
            << " swap_effect=" << actual.swapEffect << " windowed=" << actual.windowed
            << " flags=" << actual.flags << " present_sync_interval=0 present_flags=0\n";
    }

    PresentationInfo Presentation() const {
        DXGI_SWAP_CHAIN_DESC1 desc{};
        const auto description = swapchain->GetDesc1(&desc);
        if (FAILED(description)) throw std::runtime_error("GetDesc1(actual swapchain) failed: " + Hex(description));
        BOOL fullscreen = FALSE;
        const auto mode = swapchain->GetFullscreenState(&fullscreen, nullptr);
        if (FAILED(mode)) throw std::runtime_error("GetFullscreenState failed: " + Hex(mode));
        return {{desc.Width, desc.Height}, static_cast<std::uint32_t>(desc.Format), desc.BufferCount,
                desc.SampleDesc.Count, desc.SampleDesc.Quality, static_cast<std::uint32_t>(desc.SwapEffect),
                desc.Flags, fullscreen == FALSE};
    }

    UINT64 Completed() const {
        return fence ? fence->GetCompletedValue() : finalCompleted;
    }

    void Wait(UINT64 target, const char* operation) {
        if (!target && !injectTimeout) return;
        const UINT64 before = Completed();
        if (before == UINT64_MAX) {
            state = State::Failed;
            Check(device->GetDeviceRemovedReason(), "fence reported device removal");
            throw std::runtime_error("D3D12 completed fence invalid after device removal");
        }
        if (before >= target && !injectTimeout) return;
        if (gateActive) throw std::logic_error("D3D12 finite test gate must be released before any CPU fence wait");
        ResetEvent(completionEvent.value);
        const DWORD limit = injectTimeout ? 200 : WaitLimitMs;
        if (!injectTimeout) Check(fence->SetEventOnCompletion(target, completionEvent.value), "SetEventOnCompletion");
        log << "fence_wait operation=" << operation << " target=" << target << " completed_before=" << before
            << " limit_ms=" << limit << " injected=" << injectTimeout << '\n';
        const DWORD result = WaitForSingleObject(completionEvent.value, limit);
        if (result != WAIT_OBJECT_0) {
            state = State::Failed;
            CleanupLog(log, [&](auto& output) {
                output << "wait_failure operation=" << operation << " result=" << result
                       << " target=" << target << " completed=" << Completed()
                       << " injected=" << injectTimeout << " no_second_wait=true\n";
            });
            throw std::runtime_error(injectTimeout ? "D3D12 injected GPU wait timeout (simulated fence event)" :
                                                   "D3D12 bounded fence wait failed/timeout");
        }
    }

    UINT64 Submit(ID3D12GraphicsCommandList* list, bool present = false) {
        Check(list->Close(), "Close(command list)");
        ID3D12CommandList* lists[] = {list};
        queue->ExecuteCommandLists(1, lists);
        // Resize/teardown fences must cover queued presentation as well as draws.
        if (present) Check(swapchain->Present(0, 0), "Present(vsync off)");
        const UINT64 submitted = nextFence++;
        Check(queue->Signal(fence.Get(), submitted), "Signal(frame fence)");
        lastSubmitted = submitted;
        return submitted;
    }

    void Collect() {
        const UINT64 completed = Completed();
        if (completed == UINT64_MAX) {
            Check(device->GetDeviceRemovedReason(), "resource retirement fence device removed");
            throw std::runtime_error("D3D12 invalid completed fence after device removal");
        }
        for (auto item = retired.begin(); item != retired.end();) {
            if (item->fence > completed) { ++item; continue; }
            log << "retire_collect reason=" << item->reason << " retire_fence=" << item->fence
                << " completed_fence=" << completed << '\n';
            item = retired.erase(item);
            ++collected;
        }
    }

    void Retire(const ComPtr<ID3D12Resource>& resource, UINT64 lastUse, std::string reason) {
        if (!resource) return;
        retired.push_back({resource, lastUse, reason});
        CleanupLog(log, [&](auto& output) {
            output << "retire_enqueue reason=" << reason << " last_use_fence=" << lastUse
                   << " completed_fence=" << Completed() << " in_flight=" << (lastUse > Completed()) << '\n';
        });
        Collect();
    }

    Mesh& Resolve(MeshHandle handle) {
        if (handle.owner != owner || handle.slot >= meshes.size() ||
            !meshes[handle.slot].live || meshes[handle.slot].generation != handle.generation) {
            log << "mesh_handle_rejected owner=" << handle.owner << " slot=" << handle.slot
                << " generation=" << handle.generation << " reason=foreign_or_stale\n";
            throw std::invalid_argument("D3D12 stale/foreign mesh handle rejected");
        }
        return meshes[handle.slot];
    }

    ComPtr<ID3D12Resource> SnapshotMesh(const SymoCraft::MeshData& input, UINT& vertices, UINT& bytes) {
        if (input.vertices.size() % 3 != 0)
            throw std::invalid_argument("D3D12 mesh non-indexed triangle vertex count must be a multiple of 3");
        if (input.vertices.size() > UINT_MAX / sizeof(SymoCraft::BlockVertex3D))
            throw std::invalid_argument("D3D12 fixture mesh exceeds UINT vertex-buffer view size");
        for (const auto& vertex : input.vertices) {
            const auto& uv = vertex.tex_coord;
            if (!std::isfinite(uv.x) || !std::isfinite(uv.y) || !std::isfinite(uv.z) ||
                !std::isfinite(vertex.normal) || uv.z < 0 || std::floor(uv.z) != uv.z || uv.z >= layerCount)
                throw std::invalid_argument("D3D12 mesh UV/layer/normal invalid or texture layer out of range");
        }
        vertices = static_cast<UINT>(input.vertices.size());
        bytes = vertices * sizeof(SymoCraft::BlockVertex3D);
        ComPtr<ID3D12Resource> resource;
        if (bytes) {
            const auto desc = BufferDescription(bytes);
            const auto heap = Heap(D3D12_HEAP_TYPE_UPLOAD);
            const HRESULT allocation = device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource));
            if (allocation == E_OUTOFMEMORY) {
                CleanupLog(log, [](auto& output) {
                    output << "mesh_allocation_rejected hresult=0x8007000E old_content_preserved=true renderer_state_unchanged=true\n";
                });
                throw std::bad_alloc();
            }
            Check(allocation, "CreateCommittedResource(mesh fresh upload)");
            void* mapped{};
            const D3D12_RANGE noRead{0, 0};
            Check(resource->Map(0, &noRead, &mapped), "Map(mesh upload)");
            std::memcpy(mapped, input.vertices.data(), bytes);
            const D3D12_RANGE written{0, bytes};
            resource->Unmap(0, &written);
        }
        log << "mesh_snapshot vertices=" << vertices << " upload_bytes=" << bytes
            << " cpu_borrow_retained=false fresh_resource=" << (bytes != 0)
            << " memory=UPLOAD_heap_vertex_buffer\n";
        return resource;
    }

    void ReleaseAll() noexcept {
        if (gateActive && gateFence) {
            const HRESULT result = gateFence->Signal(gateValue);
            CleanupLog(log, [&](auto& output) { output << "gate_cleanup_release hresult=" << Hex(result) << '\n'; });
            gateActive = false;
        }
        if (fence) finalCompleted = fence->GetCompletedValue();
        CleanupLog(log, [&](auto& output) {
            output << "release_native_resources retired_pending=" << retired.size()
                   << " last_submitted=" << lastSubmitted << " last_completed=" << finalCompleted
                   << " abandoned=" << (state == State::Failed)
                   << " retirement_collection_claimed=false\n";
        });
        for (auto& frame : frames) { frame.list.Reset(); frame.allocator.Reset(); }
        CleanupLog(log, [](auto& output) { output << "release_stage=command_lists_allocators_done\n"; });
        retired.clear();
        meshes.clear();
        texture.Reset();
        CleanupLog(log, [](auto& output) { output << "release_stage=mesh_texture_done\n"; });
        for (auto& buffer : backbuffers) buffer.Reset();
        pipeline.Reset();
        rootSignature.Reset();
        srvHeap.Reset();
        rtvHeap.Reset();
        swapchain.Reset();
        CleanupLog(log, [](auto& output) { output << "release_stage=swapchain_done\n"; });
        queue.Reset();
        CleanupLog(log, [](auto& output) { output << "release_stage=queue_done\n"; });
        gateFence.Reset();
        fence.Reset();
    }
};

ProbeRenderer::ProbeRenderer(HWND window, Extent extent, const std::filesystem::path& shaderDir,
                             std::ostream& log, bool requireDebug)
    : impl_(std::make_unique<Impl>(window, extent, shaderDir, log, requireDebug)) {}

ProbeRenderer::~ProbeRenderer() noexcept {
    if (!impl_) return;
    try { Shutdown(); }
    catch (const std::exception& error) {
        CleanupLog(impl_->log, [&](auto& output) { output << "destructor_cleanup_error=" << error.what() << '\n'; });
    }
    catch (...) { impl_->ReleaseAll(); }
}

void ProbeRenderer::CreateTexture(std::span<const std::uint8_t> pixels, std::uint32_t width,
                                  std::uint32_t height, std::uint32_t layers, bool srgb) {
    auto& self = *impl_;
    self.RequireRunning();
    if (self.texture) throw std::invalid_argument("D3D12 R1 texture creation is initialization-only; replacement is not implemented");
    if (!width || !height || !layers || width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION ||
        height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION || layers > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
        throw std::invalid_argument("D3D12 texture dimensions/layers invalid");
    const UINT64 expected = UINT64(width) * height * layers * 4;
    if (pixels.size() != expected) throw std::invalid_argument("D3D12 texture RGBA8 byte count mismatch");
    self.Wait(self.lastSubmitted, "texture initialization precondition");
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = static_cast<UINT16>(layers);
    desc.MipLevels = 1;
    desc.Format = srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    const auto defaultHeap = Heap(D3D12_HEAP_TYPE_DEFAULT);
    ComPtr<ID3D12Resource> created;
    self.Check(self.device->CreateCommittedResource(&defaultHeap, D3D12_HEAP_FLAG_NONE, &desc,
                                                    D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                    IID_PPV_ARGS(&created)), "CreateCommittedResource(texture)");
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(layers);
    std::vector<UINT> rows(layers);
    std::vector<UINT64> rowBytes(layers);
    UINT64 uploadBytes{};
    self.device->GetCopyableFootprints(&desc, 0, layers, 0, footprints.data(), rows.data(), rowBytes.data(), &uploadBytes);
    const auto uploadDesc = BufferDescription(uploadBytes);
    const auto uploadHeap = Heap(D3D12_HEAP_TYPE_UPLOAD);
    ComPtr<ID3D12Resource> upload;
    self.Check(self.device->CreateCommittedResource(&uploadHeap, D3D12_HEAP_FLAG_NONE, &uploadDesc,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                                    IID_PPV_ARGS(&upload)), "CreateCommittedResource(texture upload)");
    void* mapped{};
    const D3D12_RANGE noRead{0, 0};
    self.Check(upload->Map(0, &noRead, &mapped), "Map(texture upload)");
    for (UINT layer = 0; layer < layers; ++layer) {
        for (UINT row = 0; row < height; ++row) {
            auto* target = static_cast<std::uint8_t*>(mapped) + footprints[layer].Offset + UINT64(row) * footprints[layer].Footprint.RowPitch;
            const auto* source = pixels.data() + (UINT64(layer) * height + row) * width * 4;
            std::memcpy(target, source, static_cast<std::size_t>(width) * 4);
        }
    }
    const D3D12_RANGE written{0, static_cast<SIZE_T>(uploadBytes)};
    upload->Unmap(0, &written);
    ComPtr<ID3D12CommandAllocator> allocator;
    ComPtr<ID3D12GraphicsCommandList> list;
    self.Check(self.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)), "CreateCommandAllocator(texture)");
    self.Check(self.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr,
                                             IID_PPV_ARGS(&list)), "CreateCommandList(texture)");
    for (UINT layer = 0; layer < layers; ++layer) {
        D3D12_TEXTURE_COPY_LOCATION target{};
        target.pResource = created.Get();
        target.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        target.SubresourceIndex = layer;
        D3D12_TEXTURE_COPY_LOCATION source{};
        source.pResource = upload.Get();
        source.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        source.PlacedFootprint = footprints[layer];
        list->CopyTextureRegion(&target, 0, 0, 0, &source, nullptr);
    }
    const auto transition = Transition(created.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    list->ResourceBarrier(1, &transition);
    const UINT64 submitted = self.Submit(list.Get());
    self.Wait(submitted, "texture upload (one-shot initialization)");
    D3D12_SHADER_RESOURCE_VIEW_DESC view{};
    view.Format = desc.Format;
    view.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
    view.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    view.Texture2DArray.MipLevels = 1;
    view.Texture2DArray.ArraySize = layers;
    self.device->CreateShaderResourceView(created.Get(), &view, self.srvHeap->GetCPUDescriptorHandleForHeapStart());
    self.texture = std::move(created);
    self.layerCount = layers;
    self.info.textureFormat = srgb ? "R8G8B8A8_UNORM_SRGB" : "R8G8B8A8_UNORM";
    self.log << "texture_snapshot extent=" << width << 'x' << height << " layers=" << layers
             << " cpu_storage=layer_major_top_left_RGBA8 tightly_packed cpu_bytes=" << expected
             << " gpu_upload_bytes=" << uploadBytes << " row_pitch=" << footprints.front().Footprint.RowPitch
             << " cpu_borrow_retained=false format=" << self.info.textureFormat
             << " submitted_fence=" << submitted << " completed_fence=" << self.Completed() << '\n';
    self.CheckDebug("CreateTexture");
}

MeshHandle ProbeRenderer::UploadMesh(const SymoCraft::MeshData& mesh) {
    auto& self = *impl_;
    self.RequireRunning();
    if (!self.texture) throw std::invalid_argument("D3D12 mesh fixture requires an accepted texture first");
    UINT vertices{}, bytes{};
    auto resource = self.SnapshotMesh(mesh, vertices, bytes);
    UINT slot{};
    for (; slot < self.meshes.size(); ++slot) if (!self.meshes[slot].live) break;
    if (slot == self.meshes.size()) self.meshes.emplace_back();
    auto& stored = self.meshes[slot];
    stored.resource = std::move(resource);
    stored.vertices = vertices;
    stored.bytes = bytes;
    stored.live = true;
    stored.lastUse = 0;
    self.log << "mesh_create owner=" << self.owner << " slot=" << slot << " generation=" << stored.generation << '\n';
    self.CheckDebug("UploadMesh");
    return {self.owner, slot, stored.generation};
}

void ProbeRenderer::UpdateMesh(MeshHandle handle, const SymoCraft::MeshData& mesh) {
    auto& self = *impl_;
    self.RequireRunning();
    auto& current = self.Resolve(handle);
    UINT vertices{}, bytes{};
    auto fresh = self.SnapshotMesh(mesh, vertices, bytes);
    self.Retire(current.resource, current.lastUse, "mesh_update");
    current.resource = std::move(fresh);
    current.vertices = vertices;
    current.bytes = bytes;
    current.lastUse = 0;
    self.log << "mesh_update slot=" << handle.slot << " generation=" << handle.generation
             << " vertices=" << vertices << " accepted=true\n";
    self.CheckDebug("UpdateMesh");
}

void ProbeRenderer::DestroyMesh(MeshHandle handle) {
    auto& self = *impl_;
    self.RequireRunning();
    auto& current = self.Resolve(handle);
    self.Retire(current.resource, current.lastUse, "mesh_destroy");
    current.resource.Reset();
    current.live = false;
    current.vertices = current.bytes = 0;
    ++current.generation;
    if (!current.generation) ++current.generation;
    self.log << "mesh_destroy slot=" << handle.slot << " old_generation=" << handle.generation
             << " new_generation=" << current.generation << " logical_invalidated=true\n";
    self.CheckDebug("DestroyMesh");
}

FrameResult ProbeRenderer::Render(MeshHandle handle, bool capture, bool present) {
    auto& self = *impl_;
    self.RequireRunning();
    auto& mesh = self.Resolve(handle);
    if (self.state == Impl::State::Suspended) {
        self.log << "render_skipped reason=zero_extent submitted_frames=" << self.frameSerial << '\n';
        return {false, true, self.lastSubmitted, self.Completed(), std::nullopt};
    }
    if (self.gateActive && (capture || present))
        throw std::invalid_argument("D3D12 gated Render requires capture=false,present=false to avoid blocking Present");
    auto& frame = self.frames[self.frameSerial % FrameCount];
    self.Wait(frame.fence, "frame ring allocator reuse");
    self.Collect();
    self.Check(frame.allocator->Reset(), "Reset(frame allocator)");
    self.Check(frame.list->Reset(frame.allocator.Get(), self.pipeline.Get()), "Reset(frame list)");
    auto* list = frame.list.Get();
    const UINT bufferIndex = self.swapchain->GetCurrentBackBufferIndex();
    auto* buffer = self.backbuffers[bufferIndex].Get();
    auto rtv = self.rtvHeap->GetCPUDescriptorHandleForHeapStart();
    rtv.ptr += bufferIndex * self.rtvStride;
    const auto toRender = Transition(buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    list->ResourceBarrier(1, &toRender);
    constexpr float clear[]{0, 0, 0, 1};
    list->ClearRenderTargetView(rtv, clear, 0, nullptr);
    list->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    const D3D12_VIEWPORT viewport{0, 0, static_cast<float>(self.extent.width), static_cast<float>(self.extent.height), 0, 1};
    const D3D12_RECT scissor{0, 0, static_cast<LONG>(self.extent.width), static_cast<LONG>(self.extent.height)};
    list->RSSetViewports(1, &viewport);
    list->RSSetScissorRects(1, &scissor);
    list->SetGraphicsRootSignature(self.rootSignature.Get());
    ID3D12DescriptorHeap* heaps[]{self.srvHeap.Get()};
    list->SetDescriptorHeaps(1, heaps);
    list->SetGraphicsRootDescriptorTable(0, self.srvHeap->GetGPUDescriptorHandleForHeapStart());
    list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    if (mesh.vertices) {
        const D3D12_VERTEX_BUFFER_VIEW vertexBuffer{mesh.resource->GetGPUVirtualAddress(), mesh.bytes, sizeof(SymoCraft::BlockVertex3D)};
        list->IASetVertexBuffers(0, 1, &vertexBuffer);
        list->DrawInstanced(mesh.vertices, 1, 0, 0);
    }
    ComPtr<ID3D12Resource> readback;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};
    UINT64 captureBytes{};
    if (capture) {
        const auto desc = buffer->GetDesc();
        self.device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, nullptr, nullptr, &captureBytes);
        const auto heap = Heap(D3D12_HEAP_TYPE_READBACK);
        const auto readbackDesc = BufferDescription(captureBytes);
        self.Check(self.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
                                                        D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                        IID_PPV_ARGS(&readback)), "CreateCommittedResource(screenshot readback)");
        const auto toCopy = Transition(buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        list->ResourceBarrier(1, &toCopy);
        D3D12_TEXTURE_COPY_LOCATION source{};
        source.pResource = buffer;
        source.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION target{};
        target.pResource = readback.Get();
        target.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        target.PlacedFootprint = footprint;
        list->CopyTextureRegion(&target, 0, 0, 0, &source, nullptr);
    }
    const auto toPresent = Transition(buffer, capture ? D3D12_RESOURCE_STATE_COPY_SOURCE : D3D12_RESOURCE_STATE_RENDER_TARGET,
                                     D3D12_RESOURCE_STATE_PRESENT);
    list->ResourceBarrier(1, &toPresent);
    const UINT64 submitted = self.Submit(list, present);
    frame.fence = submitted;
    mesh.lastUse = submitted;
    ++self.frameSerial;
    self.log << "frame_submit frame_id=" << self.frameSerial << " submitted_fence=" << submitted
             << " completed_fence=" << self.Completed() << " mesh_slot=" << handle.slot
             << " vertices=" << mesh.vertices << " capture=" << capture << " present=" << present
             << " gate_active=" << self.gateActive << " upload_bytes=0\n";
    FrameResult result{present, false, submitted, self.Completed(), std::nullopt};
    if (capture) {
        self.Wait(submitted, "explicit screenshot outside performance sample");
        void* mapped{};
        const D3D12_RANGE readRange{static_cast<SIZE_T>(footprint.Offset), static_cast<SIZE_T>(captureBytes)};
        self.Check(readback->Map(0, &readRange, &mapped), "Map(screenshot readback)");
        Image image;
        image.extent = self.extent;
        image.rgba.resize(static_cast<std::size_t>(self.extent.width) * self.extent.height * 4);
        for (UINT row = 0; row < self.extent.height; ++row) {
            const auto* source = static_cast<const std::uint8_t*>(mapped) + footprint.Offset + UINT64(row) * footprint.Footprint.RowPitch;
            auto* target = image.rgba.data() + static_cast<std::size_t>(row) * self.extent.width * 4;
            std::memcpy(target, source, static_cast<std::size_t>(self.extent.width) * 4);
        }
        const D3D12_RANGE noWrite{0, 0};
        readback->Unmap(0, &noWrite);
        result.screenshot = std::move(image);
        result.completedFence = self.Completed();
        self.log << "screenshot_owned top_left=true rgba_bytes=" << result.screenshot->rgba.size()
                 << " readback_row_pitch=" << footprint.Footprint.RowPitch << '\n';
    }
    self.CheckDebug("Render");
    return result;
}

void ProbeRenderer::Resize(Extent extent) {
    auto& self = *impl_;
    self.RequireRunning();
    if (!extent.width || !extent.height) {
        self.extent = extent;
        self.state = Impl::State::Suspended;
        self.log << "resize_suspend extent=" << extent.width << 'x' << extent.height
                 << " no_gpu_submission=true\n";
        return;
    }
    if (self.state == Impl::State::Ready && self.extent.width == extent.width && self.extent.height == extent.height) return;
    self.Wait(self.lastSubmitted, "resize prior submissions");
    self.Collect();
    for (auto& frame : self.frames) {
        self.Check(frame.allocator->Reset(), "Reset(resize allocator)");
        self.Check(frame.list->Reset(frame.allocator.Get(), self.pipeline.Get()), "Reset(resize discard list)");
        self.Check(frame.list->Close(), "Close(resize discarded list)");
        frame.fence = 0;
    }
    self.log << "resize_discarded_prior_command_lists completed_fence=" << self.Completed() << '\n';
    for (auto& buffer : self.backbuffers) buffer.Reset();
    self.log << "resize_old_backbuffers_released=true\n";
    self.Check(self.swapchain->ResizeBuffers(FrameCount, extent.width, extent.height, BackbufferFormat, 0), "ResizeBuffers");
    self.extent = extent;
    self.CreateBackbuffers();
    self.state = Impl::State::Ready;
    self.log << "resize_ready extent=" << extent.width << 'x' << extent.height
             << " completed_fence=" << self.Completed() << '\n';
    self.CheckDebug("Resize");
}

void ProbeRenderer::BeginGpuGate() {
    auto& self = *impl_;
    self.RequireRunning();
    if (self.gateActive) throw std::logic_error("D3D12 GPU test gate already active");
    self.Wait(self.lastSubmitted, "finite gate precondition drain");
    for (auto& frame : self.frames) frame.fence = 0;
    ++self.gateValue;
    self.Check(self.queue->Wait(self.gateFence.Get(), self.gateValue), "queue Wait(finite test gate)");
    self.gateActive = true;
    self.log << "gate_begin value=" << self.gateValue << " completed_fence=" << self.Completed()
             << " cpu_signal_release_required=true\n";
}

void ProbeRenderer::EndGpuGate() {
    auto& self = *impl_;
    self.RequireRunning();
    if (!self.gateActive) throw std::logic_error("D3D12 GPU test gate is not active");
    self.Check(self.gateFence->Signal(self.gateValue), "CPU Signal(finite test gate release)");
    self.gateActive = false;
    self.log << "gate_end value=" << self.gateValue << " last_submitted=" << self.lastSubmitted
             << " completed_fence=" << self.Completed() << '\n';
    self.CheckDebug("EndGpuGate");
}

void ProbeRenderer::SetInjectTimeout(bool enabled) {
    impl_->RequireRunning();
    impl_->injectTimeout = enabled;
    impl_->log << "wait_timeout_injection=" << enabled << " simulated_fence_event_only=true\n";
}

RendererStats ProbeRenderer::Stats() {
    auto& self = *impl_;
    if (self.state != Impl::State::Stopped && self.state != Impl::State::Failed) self.Collect();
    return {self.frameSerial, self.lastSubmitted, self.Completed(), self.retired.size(), self.collected,
            self.warningCount, self.errorCount};
}

const DeviceInfo& ProbeRenderer::Info() const { return impl_->info; }
PresentationInfo ProbeRenderer::Presentation() const {
    impl_->RequireRunning();
    return impl_->Presentation();
}

void ProbeRenderer::Shutdown() {
    auto& self = *impl_;
    if (self.state == Impl::State::Stopped) {
        CleanupLog(self.log, [](auto& output) { output << "shutdown_idempotent no_wait=true\n"; });
        return;
    }
    const bool abandoned = self.state == Impl::State::Failed;
    try {
        if (self.gateActive) {
            self.Check(self.gateFence->Signal(self.gateValue), "shutdown finite gate release");
            self.gateActive = false;
        }
        if (self.state != Impl::State::Failed) {
            self.Wait(self.lastSubmitted, "Shutdown");
            self.Collect();
            self.CheckDebug("Shutdown");
        } else {
            CleanupLog(self.log, [](auto& output) { output << "shutdown_failed_state no_second_wait=true\n"; });
            try { self.CheckDebug("failed Shutdown diagnostics"); }
            catch (...) {}
        }
    } catch (...) {
        self.ReleaseAll();
        self.state = Impl::State::Stopped;
        CleanupLog(self.log, [](auto& output) { output << "shutdown_error_preserved cleanup_done=true no_second_wait=true\n"; });
        throw;
    }
    self.ReleaseAll();
    if (!abandoned) {
        try { self.CheckDebug("post-resource Shutdown"); }
        catch (...) {
            self.state = Impl::State::Stopped;
            CleanupLog(self.log, [](auto& output) {
                output << "shutdown_post_release_diagnostic_failure cleanup_done=true no_second_wait=true\n";
            });
            throw;
        }
    } else {
        try { self.CheckDebug("post-resource failed Shutdown diagnostics"); }
        catch (...) {}
    }
    self.state = Impl::State::Stopped;
    CleanupLog(self.log, [&](auto& output) {
        output << (abandoned ? "shutdown_abandoned" : "shutdown_complete")
               << " last_submitted=" << self.lastSubmitted << " last_completed=" << self.finalCompleted
               << " debug_warnings=" << self.warningCount << " debug_errors=" << self.errorCount
               << " retired_collected=" << self.collected
               << " fence_completion_claimed=" << !abandoned << '\n';
    });
}

} // namespace SymoCraft::Experimental::D3D12R1
