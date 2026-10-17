#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

namespace SymoCraft::Rendering::Detail {

// Private diagnostic control. Ordinary gameplay never installs it.
struct D3D12DiagnosticAccess final {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> completion;
    UINT64 last_signal = 0;
    bool quarantined = false;
    bool native_cleared = false;
    std::weak_ptr<void> last_mesh, last_texture, frame_mesh, frame_texture;
    std::uint64_t mesh_buffers_created{}, mesh_buffers_released{}, textures_created{}, textures_released{};
    std::uint64_t live_resource_bytes{}, peak_resource_bytes{};
    unsigned wait_calls{}, query_readbacks{}, sample_drops{}, info_queue_checks{};
    unsigned validation_warnings{}, validation_errors{}, validation_corruptions{};
    struct SrvSlot {
        std::uint64_t heaps_created{}, views_written{};
        unsigned capacity{};
    };
    std::array<SrvSlot, 3> srv_slots{};
    unsigned last_srv_slot{};
    std::uint64_t last_srv_frame_id{};
    HRESULT next_mesh_allocation_result = S_OK;
    HRESULT next_frame_signal_result = S_OK;
    std::chrono::milliseconds wait_timeout{500};
    std::function<void(D3D12DiagnosticAccess&)> on_initialized;
    std::function<void(D3D12DiagnosticAccess&)> before_frame_signal;
};

// A same-thread, single-backend binding lets probes exercise the public facade
// and production application without changing the public API or game CLI.
class ScopedD3D12Diagnostics final {
public:
    explicit ScopedD3D12Diagnostics(std::shared_ptr<D3D12DiagnosticAccess> control);
    ~ScopedD3D12Diagnostics();
    ScopedD3D12Diagnostics(const ScopedD3D12Diagnostics&) = delete;
    ScopedD3D12Diagnostics& operator=(const ScopedD3D12Diagnostics&) = delete;
};

} // namespace SymoCraft::Rendering::Detail
