#pragma once

#include <d3d12.h>
#include <dxgiformat.h>
#include <wrl/client.h>

#include <cstdint>
#include <filesystem>

namespace SymoCraft::Rendering::Detail::D3D12 {

struct PipelineSet {
    Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> mesh;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> selection;
};

// Root slot 0 is a column-major MVP (16 constants, b0); slot 1 is t0.
// The shader root contains the four build-produced *.vs/ps.dxil files.
PipelineSet CreatePipelines(ID3D12Device* device,
                            const std::filesystem::path& shader_root,
                            DXGI_FORMAT color_format, DXGI_FORMAT depth_format,
                            std::uint32_t sample_count);

} // namespace SymoCraft::Rendering::Detail::D3D12
