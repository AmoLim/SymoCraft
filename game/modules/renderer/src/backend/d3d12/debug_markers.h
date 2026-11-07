#pragma once

// RenderDoc's documented native D3D12 ANSI marker encoding; no capture SDK/runtime.
#if defined(SYMOCRAFT_GPU_MARKERS_ENABLED)
#include <d3d12.h>
#include <cstdio>
#include <cstring>
#include <string>

namespace SymoCraft::Rendering::Detail::D3D12 {
    inline void DebugName(ID3D12Object* object, const std::wstring& name) noexcept {
        if (object && FAILED(object->SetName(name.c_str())))
            std::fputs("[renderer] D3D12 resource annotation unavailable\n", stderr);
    }
    class DebugRegion final {
        ID3D12GraphicsCommandList* commands_;
    public:
        DebugRegion(ID3D12GraphicsCommandList* commands, const char* name, bool active = true) noexcept : commands_(active ? commands : nullptr) {
            if (commands_) commands_->BeginEvent(1, name, static_cast<UINT>(std::strlen(name) + 1));
        }
        ~DebugRegion() { if (commands_) commands_->EndEvent(); }
        DebugRegion(const DebugRegion&) = delete;
        DebugRegion& operator=(const DebugRegion&) = delete;
    };
}
#define SYMOCRAFT_D3D12_NAME(object, name) ::SymoCraft::Rendering::Detail::D3D12::DebugName(object, name)
#define SYMOCRAFT_D3D12_REGION(commands, name) ::SymoCraft::Rendering::Detail::D3D12::DebugRegion symocraft_gpu_region(commands, name)
#define SYMOCRAFT_D3D12_REGION_IF(commands, name, active) ::SymoCraft::Rendering::Detail::D3D12::DebugRegion symocraft_gpu_region(commands, name, active)
#else
#define SYMOCRAFT_D3D12_NAME(object, name) ((void)0)
#define SYMOCRAFT_D3D12_REGION(commands, name) ((void)0)
#define SYMOCRAFT_D3D12_REGION_IF(commands, name, active) ((void)0)
#endif
