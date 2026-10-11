#include "renderer_impl.h"

#include <array>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {
    using namespace SymoCraft::Rendering;
    using namespace SymoCraft::Rendering::Detail;
    using SymoCraft::MeshData;

    int checks = 0;

    void Require(bool condition, const char* message)
    {
        ++checks;
        if (!condition) throw std::runtime_error(message);
    }

    template<class Function>
    void ExpectCategory(RenderErrorCategory category, Function&& function, const char* message)
    {
        ++checks;
        try {
            function();
        } catch (const RenderError& error) {
            if (error.Category() == category && !error.Operation().empty() && !error.BackendName().empty()) return;
            throw std::runtime_error(std::string{message} + ": unexpected RenderError category or missing context");
        }
        throw std::runtime_error(std::string{message} + ": did not throw RenderError");
    }

    struct Calls {
        int creates = 0;
        int updates = 0;
        int mesh_destroys = 0;
        int texture_creates = 0;
        int texture_destroys = 0;
        int renders = 0;
        int resizes = 0;
        int vsync_changes = 0;
        int polls = 0;
        int shutdowns = 0;
        int fallback_shutdowns = 0;
        BackendResourceId last_mesh_id;
        BackendResourceId last_texture_id;
        std::shared_ptr<const MeshData> accepted_mesh;
        std::shared_ptr<const TextureArrayData> accepted_texture;
        PreparedFrame last_frame;
    };

    // This backend records CPU boundary calls only. It never simulates GPU completion.
    class RecordingBackend final : public RenderBackend {
    public:
        explicit RecordingBackend(std::shared_ptr<Calls> calls) : calls_(std::move(calls))
        {
            info.backend = Backend::D3D12;
            info.device_name = "test-only CPU call recorder";
            info.device_identity = "no-device-no-GPU";
            info.framebuffer_extent = {2, 1};
            info.actual_samples = 1;
            info.actual_present_mode = PresentMode::Fifo;
            info.capabilities = {true, true, true, true};
        }

        RendererInfo GetInfo() const override
        {
            if (on_info) return on_info();
            if (throw_info) throw std::runtime_error("test-only initial info failure");
            return info;
        }
        void CreateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override
        {
            ++calls_->creates;
            Fail("CreateMesh");
            calls_->last_mesh_id = id;
            calls_->accepted_mesh = std::move(data);
        }
        void UpdateMesh(BackendResourceId id, std::shared_ptr<const MeshData> data) override
        {
            ++calls_->updates;
            Fail("UpdateMesh");
            calls_->last_mesh_id = id;
            calls_->accepted_mesh = std::move(data);
        }
        void DestroyMesh(BackendResourceId) override
        {
            ++calls_->mesh_destroys;
            Fail("DestroyMesh");
        }
        void CreateTextureArray(BackendResourceId id, std::shared_ptr<const TextureArrayData> data) override
        {
            ++calls_->texture_creates;
            Fail("CreateTextureArray");
            calls_->last_texture_id = id;
            calls_->accepted_texture = std::move(data);
        }
        void DestroyTextureArray(BackendResourceId) override
        {
            ++calls_->texture_destroys;
            Fail("DestroyTextureArray");
        }
        FrameResult Render(const PreparedFrame& frame) override
        {
            ++calls_->renders;
            Fail("Render");
            calls_->last_frame = frame;
            if (on_render) on_render();
            FrameResult result;
            result.frame_id = inconsistent_frame_id ? frame.frame_id + 1 : frame.frame_id;
            result.status = occluded ? FrameStatus::Skipped : FrameStatus::Presented;
            result.skip_reason = occluded ? FrameSkipReason::Occluded : FrameSkipReason::None;
            if (frame.request_screenshot && !occluded)
                result.screenshot = Screenshot{frame.extent, {1, 2, 3, 4, 5, 6, 7, 8}};
            if (on_result) on_result(result);
            return result;
        }
        void Resize(Extent2D extent) override
        {
            ++calls_->resizes;
            Fail("Resize");
            info.framebuffer_extent = extent;
            if (on_resize) on_resize(info);
        }
        void SetVSync(bool enabled) override
        {
            ++calls_->vsync_changes;
            Fail("SetVSync");
            info.actual_present_mode = enabled ? PresentMode::Fifo : PresentMode::Immediate;
            if (on_vsync) on_vsync(info, enabled);
        }
        std::vector<GpuFrameSample> PollGpuSamples() override
        {
            ++calls_->polls;
            Fail("PollGpuSamples");
            return samples;
        }
        void Shutdown() override
        {
            ++calls_->shutdowns;
            Fail("Shutdown");
        }
        void ShutdownNoexcept() noexcept override { ++calls_->fallback_shutdowns; }

        void Fail(const char* operation)
        {
            if (failure_operation != operation) return;
            failure_operation.clear();
            if (throw_bad_alloc) throw std::bad_alloc{};
            if (throw_unknown) throw 7;
            throw RenderError{failure_category, "D3D12", operation, "test-only injected failure",
                              "test-native", "not a GPU fault experiment"};
        }

        RendererInfo info;
        std::string failure_operation;
        RenderErrorCategory failure_category = RenderErrorCategory::AllocationFailure;
        bool throw_bad_alloc = false;
        bool throw_unknown = false;
        bool inconsistent_frame_id = false;
        bool occluded = false;
        bool throw_info = false;
        std::function<void()> on_render;
        std::function<void(FrameResult&)> on_result;
        std::function<void(RendererInfo&)> on_resize;
        std::function<void(RendererInfo&, bool)> on_vsync;
        std::function<RendererInfo()> on_info;
        std::vector<GpuFrameSample> samples;

    private:
        std::shared_ptr<Calls> calls_;
    };

    struct Fixture {
        explicit Fixture(std::function<void(RecordingBackend&)> configure = {})
        {
            auto backend = std::make_unique<RecordingBackend>(calls);
            recording = backend.get();
            if (configure) configure(*recording);
            state = std::make_unique<RendererState>(RendererConfig{}, std::move(backend));
        }

        std::shared_ptr<Calls> calls = std::make_shared<Calls>();
        RecordingBackend* recording = nullptr;
        std::unique_ptr<RendererState> state;
    };

    MeshData Triangle(float layer = 0.0f)
    {
        return {{{{0, 0, 0}, {0.0f, 0.0f, layer}, 1.0f},
                 {{1, 0, 0}, {1.0f, 0.0f, layer}, 1.0f},
                 {{0, 1, 0}, {0.0f, 1.0f, layer}, 1.0f}}};
    }

    TextureArrayData Texture(std::uint32_t layers = 2)
    {
        TextureArrayData texture;
        texture.width = 1;
        texture.height = 1;
        texture.layers = layers;
        texture.pixels.resize(static_cast<std::size_t>(layers) * 4, 127);
        return texture;
    }

    FrameInput Frame(std::uint64_t id, std::span<const MeshDraw> draws = {})
    {
        FrameInput frame;
        frame.frame_id = id;
        frame.draws = draws;
        return frame;
    }

    void CheckResourceOwnership()
    {
        Fixture first;
        Fixture second;
        auto mesh_data = Triangle();
        auto texture_data = Texture();
        const auto mesh = first.state->CreateMesh(mesh_data);
        const auto texture = first.state->CreateTextureArray(texture_data);
        Require(!mesh.IsNull() && !texture.IsNull(), "accepted resources must have nonnull handles");
        mesh_data.vertices[0].pos_coord.x = 99;
        texture_data.pixels[0] = 3;
        Require(first.calls->accepted_mesh->vertices[0].pos_coord.x == 0, "mesh input must be self-owned at acceptance");
        Require(first.calls->accepted_texture->pixels[0] == 127, "texture input must be self-owned at acceptance");

        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { first.state->UpdateMesh({}, Triangle()); }, "null mesh update");
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { first.state->DestroyTextureArray({}); }, "null texture destroy");
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { second.state->DestroyMesh(mesh); }, "cross-owner mesh");
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { second.state->DestroyTextureArray(texture); }, "cross-owner texture");
        Require(second.calls->mesh_destroys == 0 && second.calls->texture_destroys == 0, "foreign handles must not reach backend");

        const auto first_id = first.calls->last_mesh_id;
        first.state->DestroyMesh(mesh);
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { first.state->DestroyMesh(mesh); }, "repeated mesh destroy");
        const auto replacement = first.state->CreateMesh(Triangle());
        Require(replacement != mesh && first.calls->last_mesh_id.slot == first_id.slot &&
                first.calls->last_mesh_id.generation != first_id.generation, "reused slot must advance generation");
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { first.state->UpdateMesh(mesh, Triangle()); }, "stale mesh update after slot reuse");
        first.state->DestroyTextureArray(texture);
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { first.state->DestroyTextureArray(texture); }, "repeated texture destroy");
        const auto new_texture = first.state->CreateTextureArray(Texture());
        Require(new_texture != texture, "texture generation must not resurrect stale handles");

        const auto empty_mesh = first.state->CreateMesh({});
        const std::array draws{MeshDraw{empty_mesh, new_texture}};
        first.state->Render(Frame(0, draws));
        Require(first.calls->last_frame.draws[0].mesh_data->vertices.empty(), "empty mesh must remain a valid owned resource");
    }

    void CheckConstructionAndCleanup()
    {
        auto mismatch_calls = std::make_shared<Calls>();
        auto mismatch = std::make_unique<RecordingBackend>(mismatch_calls);
        mismatch->info.backend = Backend::Vulkan;
        ExpectCategory(RenderErrorCategory::InitializationFailure, [&] {
            RendererState state{RendererConfig{}, std::move(mismatch)};
        }, "requested and actual backend identity must match");
        Require(mismatch_calls->fallback_shutdowns == 1, "partial construction mismatch must clean up backend");

        auto info_calls = std::make_shared<Calls>();
        auto info = std::make_unique<RecordingBackend>(info_calls);
        info->throw_info = true;
        ExpectCategory(RenderErrorCategory::InitializationFailure, [&] {
            RendererState state{RendererConfig{}, std::move(info)};
        }, "initial info exception translation");
        Require(info_calls->fallback_shutdowns == 1, "initial info failure must clean up backend");

        ExpectCategory(RenderErrorCategory::InitializationFailure, [&] {
            RendererState state{RendererConfig{}, nullptr};
        }, "null internal backend must not form a usable renderer");

        Fixture shutdown_failure;
        shutdown_failure.recording->failure_operation = "Shutdown";
        ExpectCategory(RenderErrorCategory::AllocationFailure, [&] { shutdown_failure.state->Shutdown(); }, "shutdown failure diagnostic");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { shutdown_failure.state->Render(Frame(0)); }, "failed shutdown must prevent resumed submission");
        shutdown_failure.state->Shutdown();
        shutdown_failure.state->Shutdown();
        Require(shutdown_failure.calls->shutdowns == 2, "cleanup retry may succeed, then must become idempotent");
    }

    void CheckTransactionalUpdates()
    {
        Fixture fixture;
        const auto mesh = fixture.state->CreateMesh(Triangle());
        const auto texture = fixture.state->CreateTextureArray(Texture());
        std::array draws{MeshDraw{mesh, texture}};
        auto update = Triangle();
        update.vertices[0].pos_coord.x = 23;
        fixture.recording->failure_operation = "UpdateMesh";
        ExpectCategory(RenderErrorCategory::AllocationFailure, [&] { fixture.state->UpdateMesh(mesh, update); }, "backend update rejected");
        fixture.state->Render(Frame(0, draws));
        Require(fixture.calls->last_frame.draws[0].mesh_data->vertices[0].pos_coord.x == 0, "rejected update must preserve prior CPU publication");
        fixture.state->UpdateMesh(mesh, update);
        update.vertices[0].pos_coord.x = 77;
        fixture.state->Render(Frame(1, draws));
        Require(fixture.calls->last_frame.draws[0].mesh_data->vertices[0].pos_coord.x == 23, "successful update must self-own new publication");

        auto invalid = Triangle();
        invalid.vertices.pop_back();
        const auto updates = fixture.calls->updates;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->UpdateMesh(mesh, invalid); }, "invalid mesh update");
        Require(fixture.calls->updates == updates, "invalid update must never reach backend");
        fixture.state->Render(Frame(2, draws));
        Require(fixture.calls->last_frame.draws[0].mesh_data->vertices[0].pos_coord.x == 23, "input rejection must retain successful update");

        fixture.recording->failure_operation = "DestroyMesh";
        ExpectCategory(RenderErrorCategory::AllocationFailure, [&] { fixture.state->DestroyMesh(mesh); }, "failed destroy");
        fixture.state->Render(Frame(3, draws));
        Require(fixture.calls->last_frame.draws[0].mesh_data->vertices.size() == 3, "failed destroy must preserve live resource");
        fixture.state->DestroyMesh(mesh);
        const auto renders = fixture.calls->renders;
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { fixture.state->Render(Frame(4, draws)); }, "destroyed resource in frame");
        Require(fixture.calls->renders == renders, "destroyed resource must not submit a frame");

        Fixture allocation;
        allocation.recording->failure_operation = "CreateMesh";
        allocation.recording->throw_bad_alloc = true;
        ExpectCategory(RenderErrorCategory::AllocationFailure, [&] { allocation.state->CreateMesh(Triangle()); }, "CPU bad_alloc translation");
        Require(!allocation.state->CreateMesh(Triangle()).IsNull(), "allocation failure must permit a later valid create");
    }

    void CheckInputValidation()
    {
        Fixture fixture;
        auto bad_mesh = Triangle();
        bad_mesh.vertices.pop_back();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateMesh(bad_mesh); }, "incomplete triangle");
        for (const float layer : {-1.0f, 0.5f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN(), 4294967296.0f}) {
            bad_mesh = Triangle(layer);
            ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateMesh(bad_mesh); }, "invalid layer encoding");
        }
        bad_mesh = Triangle();
        bad_mesh.vertices[0].tex_coord.x = std::numeric_limits<float>::quiet_NaN();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateMesh(bad_mesh); }, "nonfinite UV");
        bad_mesh = Triangle();
        bad_mesh.vertices[0].normal = std::numeric_limits<float>::infinity();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateMesh(bad_mesh); }, "nonfinite material value");
        Require(fixture.calls->creates == 0, "invalid mesh input must not create native resources");

        auto bad_texture = Texture();
        bad_texture.width = 0;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "zero texture dimension");
        bad_texture = Texture();
        bad_texture.pixels.pop_back();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "short pixel payload");
        bad_texture = Texture();
        bad_texture.pixels.push_back(0);
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "oversized pixel payload");
        bad_texture = Texture();
        bad_texture.format = static_cast<PixelFormat>(99);
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "unsupported pixel encoding");
        bad_texture = Texture();
        bad_texture.color_space = static_cast<ColorSpace>(99);
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "unsupported color encoding");
        bad_texture = {};
        bad_texture.width = bad_texture.height = bad_texture.layers = std::numeric_limits<std::uint32_t>::max();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->CreateTextureArray(bad_texture); }, "texture byte multiplication overflow");
        Require(fixture.calls->texture_creates == 0, "invalid texture input must not create native resources");

        const auto texture = fixture.state->CreateTextureArray(Texture(2));
        auto mixed = Triangle();
        mixed.vertices[1].tex_coord.z = 1.0f;
        const auto mixed_mesh = fixture.state->CreateMesh(mixed);
        const std::array mixed_draws{MeshDraw{mixed_mesh, texture}};
        fixture.state->Render(Frame(0, mixed_draws));
        Require(fixture.calls->last_frame.draws[0].mesh_data->vertices[1].tex_coord.z == 1.0f, "mixed-layer triangle must not acquire an unapproved restriction");

        const auto beyond = fixture.state->CreateMesh(Triangle(2.0f));
        const std::array invalid_draws{MeshDraw{mixed_mesh, texture}, MeshDraw{beyond, texture}};
        const auto renders = fixture.calls->renders;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(Frame(1, invalid_draws)); }, "second draw layer outside texture");
        Require(fixture.calls->renders == renders, "all draws must validate before any backend frame submission");
        fixture.state->Render(Frame(1, mixed_draws));
        Require(fixture.calls->renders == renders + 1, "rejected frame must not consume frame identity");
    }

    void CheckFrameValidationAndOwnedResults()
    {
        Fixture fixture;
        const auto mesh = fixture.state->CreateMesh(Triangle());
        const auto texture = fixture.state->CreateTextureArray(Texture());
        std::array draws{MeshDraw{mesh, texture}};
        auto frame = Frame(0, draws);
        frame.camera.forward = {};
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "zero camera forward");
        frame = Frame(0, draws);
        frame.camera.up = frame.camera.forward;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "collinear camera basis");
        frame = Frame(0, draws);
        frame.camera.position.x = std::numeric_limits<float>::infinity();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "nonfinite camera");
        frame = Frame(0, draws);
        frame.camera.near_plane = 0.0f;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "invalid near plane");
        frame = Frame(0, draws);
        frame.camera.far_plane = frame.camera.near_plane;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "invalid far plane");
        frame = Frame(0, draws);
        frame.camera.vertical_fov_radians = std::numbers::pi_v<float>;
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "invalid field of view");
        frame = Frame(0, draws);
        draws[0].object_transform[3][0] = std::numeric_limits<float>::quiet_NaN();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "nonfinite object transform");
        draws[0].object_transform = glm::mat4{1.0f};
        frame.selection = SelectionBox{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}};
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "inverted selection");
        frame.selection = SelectionBox{};
        frame.selection->minimum.x = std::numeric_limits<float>::infinity();
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(frame); }, "nonfinite selection");
        Require(fixture.calls->renders == 0, "invalid frame fields must not reach backend");

        frame = Frame(0, draws);
        const auto large = std::numeric_limits<float>::max();
        frame.camera.forward = {large, 0.0f, 0.0f};
        frame.camera.up = {0.0f, large, 0.0f};
        frame.selection = SelectionBox{{-large, -large, -large}, {large, large, large}};
        draws[0].object_transform[0][0] = large;
        frame.request_screenshot = true;
        auto result = fixture.state->Render(frame);
        Require(result.frame_id == 0 && result.status == FrameStatus::Presented, "first frame zero and finite extremes remain legal CPU inputs");
        Require(result.screenshot && result.screenshot->pixels.size() == 8, "result must return an owned RGBA screenshot");
        Require(fixture.calls->last_frame.selection->minimum.x == -large, "owned prepared frame must preserve selection values");
        draws[0].object_transform[0][0] = 2.0f;
        frame.selection->minimum.x = 7.0f;
        Require(fixture.calls->last_frame.draws[0].object_transform[0][0] == large &&
                fixture.calls->last_frame.selection->minimum.x == -large, "prepared frame must not borrow caller draw or selection storage");
        result.screenshot->pixels[0] = 99;
        frame = Frame(1);
        frame.request_screenshot = true;
        const auto next = fixture.state->Render(frame);
        Require(next.screenshot->pixels[0] == 1, "screenshot ownership must not alias future results");
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(Frame(1)); }, "duplicate frame identity");
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(Frame(0)); }, "decreasing frame identity");
    }

    void CheckSuspensionAndLifecycle()
    {
        Fixture fixture;
        fixture.state->Resize({0, 12});
        const auto skipped = fixture.state->Render(Frame(0));
        Require(skipped.status == FrameStatus::Skipped && skipped.skip_reason == FrameSkipReason::ZeroExtent &&
                skipped.frame_id == 0 && fixture.calls->renders == 0, "zero extent must skip without backend submit or present");
        ExpectCategory(RenderErrorCategory::InvalidInput, [&] { fixture.state->Render(Frame(0)); }, "skipped frame consumes identity");
        const std::array bad_draws{MeshDraw{}};
        ExpectCategory(RenderErrorCategory::InvalidHandle, [&] { fixture.state->Render(Frame(1, bad_draws)); }, "suspended frames still validate resource handles");
        fixture.state->Resize({2, 1});
        fixture.state->Render(Frame(1));
        Require(fixture.calls->renders == 1, "restored extent must resume backend render");
        fixture.state->Resize({2, 1});
        Require(fixture.calls->resizes == 2, "same extent must not duplicate backend resize");
        fixture.state->SetVSync(false);
        Require(fixture.state->GetInfo().actual_present_mode == PresentMode::Immediate, "GetInfo must report actual backend present mode");
        fixture.state->SetVSync(false);
        Require(fixture.calls->vsync_changes == 1, "unchanged vsync must not duplicate backend work");
        fixture.recording->occluded = true;
        const auto occluded = fixture.state->Render(Frame(2));
        Require(occluded.status == FrameStatus::Skipped && occluded.skip_reason == FrameSkipReason::Occluded, "backend occlusion remains a skipped frame");

        fixture.state->Shutdown();
        fixture.state->Shutdown();
        Require(fixture.calls->shutdowns == 1, "Shutdown must be idempotent after success");
        Require(fixture.state->GetInfo().device_identity == "no-device-no-GPU", "stopped renderer retains last info snapshot");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->CreateMesh({}); }, "resource creation after shutdown");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->Render(Frame(3)); }, "render after shutdown");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->PollGpuSamples(); }, "poll after shutdown");
        fixture.state.reset();
        Require(fixture.calls->fallback_shutdowns == 0, "normal shutdown must not trigger a second backend cleanup");

        auto calls = std::make_shared<Calls>();
        {
            RendererState state{RendererConfig{}, std::make_unique<RecordingBackend>(calls)};
        }
        Require(calls->fallback_shutdowns == 1, "destructor must invoke noexcept backend cleanup when not shut down");
    }

    void CheckThreadAndReentry()
    {
        Fixture fixture;
        std::exception_ptr failure;
        std::thread other([&] {
            try {
                ExpectCategory(RenderErrorCategory::WrongThread, [&] { fixture.state->Render(Frame(0)); }, "wrong-thread Render");
                ExpectCategory(RenderErrorCategory::WrongThread, [&] { fixture.state->GetInfo(); }, "wrong-thread GetInfo");
                ExpectCategory(RenderErrorCategory::WrongThread, [&] { fixture.state->Shutdown(); }, "wrong-thread Shutdown");
            } catch (...) {
                failure = std::current_exception();
            }
        });
        other.join();
        if (failure) std::rethrow_exception(failure);
        Require(fixture.calls->renders == 0 && fixture.calls->shutdowns == 0, "wrong-thread calls must not touch backend");

        fixture.recording->on_render = [&] {
            ExpectCategory(RenderErrorCategory::ReentrantCall, [&] { fixture.state->SetVSync(false); }, "reentrant SetVSync");
            ExpectCategory(RenderErrorCategory::ReentrantCall, [&] { fixture.state->GetInfo(); }, "reentrant GetInfo");
        };
        fixture.state->Render(Frame(0));
        Require(fixture.calls->vsync_changes == 0, "reentrant calls must be rejected before backend work");
        fixture.recording->on_render = {};
        fixture.state->Render(Frame(1));
        Require(fixture.calls->renders == 2, "reentrant rejection must release call guard for later frames");
    }

    void CheckFrameStatisticsComposition()
    {
        Fixture fixture;
        const auto native_prepare = std::chrono::seconds{1};
        fixture.recording->on_result = [&](FrameResult& result) {
            result.statistics.cpu_prepare = native_prepare;
            result.statistics.cpu_submit = std::chrono::nanoseconds{17};
            result.statistics.present_wait = std::chrono::nanoseconds{29};
            result.statistics.upload_bytes = 42;
            result.statistics.draw_calls = 3;
            result.statistics.drawn_vertices = 9;
        };
        const auto result = fixture.state->Render(Frame(0));
        Require(result.statistics.cpu_prepare >= native_prepare,
                "facade preparation must add to native preparation rather than overwrite it");
        Require(result.statistics.cpu_submit == std::chrono::nanoseconds{17} &&
                result.statistics.present_wait == std::chrono::nanoseconds{29},
                "preparation aggregation must preserve native submit and presentation durations");
        Require(result.statistics.upload_bytes == 42 && result.statistics.draw_calls == 3 &&
                result.statistics.drawn_vertices == 9,
                "preparation aggregation must preserve native work counters");

        Fixture overflow;
        const auto mesh = overflow.state->CreateMesh(Triangle());
        const auto texture = overflow.state->CreateTextureArray(Texture());
        // Validate enough owned draws to exceed the real clock's tick resolution.
        const std::vector<MeshDraw> draws(2048, MeshDraw{mesh, texture});
        overflow.recording->on_result = [](FrameResult& value) {
            value.statistics.cpu_prepare = std::chrono::nanoseconds::max();
        };
        ExpectCategory(RenderErrorCategory::SubmissionFailure,
                       [&] { overflow.state->Render(Frame(0, draws)); },
                       "post-acceptance preparation duration overflow");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { overflow.state->Render(Frame(1)); },
                       "preparation overflow must stop further native submission");
        Require(overflow.calls->renders == 1, "preparation overflow must not permit a second backend render");
        Require(overflow.state->GetInfo().device_identity == "no-device-no-GPU",
                "preparation overflow must preserve the last device identity snapshot");
        overflow.state->Shutdown();
        Require(overflow.calls->shutdowns == 1, "preparation overflow must allow bounded backend cleanup");
    }

    void CheckBackendResultValidation()
    {
        const std::array<std::function<void(FrameResult&)>, 8> malformed_results{
            [](FrameResult& result) { result.screenshot.reset(); },
            [](FrameResult& result) { result.screenshot->extent = {1, 2}; },
            [](FrameResult& result) { result.screenshot->pixels.pop_back(); },
            [](FrameResult& result) { result.statistics.cpu_submit = std::chrono::nanoseconds{-1}; },
            [](FrameResult& result) { result.statistics.present_wait = std::chrono::nanoseconds{-1}; },
            [](FrameResult& result) { result.statistics.cpu_prepare = std::chrono::nanoseconds{-1}; },
            [](FrameResult& result) { result.status = static_cast<FrameStatus>(99); },
            [](FrameResult& result) { result.skip_reason = static_cast<FrameSkipReason>(99); }
        };
        for (const auto& malformed_result : malformed_results) {
            Fixture fixture;
            fixture.recording->on_result = malformed_result;
            auto frame = Frame(0);
            frame.request_screenshot = true;
            ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { fixture.state->Render(frame); }, "malformed or missing screenshot/statistics result");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->Render(Frame(1)); }, "malformed backend result must prevent later submit");
            Require(fixture.calls->renders == 1, "malformed result must not permit a second backend render");
        }

        Fixture unsolicited;
        unsolicited.recording->on_result = [](FrameResult& result) {
            result.screenshot = Screenshot{{2, 1}, {1, 2, 3, 4, 5, 6, 7, 8}};
        };
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { unsolicited.state->Render(Frame(0)); }, "unrequested screenshot result");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { unsolicited.state->Render(Frame(1)); }, "unrequested capture must fail backend state");

        Fixture inconsistent_resize;
        inconsistent_resize.recording->on_resize = [](RendererInfo& info) { info.framebuffer_extent = {2, 1}; };
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { inconsistent_resize.state->Resize({4, 2}); }, "resize returned inconsistent actual extent");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { inconsistent_resize.state->Render(Frame(0)); }, "inconsistent resize must stop submissions");
        Require(inconsistent_resize.state->GetInfo().framebuffer_extent.width == 2,
                "failed resize must preserve last successful info snapshot");

        Fixture vsync_extent;
        vsync_extent.recording->on_vsync = [](RendererInfo& info, bool enabled) {
            info.framebuffer_extent = enabled ? Extent2D{2, 1} : Extent2D{0, 1};
        };
        vsync_extent.state->SetVSync(false);
        const auto suspended = vsync_extent.state->Render(Frame(0));
        Require(suspended.status == FrameStatus::Skipped && suspended.skip_reason == FrameSkipReason::ZeroExtent &&
                vsync_extent.calls->renders == 0, "vsync rebuilding to zero extent must synchronize Suspended lifecycle");
        vsync_extent.state->SetVSync(true);
        const auto restored = vsync_extent.state->Render(Frame(1));
        Require(restored.status == FrameStatus::Presented && vsync_extent.calls->renders == 1,
                "vsync rebuilding to positive extent must synchronize Ready lifecycle");

        Fixture native_zero;
        native_zero.recording->on_result = [&](FrameResult& result) {
            result.status = FrameStatus::Skipped;
            result.skip_reason = FrameSkipReason::ZeroExtent;
            native_zero.recording->info.framebuffer_extent = {0, 1};
        };
        const auto native_skipped = native_zero.state->Render(Frame(0));
        Require(native_skipped.status == FrameStatus::Skipped && native_skipped.skip_reason == FrameSkipReason::ZeroExtent &&
                native_zero.state->GetInfo().framebuffer_extent.IsSuspended(),
                "backend zero extent skip must synchronize actual size and Suspended lifecycle");
        native_zero.state->Render(Frame(1));
        Require(native_zero.calls->renders == 1, "subsequent zero extent frame must avoid another backend render");

        Fixture false_zero;
        false_zero.recording->on_result = [](FrameResult& result) {
            result.status = FrameStatus::Skipped;
            result.skip_reason = FrameSkipReason::ZeroExtent;
        };
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { false_zero.state->Render(Frame(0)); }, "zero extent skip with positive actual size");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { false_zero.state->Render(Frame(1)); }, "inconsistent zero extent result must stop submit");
    }

    void CheckFatalBackendFailures()
    {
        for (const auto category : {RenderErrorCategory::DeviceLost, RenderErrorCategory::SubmissionFailure, RenderErrorCategory::Timeout}) {
            Fixture fixture;
            fixture.recording->failure_operation = "Render";
            fixture.recording->failure_category = category;
            ExpectCategory(category, [&] { fixture.state->Render(Frame(0)); }, "test-only fatal backend failure");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->Render(Frame(1)); }, "failed renderer must stop new submissions");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->CreateMesh(Triangle()); }, "failed renderer must stop uploads");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->Resize({5, 5}); }, "failed renderer must stop resize work");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->PollGpuSamples(); }, "failed renderer must stop polls");
            Require(fixture.calls->renders == 1 && fixture.calls->creates == 0 && fixture.calls->resizes == 0 && fixture.calls->polls == 0,
                    "fatal category must prevent all further backend operations except cleanup");
            Require(fixture.state->GetInfo().device_identity == "no-device-no-GPU", "failed renderer must expose last actual info snapshot");
            fixture.state->Shutdown();
            Require(fixture.calls->shutdowns == 1, "failed renderer must allow cleanup");
        }

        Fixture malformed;
        malformed.recording->inconsistent_frame_id = true;
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { malformed.state->Render(Frame(0)); }, "inconsistent backend frame result");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { malformed.state->Render(Frame(1)); }, "malformed result must fail renderer");

        Fixture unknown;
        unknown.recording->failure_operation = "Render";
        unknown.recording->throw_unknown = true;
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { unknown.state->Render(Frame(0)); }, "unknown backend exception translation");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { unknown.state->Render(Frame(1)); }, "unknown failure must stop submissions");

        Fixture diagnostics;
        diagnostics.recording->failure_operation = "Render";
        diagnostics.recording->failure_category = RenderErrorCategory::DeviceLost;
        try {
            diagnostics.state->Render(Frame(0));
            throw std::runtime_error("expected injected diagnostic error");
        } catch (const RenderError& error) {
            Require(error.NativeCode() == "test-native" && error.Diagnostic() == "not a GPU fault experiment",
                    "native codes and diagnostics must survive facade propagation");
        }
    }

    void CheckAcceptedOperationInfoFailures()
    {
        const std::array categories{RenderErrorCategory::AllocationFailure,
                                    RenderErrorCategory::AssetUnavailable,
                                    RenderErrorCategory::InvalidInput};
        for (int operation = 0; operation < 3; ++operation) {
            for (int failure = 0; failure < 3; ++failure) {
                Fixture fixture;
                const auto arm_info_failure = [&] {
                    fixture.recording->on_info = [failure]() -> RendererInfo {
                        if (failure == 0) throw std::bad_alloc{};
                        if (failure == 1)
                            throw RenderError{RenderErrorCategory::AssetUnavailable, "D3D12", "GetInfo",
                                              "test-only accepted-operation info failure", "info-native", "info-diagnostic"};
                        throw std::invalid_argument("test-only invalid info after native acceptance");
                    };
                };
                if (operation == 0) {
                    fixture.recording->on_resize = [&](RendererInfo&) { arm_info_failure(); };
                } else if (operation == 1) {
                    fixture.recording->on_vsync = [&](RendererInfo&, bool) { arm_info_failure(); };
                } else {
                    fixture.recording->on_result = [&](FrameResult& result) {
                        result.status = FrameStatus::Skipped;
                        result.skip_reason = FrameSkipReason::ZeroExtent;
                        fixture.recording->info.framebuffer_extent = {0, 1};
                        arm_info_failure();
                    };
                }
                ExpectCategory(categories[failure], [&] {
                    try {
                        if (operation == 0) fixture.state->Resize({4, 2});
                        else if (operation == 1) fixture.state->SetVSync(false);
                        else fixture.state->Render(Frame(0));
                    } catch (const RenderError& error) {
                        if (failure == 1)
                            Require(error.NativeCode() == "info-native" && error.Diagnostic() == "info-diagnostic",
                                    "post-acceptance info error must retain native diagnostics");
                        throw;
                    }
                }, "info failure after backend operation was accepted must retain error category");
                ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->Render(Frame(1)); }, "post-acceptance info failure must stop submission");
                ExpectCategory(RenderErrorCategory::InvalidState, [&] { fixture.state->CreateMesh(Triangle()); }, "post-acceptance info failure must stop resource mutation");
                const auto cached = fixture.state->GetInfo();
                Require(cached.framebuffer_extent.width == 2 && cached.framebuffer_extent.height == 1 &&
                        cached.actual_present_mode == PresentMode::Fifo,
                        "post-acceptance info failure must retain last complete CPU info snapshot");
                fixture.state->Shutdown();
                Require(fixture.calls->shutdowns == 1, "post-acceptance info failure must still allow cleanup");
            }
        }
    }

    void CheckGpuSampleSemanticsAndCapabilities()
    {
        Fixture fixture;
        fixture.state->Render(Frame(5));
        fixture.state->Render(Frame(6));
        fixture.state->Render(Frame(7));
        fixture.recording->samples = {{5, true, std::chrono::nanoseconds{17}, GpuSampleMissingReason::None},
                                      {6, false, std::nullopt, GpuSampleMissingReason::Unsupported},
                                      {7, false, std::nullopt, GpuSampleMissingReason::Discarded}};
        const auto samples = fixture.state->PollGpuSamples();
        Require(samples.size() == 3 && samples[0].gpu_draw_time->count() == 17 &&
                !samples[1].gpu_draw_time && !samples[2].gpu_draw_time, "missing GPU duration must never become zero duration");
        fixture.recording->samples[0].gpu_draw_time = std::chrono::nanoseconds{99};
        Require(samples[0].gpu_draw_time->count() == 17, "sample batch must own its returned values");

        for (const auto sample : {GpuFrameSample{0, true, std::nullopt, GpuSampleMissingReason::None},
                                  GpuFrameSample{0, false, std::chrono::nanoseconds{0}, GpuSampleMissingReason::Discarded},
                                  GpuFrameSample{0, true, std::chrono::nanoseconds{-1}, GpuSampleMissingReason::None},
                                  GpuFrameSample{0, false, std::nullopt, GpuSampleMissingReason::None}}) {
            Fixture malformed;
            malformed.state->Render(Frame(0));
            malformed.recording->samples = {sample};
            ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { malformed.state->PollGpuSamples(); }, "inconsistent GPU sample");
            ExpectCategory(RenderErrorCategory::InvalidState, [&] { malformed.state->Render(Frame(0)); }, "malformed sample must stop backend submissions");
        }

        Fixture unsubmitted_sample;
        unsubmitted_sample.recording->samples = {{0, false, std::nullopt, GpuSampleMissingReason::Discarded}};
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { unsubmitted_sample.state->PollGpuSamples(); }, "sample without any backend submitted frame");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { unsubmitted_sample.state->Render(Frame(0)); }, "unsubmitted sample must fail renderer");
        Fixture future_sample;
        future_sample.state->Render(Frame(0));
        future_sample.recording->samples = {{1, false, std::nullopt, GpuSampleMissingReason::Discarded}};
        ExpectCategory(RenderErrorCategory::SubmissionFailure, [&] { future_sample.state->PollGpuSamples(); }, "sample claims future frame identity");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { future_sample.state->Render(Frame(1)); }, "future sample must fail renderer");

        Fixture device_failure;
        device_failure.state->Render(Frame(9));
        device_failure.recording->samples = {{9, false, std::nullopt, GpuSampleMissingReason::DeviceFailure}};
        const auto failure_samples = device_failure.state->PollGpuSamples();
        Require(failure_samples.size() == 1 && !failure_samples[0].valid && !failure_samples[0].gpu_draw_time &&
                failure_samples[0].missing_reason == GpuSampleMissingReason::DeviceFailure,
                "device failure sample must remain an explicit missing sample, not a zero duration");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { device_failure.state->Render(Frame(10)); }, "device failure sample must stop submissions");
        ExpectCategory(RenderErrorCategory::InvalidState, [&] { device_failure.state->PollGpuSamples(); }, "device failure sample must stop later polling");
        Require(device_failure.state->GetInfo().device_identity == "no-device-no-GPU", "device failure sample retains diagnostics identity");
        device_failure.state->Shutdown();

        Fixture unsupported([](RecordingBackend& backend) { backend.info.capabilities.screenshots = false; });
        auto frame = Frame(0);
        frame.request_screenshot = true;
        ExpectCategory(RenderErrorCategory::Unsupported, [&] { unsupported.state->Render(frame); }, "unsupported screenshot request");
        Require(unsupported.calls->renders == 0, "unsupported capability must not submit backend frame");
        frame.request_screenshot = false;
        unsupported.state->Render(frame);
        Require(unsupported.calls->renders == 1, "unsupported request must not consume identity or fail renderer");
    }
}

int main()
{
    try {
        CheckResourceOwnership();
        CheckConstructionAndCleanup();
        CheckTransactionalUpdates();
        CheckInputValidation();
        CheckFrameValidationAndOwnedResults();
        CheckSuspensionAndLifecycle();
        CheckThreadAndReentry();
        CheckFrameStatisticsComposition();
        CheckBackendResultValidation();
        CheckFatalBackendFailures();
        CheckAcceptedOperationInfoFailures();
        CheckGpuSampleSemanticsAndCapabilities();
        std::cout << "R2 real facade CPU lifecycle: " << checks
                  << " checks passed with test-only recording backend; no GPU or production backend acceptance.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "R2 facade CPU test failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
