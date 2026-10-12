#pragma once

#include <d3d12.h>
#include <wrl/client.h>

namespace SymoCraft::Rendering::Detail {

// Private, explicit diagnostic factory only. The ordinary game never creates
// this control; probes own its references through backend shutdown.
struct D3D12DiagnosticAccess final {
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12Fence> completion;
    UINT64 last_signal = 0;
    bool quarantined = false;
    HRESULT next_mesh_allocation_result = S_OK;
    HRESULT next_frame_signal_result = S_OK;
};

} // namespace SymoCraft::Rendering::Detail
