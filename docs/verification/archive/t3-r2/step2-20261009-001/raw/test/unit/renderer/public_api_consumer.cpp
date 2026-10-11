#include <symocraft/renderer/error.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/renderer/types.h>

#include <iostream>
#include <memory>
#include <type_traits>

using namespace SymoCraft::Rendering;
using SymoCraft::MeshData;

static_assert(std::is_final_v<Renderer>);
static_assert(!std::is_copy_constructible_v<Renderer>);
static_assert(!std::is_copy_assignable_v<Renderer>);
static_assert(!std::is_move_constructible_v<Renderer>);
static_assert(!std::is_move_assignable_v<Renderer>);
static_assert(std::is_nothrow_destructible_v<Renderer>);
static_assert(std::is_constructible_v<Renderer, SymoCraft::Window&, const RendererConfig&>);
static_assert(sizeof(Renderer) == sizeof(std::unique_ptr<int>));
static_assert(std::is_default_constructible_v<MeshHandle>);
static_assert(std::is_copy_constructible_v<MeshHandle>);
static_assert(!std::is_same_v<MeshHandle, TextureArrayHandle>);
static_assert(!std::is_convertible_v<MeshHandle, TextureArrayHandle>);
static_assert(!std::is_constructible_v<MeshHandle, std::uint64_t, std::size_t, std::uint64_t>);
static_assert(!std::is_constructible_v<TextureArrayHandle, std::uint64_t, std::size_t, std::uint64_t>);
static_assert(std::is_same_v<decltype(&Renderer::CreateMesh), MeshHandle (Renderer::*)(const MeshData&)>);
static_assert(std::is_same_v<decltype(&Renderer::UpdateMesh), void (Renderer::*)(MeshHandle, const MeshData&)>);
static_assert(std::is_same_v<decltype(&Renderer::DestroyMesh), void (Renderer::*)(MeshHandle)>);
static_assert(std::is_same_v<decltype(&Renderer::CreateTextureArray), TextureArrayHandle (Renderer::*)(const TextureArrayData&)>);
static_assert(std::is_same_v<decltype(&Renderer::DestroyTextureArray), void (Renderer::*)(TextureArrayHandle)>);
static_assert(std::is_same_v<decltype(&Renderer::Render), FrameResult (Renderer::*)(const FrameInput&)>);
static_assert(std::is_same_v<decltype(&Renderer::Resize), void (Renderer::*)(Extent2D)>);
static_assert(std::is_same_v<decltype(&Renderer::SetVSync), void (Renderer::*)(bool)>);
static_assert(std::is_same_v<decltype(&Renderer::GetInfo), RendererInfo (Renderer::*)() const>);
static_assert(std::is_same_v<decltype(&Renderer::PollGpuSamples), std::vector<GpuFrameSample> (Renderer::*)()>);
static_assert(std::is_same_v<decltype(&Renderer::Shutdown), void (Renderer::*)()>);
static_assert(std::is_same_v<decltype(Screenshot::pixels), std::vector<std::uint8_t>>);
static_assert(std::is_same_v<decltype(FrameInput::draws), std::span<const MeshDraw>>);
static_assert(std::is_same_v<decltype(GpuFrameSample::gpu_draw_time), std::optional<std::chrono::nanoseconds>>);

namespace {
    void ConstructAndDestroy(SymoCraft::Window& window, const RendererConfig& config)
    {
        Renderer renderer{window, config};
    }

    // Volatile references keep production definitions required in Release link builds too.
    void (*volatile construct_probe)(SymoCraft::Window&, const RendererConfig&) = &ConstructAndDestroy;
    auto volatile create_mesh = &Renderer::CreateMesh;
    auto volatile update_mesh = &Renderer::UpdateMesh;
    auto volatile destroy_mesh = &Renderer::DestroyMesh;
    auto volatile create_texture = &Renderer::CreateTextureArray;
    auto volatile destroy_texture = &Renderer::DestroyTextureArray;
    auto volatile render_frame = &Renderer::Render;
    auto volatile resize = &Renderer::Resize;
    auto volatile set_vsync = &Renderer::SetVSync;
    auto volatile get_info = &Renderer::GetInfo;
    auto volatile poll_samples = &Renderer::PollGpuSamples;
    auto volatile shutdown = &Renderer::Shutdown;
}

int main()
{
    if (!construct_probe || !create_mesh || !update_mesh || !destroy_mesh || !create_texture ||
        !destroy_texture || !render_frame || !resize || !set_vsync || !get_info || !poll_samples || !shutdown)
        return 1;

    const MeshHandle mesh;
    const TextureArrayHandle texture;
    if (!mesh.IsNull() || !texture.IsNull()) return 2;

    const RenderError error{RenderErrorCategory::Unsupported, "D3D12", "CreateBackend",
                            "backend not delivered", "native-test-code", "diagnostic-test"};
    if (error.Category() != RenderErrorCategory::Unsupported || error.BackendName() != "D3D12" ||
        error.Operation() != "CreateBackend" || error.NativeCode() != "native-test-code" ||
        error.Diagnostic() != "diagnostic-test" || std::string{error.what()}.find("backend not delivered") == std::string::npos)
        return 3;

    std::cout << "R2 public API shape and real production symbols linked; no backend or GPU acceptance.\n";
    return 0;
}
