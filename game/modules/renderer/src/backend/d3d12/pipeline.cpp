#include "pipeline.h"

#include <symocraft/renderer/error.h>
#include <symocraft/scene/mesh.h>

#include <dxgi.h>

#include <array>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace SymoCraft::Rendering::Detail::D3D12 {
namespace {

using Microsoft::WRL::ComPtr;

std::string PathText(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    return {utf8.begin(), utf8.end()};
}

std::string NativeCode(HRESULT result) {
    std::array<char, 11> text{};
    std::snprintf(text.data(), text.size(), "0x%08lX",
                  static_cast<unsigned long>(result));
    return text.data();
}

std::string BlobText(ID3DBlob* blob) {
    if (!blob || blob->GetBufferSize() == 0) return {};
    std::string text(static_cast<const char*>(blob->GetBufferPointer()),
                     blob->GetBufferSize());
    while (!text.empty() && text.back() == '\0') text.pop_back();
    return text;
}

void CheckNative(HRESULT result, ID3D12Device* device, const char* operation,
                 std::string diagnostic) {
    if (SUCCEEDED(result)) return;
    auto category = RenderErrorCategory::InitializationFailure;
    if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET ||
        result == DXGI_ERROR_DEVICE_HUNG) {
        category = RenderErrorCategory::DeviceLost;
    }
    if (device) {
        const auto removal = device->GetDeviceRemovedReason();
        if (FAILED(removal)) {
            category = RenderErrorCategory::DeviceLost;
            diagnostic += "\nDevice removal reason: " + NativeCode(removal);
        }
    }
    throw RenderError(category, "D3D12", "CreatePipelines",
                      std::string(operation) + " failed", NativeCode(result),
                      std::move(diagnostic));
}

std::vector<std::uint8_t> ReadShader(const std::filesystem::path& path) {
    const auto location = PathText(path);
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) {
        throw RenderError(RenderErrorCategory::AssetUnavailable, "D3D12",
                          "CreatePipelines", "Cannot open compiled shader: " + location,
                          {}, "Expected build-produced DXIL; no runtime shader compiler is used.");
    }
    const auto end = stream.tellg();
    if (end <= std::streampos{0}) {
        throw RenderError(RenderErrorCategory::AssetUnavailable, "D3D12",
                          "CreatePipelines", "Compiled shader is empty or unreadable: " + location);
    }
    const auto length = static_cast<std::uintmax_t>(static_cast<std::streamoff>(end));
    if (length > std::numeric_limits<std::size_t>::max() ||
        length > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        throw RenderError(RenderErrorCategory::AssetUnavailable, "D3D12",
                          "CreatePipelines", "Compiled shader size cannot be represented: " + location);
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(length));
    stream.seekg(0, std::ios::beg);
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(length));
    if (!stream) {
        throw RenderError(RenderErrorCategory::AssetUnavailable, "D3D12",
                          "CreatePipelines", "Cannot read complete compiled shader: " + location);
    }
    return bytes;
}

ComPtr<ID3D12RootSignature> CreateRootSignature(ID3D12Device* device) {
    D3D12_DESCRIPTOR_RANGE texture{};
    texture.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    texture.NumDescriptors = 1;
    texture.BaseShaderRegister = 0;
    texture.RegisterSpace = 0;
    texture.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    std::array<D3D12_ROOT_PARAMETER, 2> parameters{};
    parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    parameters[0].Constants = {0, 0, 16};
    parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
    parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    parameters[1].DescriptorTable = {1, &texture};
    parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.MaxAnisotropy = 1;
    sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = 0.0f;
    sampler.ShaderRegister = 0;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC descriptor{};
    descriptor.NumParameters = static_cast<UINT>(parameters.size());
    descriptor.pParameters = parameters.data();
    descriptor.NumStaticSamplers = 1;
    descriptor.pStaticSamplers = &sampler;
    descriptor.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
                       D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    ComPtr<ID3DBlob> serialized;
    ComPtr<ID3DBlob> errors;
    const auto result = D3D12SerializeRootSignature(
        &descriptor, D3D_ROOT_SIGNATURE_VERSION_1, &serialized, &errors);
    CheckNative(result, device, "D3D12SerializeRootSignature", BlobText(errors.Get()));

    ComPtr<ID3D12RootSignature> signature;
    CheckNative(device->CreateRootSignature(0, serialized->GetBufferPointer(),
                                           serialized->GetBufferSize(),
                                           IID_PPV_ARGS(&signature)),
                device, "CreateRootSignature", "MVP b0, texture-array t0, point-wrap s0.");
    return signature;
}

