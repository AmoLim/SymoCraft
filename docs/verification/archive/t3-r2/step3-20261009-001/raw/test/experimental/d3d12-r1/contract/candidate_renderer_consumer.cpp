#include "candidate_renderer.h"

#if defined(_WINDOWS_) || defined(_INC_WINDOWS) || defined(__d3d12_h__) || \
    defined(__dxgi_h__) || defined(VK_VERSION_1_0) || defined(GL_VERSION_1_0) || \
    defined(SDL_MAJOR_VERSION) || defined(SDL_h_)
#error The candidate public consumer must not receive graphics or window SDK headers.
#endif

#if defined(NOMINMAX) || defined(WIN32_LEAN_AND_MEAN) || \
    defined(VK_USE_PLATFORM_WIN32_KHR) || defined(VK_NO_PROTOTYPES) || \
    defined(GLM_FORCE_DEPTH_ZERO_TO_ONE) || defined(GLM_FORCE_LEFT_HANDED)
#error The candidate public consumer must not receive backend configuration macros.
#endif

namespace Candidate = SymoCraft::Experimental::D3D12R1::Candidate;
using Candidate::Renderer;

static_assert(std::is_final_v<Renderer>);
static_assert(std::is_constructible_v<Renderer, SymoCraft::Window&, const Candidate::RendererConfig&>);
static_assert(!std::is_default_constructible_v<Renderer>);
static_assert(!std::is_copy_constructible_v<Renderer> && !std::is_copy_assignable_v<Renderer>);
static_assert(!std::is_move_constructible_v<Renderer> && !std::is_move_assignable_v<Renderer>);
static_assert(std::is_nothrow_destructible_v<Renderer>);
static_assert(sizeof(Renderer) == sizeof(std::unique_ptr<int>));

using CreateMesh = Candidate::MeshHandle (Renderer::*)(const SymoCraft::MeshData&);
using UpdateMesh = void (Renderer::*)(Candidate::MeshHandle, const SymoCraft::MeshData&);
using DestroyMesh = void (Renderer::*)(Candidate::MeshHandle);
using CreateTextureArray = Candidate::TextureArrayHandle (Renderer::*)(const Candidate::TextureArrayData&);
using DestroyTextureArray = void (Renderer::*)(Candidate::TextureArrayHandle);
using Render = Candidate::FrameResult (Renderer::*)(const Candidate::FrameInput&);
using Resize = void (Renderer::*)(Candidate::Extent2D);
using SetVSync = void (Renderer::*)(bool);
using GetInfo = Candidate::RendererInfo (Renderer::*)() const;
using PollGpuSamples = std::vector<Candidate::GpuFrameSample> (Renderer::*)();
using Shutdown = void (Renderer::*)();

static_assert(std::is_same_v<decltype(&Renderer::CreateMesh), CreateMesh>);
static_assert(std::is_same_v<decltype(&Renderer::UpdateMesh), UpdateMesh>);
static_assert(std::is_same_v<decltype(&Renderer::DestroyMesh), DestroyMesh>);
static_assert(std::is_same_v<decltype(&Renderer::CreateTextureArray), CreateTextureArray>);
static_assert(std::is_same_v<decltype(&Renderer::DestroyTextureArray), DestroyTextureArray>);
static_assert(std::is_same_v<decltype(&Renderer::Render), Render>);
static_assert(std::is_same_v<decltype(&Renderer::Resize), Resize>);
static_assert(std::is_same_v<decltype(&Renderer::SetVSync), SetVSync>);
static_assert(std::is_same_v<decltype(&Renderer::GetInfo), GetInfo>);
static_assert(std::is_same_v<decltype(&Renderer::PollGpuSamples), PollGpuSamples>);
static_assert(std::is_same_v<decltype(&Renderer::Shutdown), Shutdown>);

static_assert(std::is_same_v<decltype(Candidate::MeshDraw{}.mesh), Candidate::MeshHandle>);
static_assert(std::is_same_v<decltype(Candidate::MeshDraw{}.texture_array), Candidate::TextureArrayHandle>);
static_assert(std::is_same_v<decltype(Candidate::FrameInput{}.draws), std::span<const Candidate::MeshDraw>>);
static_assert(std::is_same_v<decltype(Candidate::FrameInput{}.camera), Candidate::CameraParameters>);
static_assert(std::is_same_v<decltype(Candidate::FrameResult{}.screenshot), std::optional<Candidate::Screenshot>>);
static_assert(std::is_same_v<decltype(Candidate::Screenshot{}.extent), Candidate::Extent2D>);
static_assert(std::is_base_of_v<std::runtime_error, Candidate::RenderError>);
static_assert(!std::is_convertible_v<Candidate::MeshHandle, Candidate::TextureArrayHandle>);
static_assert(!std::is_constructible_v<Candidate::MeshHandle, std::uint64_t, std::size_t, std::uint64_t>);

int main() {
    // No Renderer construction, runtime backend, window, or GPU operation.
    return 0;
}
