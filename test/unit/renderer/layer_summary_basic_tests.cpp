#include "renderer_impl.h"
#include "validation.h"
#include <array>
#include <iostream>

namespace {
using namespace SymoCraft;
using namespace SymoCraft::Rendering;
using namespace SymoCraft::Rendering::Detail;
void Require(bool ok) { if (!ok) throw std::runtime_error("layer summary basic check failed"); }
MeshData Triangle(unsigned maximum_layer) {
    MeshData mesh;
    mesh.vertices = {{{0, 0, 0}, {0, 0, 1}, 0},
                     {{1, 0, 0}, {1, 0, float(maximum_layer)}, 0},
                     {{0, 1, 0}, {0, 1, 2}, 0}};
    return mesh;
}
// A normal-path CPU recorder, not a GPU or fault fixture.
class BasicBackend final : public RenderBackend {
public:
    explicit BasicBackend(Backend backend) {
        info.backend = backend; info.framebuffer_extent = {64, 64};
        info.capabilities.texture_arrays = true;
    }
    RendererInfo GetInfo() const override { return info; }
    void CreateMesh(BackendResourceId, std::shared_ptr<const MeshData> data) override { accepted = std::move(data); }
    void UpdateMesh(BackendResourceId, std::shared_ptr<const MeshData> data) override { accepted = std::move(data); }
    void DestroyMesh(BackendResourceId) override {}
    void CreateTextureArray(BackendResourceId, std::shared_ptr<const TextureArrayData>) override {}
    void DestroyTextureArray(BackendResourceId) override {}
    FrameResult Render(const PreparedFrame& input) override {
        Require(input.draws.size() == 1 && input.draws[0].mesh_data == accepted);
        ++renders;
        FrameResult result; result.frame_id = input.frame_id; result.status = FrameStatus::Presented;
        return result;
    }
    void Resize(Extent2D extent) override { info.framebuffer_extent = extent; }
    void SetVSync(bool) override {}
    std::vector<GpuFrameSample> PollGpuSamples() override { return {}; }
    void Shutdown() override {}
    void ShutdownNoexcept() noexcept override {}
    RendererInfo info;
    std::shared_ptr<const MeshData> accepted;
    unsigned renders{};
};
void Summary() {
    ResourceRegistry resources;
    auto first = Triangle(3);
    const auto reservation = resources.PrepareMesh(first);
    Require(reservation.maximum_layer == 3);
    resources.Commit(reservation);
    Require(resources.MeshMaximumLayer(reservation.handle) == 3);
    ValidateMeshTextureLayers(resources.MeshMaximumLayer(reservation.handle), 8);
    auto next = std::make_shared<const MeshData>(Triangle(5));
    const auto maximum_layer = ValidateMesh(*next);
    resources.ReplaceMesh(reservation.handle, next, maximum_layer);
    Require(resources.MeshMaximumLayer(reservation.handle) == 5);
    Require(resources.Mesh(reservation.handle) == next);
    ValidateMeshTextureLayers(resources.MeshMaximumLayer(reservation.handle), 8);
}
void NormalRender(Backend backend) {
    auto recorder = std::make_unique<BasicBackend>(backend);
    auto* calls = recorder.get();
    RendererConfig config; config.backend = backend;
    RendererState state(config, std::move(recorder));
    const auto mesh = state.CreateMesh(Triangle(3));
    TextureArrayData texture; texture.width = texture.height = 1; texture.layers = 8;
    texture.pixels.resize(32, 255);
    const auto atlas = state.CreateTextureArray(texture);
    const std::array draws{MeshDraw{mesh, atlas}};
    FrameInput frame; frame.frame_id = 1; frame.draws = draws;
    Require(state.Render(frame).status == FrameStatus::Presented);
    state.UpdateMesh(mesh, Triangle(5));
    frame.frame_id = 2;
    Require(state.Render(frame).status == FrameStatus::Presented);
    Require(calls->renders == 2 && calls->accepted->vertices[1].tex_coord.z == 5);
    state.Shutdown();
}
}
int main() {
    try {
        Summary(); NormalRender(Backend::D3D12); NormalRender(Backend::Vulkan);
        std::cout << "3 basic cases passed: summary create/update and both backend identities' normal CPU render paths; no GPU/boundary/regression tests\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