D3D12_GRAPHICS_PIPELINE_STATE_DESC PipelineDescriptor(
    ID3D12RootSignature* signature, DXGI_FORMAT color_format,
    DXGI_FORMAT depth_format, std::uint32_t sample_count) {
    D3D12_GRAPHICS_PIPELINE_STATE_DESC descriptor{};
    descriptor.pRootSignature = signature;
    descriptor.SampleMask = std::numeric_limits<UINT>::max();
    for (auto& target : descriptor.BlendState.RenderTarget) {
        target.SrcBlend = D3D12_BLEND_ONE;
        target.DestBlend = D3D12_BLEND_ZERO;
        target.BlendOp = D3D12_BLEND_OP_ADD;
        target.SrcBlendAlpha = D3D12_BLEND_ONE;
        target.DestBlendAlpha = D3D12_BLEND_ZERO;
        target.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        target.LogicOp = D3D12_LOGIC_OP_NOOP;
        target.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }
    descriptor.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    descriptor.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    // Scene meshes retain their RH, counter-clockwise winding. No shader Y flip.
    descriptor.RasterizerState.FrontCounterClockwise = TRUE;
    descriptor.RasterizerState.DepthClipEnable = TRUE;
    descriptor.RasterizerState.MultisampleEnable = sample_count > 1;
    descriptor.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    descriptor.DepthStencilState.DepthEnable = TRUE;
    descriptor.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    descriptor.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    descriptor.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    descriptor.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    descriptor.DepthStencilState.FrontFace = {
        D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
        D3D12_COMPARISON_FUNC_ALWAYS};
    descriptor.DepthStencilState.BackFace = descriptor.DepthStencilState.FrontFace;
    descriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descriptor.NumRenderTargets = 1;
    descriptor.RTVFormats[0] = color_format;
    descriptor.DSVFormat = depth_format;
    descriptor.SampleDesc = {sample_count, 0};
    return descriptor;
}

} // namespace

PipelineSet CreatePipelines(ID3D12Device* device,
                            const std::filesystem::path& shader_root,
                            DXGI_FORMAT color_format, DXGI_FORMAT depth_format,
                            std::uint32_t sample_count) {
    if (!device || sample_count == 0 || color_format == DXGI_FORMAT_UNKNOWN ||
        depth_format == DXGI_FORMAT_UNKNOWN) {
        throw RenderError(RenderErrorCategory::InvalidInput, "D3D12", "CreatePipelines",
                          "Device, formats, and a nonzero sample count are required");
    }
    const auto blocks_vertex_path = shader_root / "blocks.vs.dxil";
    const auto blocks_pixel_path = shader_root / "blocks.ps.dxil";
    const auto selection_vertex_path = shader_root / "selection.vs.dxil";
    const auto selection_pixel_path = shader_root / "selection.ps.dxil";
    const auto blocks_vertex = ReadShader(blocks_vertex_path);
    const auto blocks_pixel = ReadShader(blocks_pixel_path);
    const auto selection_vertex = ReadShader(selection_vertex_path);
    const auto selection_pixel = ReadShader(selection_pixel_path);

    static_assert(sizeof(BlockVertex3D) == 28);
    static_assert(offsetof(BlockVertex3D, pos_coord) == 0);
    static_assert(offsetof(BlockVertex3D, tex_coord) == 12);
    static_assert(offsetof(BlockVertex3D, normal) == 24);
    static_assert(sizeof(LineVertex3D) == 12);
    static_assert(offsetof(LineVertex3D, pos_coord) == 0);
    constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 3> mesh_input{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_SINT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        {"DIRECTION", 0, DXGI_FORMAT_R32_FLOAT, 0, 24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    }};
    constexpr std::array<D3D12_INPUT_ELEMENT_DESC, 1> selection_input{{
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
    }};

    PipelineSet pipelines;
    pipelines.root_signature = CreateRootSignature(device);
    auto descriptor = PipelineDescriptor(pipelines.root_signature.Get(), color_format,
                                         depth_format, sample_count);
    descriptor.VS = {blocks_vertex.data(), blocks_vertex.size()};
    descriptor.PS = {blocks_pixel.data(), blocks_pixel.size()};
    descriptor.InputLayout = {mesh_input.data(), static_cast<UINT>(mesh_input.size())};
    CheckNative(device->CreateGraphicsPipelineState(&descriptor, IID_PPV_ARGS(&pipelines.mesh)),
                device, "CreateGraphicsPipelineState(mesh)",
                "VS: " + PathText(blocks_vertex_path) + "\nPS: " + PathText(blocks_pixel_path));

    descriptor.VS = {selection_vertex.data(), selection_vertex.size()};
    descriptor.PS = {selection_pixel.data(), selection_pixel.size()};
    descriptor.InputLayout = {selection_input.data(), static_cast<UINT>(selection_input.size())};
    descriptor.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    descriptor.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    CheckNative(device->CreateGraphicsPipelineState(&descriptor, IID_PPV_ARGS(&pipelines.selection)),
                device, "CreateGraphicsPipelineState(selection)",
                "VS: " + PathText(selection_vertex_path) + "\nPS: " + PathText(selection_pixel_path));
    return pipelines;
}

} // namespace SymoCraft::Rendering::Detail::D3D12
