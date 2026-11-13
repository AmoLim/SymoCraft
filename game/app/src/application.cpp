#include "application.h"
#include "application_probe.h"
#include "benchmark_scenario.h"
#include "game_renderer.h"
#include "startup_options.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/foundation/document_io.h>
#include <symocraft/world/benchmark_workload.h>
#include <symocraft/platform/window.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <symocraft/world/test_scene.h>
#include <symocraft/profiling/profiling.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <limits>
#include <thread>
#include <utility>

namespace SymoCraft::Application {
    using Clock = std::chrono::steady_clock;
    static double Milliseconds(Clock::time_point start, Clock::time_point end = Clock::now()) {
        return std::chrono::duration<double, std::milli>(end - start).count();
    }
    namespace {
        thread_local Detail::AssemblyProbe assembly_probe = nullptr;
        thread_local void* assembly_context = nullptr;
        thread_local Detail::ShutdownProbe shutdown_probe = nullptr;
        thread_local void* shutdown_context = nullptr;
        void Assembled(Detail::AssemblyStage stage) { if (assembly_probe) assembly_probe(assembly_context, stage); }
        void Released(ShutdownStep step) { if (shutdown_probe) shutdown_probe(shutdown_context, step); }
    }
    void Detail::SetAssemblyProbe(AssemblyProbe probe, void* context) noexcept { assembly_probe = probe; assembly_context = context; }
    void Detail::SetShutdownProbe(ShutdownProbe probe, void* context) noexcept { shutdown_probe = probe; shutdown_context = context; }
        struct PreparedWorld {
            std::unique_ptr<World::VoxelWorld> instance;
            TestScene::Pose pose;
            Data::Value report;
        };

        static World::BlockDefinition ReadBlockDefinition()
        {
            try {
                const auto bytes = Assets::ReadBytes(Assets::Resolve("configs/blockFormats.yaml"));
                return World::BlockDefinition::FromConfig(std::string(bytes.begin(), bytes.end()));
            } catch (const std::exception& error) {
                throw std::runtime_error(std::string("Failed to load block configuration: ") + error.what());
            }
        }

        static PreparedWorld PrepareWorld(World::BlockDefinition definition, const StartupOptions& options)
        {
            SYMOCRAFT_PROFILE_ZONE("world.prepare");
            Generation::Settings settings;
            settings.seed = options.seed ? *options.seed :
                options.regression_scene ? TestScene::DefaultSeed : Generation::RandomSeed();
            auto instance = World::VoxelWorld::Create(std::move(definition), settings);
            PreparedWorld world;
            world.instance = std::move(instance);
            world.report["generation"] = world.instance->Describe();
            world.report["seed_source"] = options.seed ? "explicit" : options.regression_scene ? "scene-default" : "random";
            world.report["chunks"] = world.instance->ChunkCount();
            world.report["terrain_digest"] = world.instance->Digest();
            if (options.regression_scene) {
                TestScene::Install(*world.instance);
                world.pose = TestScene::Checkpoint(options.checkpoint);
                world.report["scene"] = TestScene::Describe();
                world.report["scene_digest"] = world.instance->Digest();
            } else {
                world.pose = {"spawn", world.instance->FindSpawn(), -90.0f, 0.0f};
                world.report["scene"]["name"] = "terrain";
                world.report["scene_digest"] = world.report["terrain_digest"].as<std::string>();
            }
            world.report["test_edits_applied"] = options.test_edits;
            if (options.test_edits) TestScene::ApplyEdits(*world.instance);
            world.report["final_digest"] = options.test_edits ? world.instance->Digest() : world.report["scene_digest"].as<std::string>();
            auto& pose = world.report["initial_pose"];
            pose["checkpoint"] = std::string(world.pose.name);
            pose["position"].push_back(world.pose.position.x);
            pose["position"].push_back(world.pose.position.y);
            pose["position"].push_back(world.pose.position.z);
            pose["yaw"] = world.pose.yaw;
            pose["pitch"] = world.pose.pitch;
            pose["fov"] = 45;
            return world;
        }

        void PrintWorldSummary(const StartupOptions& options)
        {
            const auto world = PrepareWorld(ReadBlockDefinition(), options);
            std::cout << Data::DumpYaml(world.report) << std::endl;
        }

        bool CheckAssets(const StartupOptions& options, std::ostream& errors)
        {
            return Detail::CheckRendererAssets(options, errors);
        }

    namespace {
        void RegisterComponents(ECS::Registry& registry) {
            registry.RegisterComponent<Simulation::PhysicsStepOrder>("PhysicsStepOrder");
            registry.RegisterComponent<Transform>("Transform");
            registry.RegisterComponent<Physics::RigidBody>("RigidBody");
            registry.RegisterComponent<Physics::HitBox>("HitBox");
            registry.RegisterComponent<Character::CharacterComponent>("CharacterComponent");
            registry.RegisterComponent<Character::PlayerComponent>("PlayerComponent");
        }
        Simulation::AdvanceInput MapInput(const InputFrame& input) {
            Simulation::AdvanceInput result;
            auto& intent = result.player;
            intent.run = input.Down(Key::LeftShift); intent.sensor = input.Down(Key::CapsLock);
            intent.descend = input.Down(Key::LeftControl); intent.forward = input.Down(Key::W);
            intent.backward = input.Down(Key::S); intent.right = input.Down(Key::D);
            intent.left = input.Down(Key::A); intent.jump = input.Down(Key::Space);
            intent.next_block = input.Down(Key::E); intent.previous_block = input.Down(Key::Q);
            result.place = input.right_button; result.remove = input.left_button;
            return result;
        }
        std::optional<glm::vec3> Selection(const Simulation::AdvanceResult& result) {
            if (!result.selection) return {};
            return glm::vec3(result.selection->cell) + glm::vec3(0.5f);
        }
    }

    struct Application::Impl {
        std::array<std::string, 3> option_strings;
        StartupOptions options;
        Clock::time_point entry;
        std::thread::id owner = std::this_thread::get_id();
        Telemetry::DebugObserver debug;
        Telemetry::DebugBackend debug_backend = Telemetry::DebugBackend::Unknown;
        std::unique_ptr<Platform::Runtime> runtime;
        std::unique_ptr<Window> window;
        std::unique_ptr<Detail::GameRenderer> renderer;
        std::unique_ptr<World::VoxelWorld> world;
        std::unique_ptr<ECS::Registry> registry;
        std::unique_ptr<Simulation::SimulationSession> session;
        std::unique_ptr<Detail::BenchmarkScenario> scenario;
        InputFrame input;
        std::vector<Simulation::PointerInput> pointers = std::vector<Simulation::PointerInput>(0);
        std::optional<Simulation::AdvanceResult> last_result;
        ECS::EntityId player = ECS::null_entity;
        bool initialization_started = false, initialized = false, run_started = false, shutdown_started = false;
        ShutdownReport shutdown;
        explicit Impl(const StartupOptions& value, Clock::time_point start)
            : option_strings{std::string(value.checkpoint), std::string(value.renderer), std::string(value.profile_scenario)},
              options(value), entry(start) {
            options.checkpoint = option_strings[0]; options.renderer = option_strings[1];
            options.profile_scenario = option_strings[2];
        }
        void RequireThread() const {
            if (owner != std::this_thread::get_id()) throw std::logic_error("Application requires its creation thread");
        }
    };

    Application::Application(const StartupOptions& options, Clock::time_point entry)
        : impl_(std::make_unique<Impl>(options, entry)) {}
    Application::~Application() noexcept {
        if (impl_->shutdown_started) return;
        try { Shutdown(); }
        catch (const std::exception& error) { std::fprintf(stderr, "Application destructor cleanup: %s\n", error.what()); }
        catch (...) { std::fprintf(stderr, "Application destructor cleanup: unknown failure\n"); }
    }
    Telemetry::DebugSnapshot Application::DebugValues() const {
        impl_->RequireThread(); return impl_->debug.Snapshot();
    }

    void Application::Initialize() {
        SYMOCRAFT_PROFILE_ZONE("app.initialize");
        auto& state = *impl_;
        state.RequireThread();
        if (state.initialization_started || state.shutdown_started)
            throw std::logic_error("Application Initialize requires a fresh instance");
        state.initialization_started = true;
        const auto& options = state.options;
        if (!options.profile_scenario.empty()) {
            state.scenario = std::make_unique<Detail::BenchmarkScenario>(options.profile_scenario);
        }
        Assembled(Detail::AssemblyStage::DebugState);

        auto definition = ReadBlockDefinition();
        Assembled(Detail::AssemblyStage::Definition);

        const auto window_mode = Detail::SelectWindowMode(options);

        state.runtime = std::make_unique<Platform::Runtime>();
        Assembled(Detail::AssemblyStage::Runtime);
        state.window = Window::Create(*state.runtime, "SymoCraft", state.scenario ? options.width : 0,
            state.scenario ? options.height : 0, state.scenario != nullptr,
            state.scenario != nullptr, window_mode);
        Assembled(Detail::AssemblyStage::Window);
        Detail::AttachRendererContext(*state.window, options);

        state.renderer = std::make_unique<Detail::GameRenderer>(*state.window, options);
        switch (state.renderer->Backend()) {
        case Rendering::Backend::OpenGL: state.debug_backend = Telemetry::DebugBackend::OpenGL; break;
        case Rendering::Backend::D3D12: state.debug_backend = Telemetry::DebugBackend::D3D12; break;
        case Rendering::Backend::Vulkan: state.debug_backend = Telemetry::DebugBackend::Vulkan; break;
        }
        Assembled(Detail::AssemblyStage::Renderer);

        definition.ValidateTextureLayers(state.renderer->LoadAtlas(Assets::Resolve("textures/texture_atlas.png")));
        Assembled(Detail::AssemblyStage::Atlas);
        if (!state.scenario) state.window->SetCursorMode(CursorMode::Lock);
        auto prepared = PrepareWorld(std::move(definition), options);
        state.world = std::move(prepared.instance);
        Assembled(Detail::AssemblyStage::World);
        std::cout << "[world] summary-begin\n" << Data::DumpYaml(prepared.report) << "\n[world] summary-end" << std::endl;
        state.registry = std::make_unique<ECS::Registry>();
        RegisterComponents(*state.registry);
        Assembled(Detail::AssemblyStage::Registry);
        state.session = Simulation::SimulationSession::Create(*state.registry, *state.world,
            {prepared.pose.position, prepared.pose.yaw, prepared.pose.pitch});
        Assembled(Detail::AssemblyStage::Session);
        state.registry->Each<const Character::PlayerComponent>([&](ECS::EntityId entity, const Character::PlayerComponent&) {
            if (state.player != ECS::null_entity) throw std::logic_error("Application requires one assembled player");
            state.player = entity;
        });
        const auto& transform = state.registry->GetComponent<Transform>(state.player);
        state.last_result = Simulation::AdvanceResult{
            {transform.position, transform.yaw, transform.pitch, transform.front, transform.up}, {}, {}, {}, {}};
        state.debug.Observe(Clock::now(), false, 0,
            std::array<double, 3>{transform.position.x, transform.position.y, transform.position.z}, state.debug_backend);
        SYMOCRAFT_PROFILE_APP_INFO(state.renderer->ProfilingIdentity() +
            "; seed=" + std::to_string(prepared.report["generation"]["seed"].as<std::uint32_t>()) +
            "; scene=" + (options.regression_scene ? "regression-v1" : "terrain") +
            "; workload=" + std::string(options.profile_scenario.empty() ? "ordinary" : options.profile_scenario));
        if (!options.profile_scenario.empty())
            std::cout << "[scenario] initial; checkpoint=" << options.checkpoint
                      << "; position=" << transform.position.x << ',' << transform.position.y << ',' << transform.position.z
                      << "; yaw=" << transform.yaw << "; pitch=" << transform.pitch
                      << "; fov=" << state.last_result->lens.fov << std::endl;

        state.world->RebuildDirtyMeshes();

        if (state.renderer->Modern()) state.renderer->Synchronize(*state.world);
        Assembled(Detail::AssemblyStage::Meshes);
        state.initialized = true;
    }

    void Application::Run() {
        SYMOCRAFT_PROFILE_ZONE("app.run");
        auto& state = *impl_;
        state.RequireThread();
        if (!state.initialized || state.run_started || state.shutdown_started)
            throw std::logic_error("Application Run requires a fresh successful assembly");
        state.run_started = true;
        auto& window = *state.window;
        auto& world = *state.world;
        const auto& options = state.options;
        double previous_frame_time = Window::Time();
        bool paused = false, paused_minimized = false;
        unsigned rendered_frames = 0;
        std::uint64_t next_frame_id = 1;
        std::cout << "[runtime] ready; chunks=" << world.ChunkCount()
                  << "; loading_ms=" << Milliseconds(state.entry)
                  << "; spawn=" << state.last_result->pose.position.x << ',' << state.last_result->pose.position.y
                  << ',' << state.last_result->pose.position.z << std::endl;
        const auto scenario_start = Clock::now();
        bool scenario_completed = false;
        while (!window.ShouldClose()) {
            SYMOCRAFT_PROFILE_ZONE("app.frame");

            { SYMOCRAFT_PROFILE_ZONE("app.events"); window.PollInt(); }

            if (window.ShouldClose()) break;
            const double current_frame_time = Window::Time();
            const bool non_drawable = window.Minimized() || window.Width() <= 0 || window.Height() <= 0;
            const bool inactive = !window.Focused() || non_drawable;
            const double elapsed = std::chrono::duration<double>(Clock::now() - scenario_start).count();
            if (!options.profile_scenario.empty()) {
                SYMOCRAFT_PROFILE_ZONE("app.profile_window_input");
                window.CaptureInput(state.input);
                if (state.input.Down(Key::Escape)) { window.Close(); state.input.Clear(); break; }
                if (non_drawable || window.Width() != static_cast<int>(options.width) ||
                    window.Height() != static_cast<int>(options.height))
                    throw std::runtime_error("Profiling scenario framebuffer changed or was minimized");
                if (elapsed >= options.scenario_seconds) {
                    scenario_completed = true; window.Close(); break;
                }
            }
            state.renderer->Resize();

            if (options.profile_scenario.empty()) {
                SYMOCRAFT_PROFILE_ZONE("app.input"); window.CaptureInput(state.input);
            }
            if (!state.scenario && (non_drawable || (inactive && options.frame_limit == 0))) {
                if (!paused || paused_minimized != window.Minimized())
                    std::cout << "[runtime] suspended; minimized=" << window.Minimized() << "; extent="
                              << window.Width() << ',' << window.Height() << "; rendered_frames=" << rendered_frames << std::endl;
                paused_minimized = window.Minimized();
                state.input.Clear(); state.pointers.clear(); window.ResetInput();
                state.session->Pause(); paused = true;
                state.debug.Pause();
                previous_frame_time = current_frame_time;
                window.WaitEvents(0.05); continue;
            }
            if (!state.scenario && (paused || state.input.focus_lost)) {
                state.debug.Pause();
                state.input.Clear(); state.pointers.clear(); window.ResetInput();
                state.session->Pause(); state.session->Resume();
                window.CaptureInput(state.input);
                std::cout << "[runtime] resumed; extent=" << window.Width() << ',' << window.Height()
                          << "; rendered_frames=" << rendered_frames << std::endl;
            }
            paused = false;
            const double delta = current_frame_time - previous_frame_time;
            previous_frame_time = current_frame_time;
            if (state.input.Down(Key::Escape)) { window.Close(); state.input.Clear(); break; }
            Simulation::AdvanceInput intent;
            if (state.scenario) {
                const auto scenario = state.scenario->BeforeAdvance(elapsed, state.last_result->pose, world);
                intent = scenario.input;

                if (scenario.pose_override) {
                    auto& transform = state.registry->GetComponent<Transform>(state.player);
                    transform.position = scenario.pose_override->position;
                    transform.yaw = scenario.pose_override->yaw;
                    transform.pitch = scenario.pose_override->pitch;
                }
            } else {
                intent = MapInput(state.input);
                state.pointers.reserve(state.input.pointer_events.size());
                for (const auto& event : state.input.pointer_events)
                    state.pointers.push_back({event.mouse_dx, event.mouse_dy, event.scroll_y});
            }
            state.last_result = state.session->Advance(delta, intent, state.pointers);
            const auto& result = *state.last_result;
            SYMOCRAFT_PROFILE_PLOT("physics.steps [per frame]", result.statistics.physical_steps);
            if (result.edit) world.TryEdit(*result.edit); // Ordinary edits remain after the advance.

            if (result.statistics.recovered) std::cout << "[runtime] returned player to safe spawn" << std::endl;
            state.input.Clear(); state.pointers.clear(); // Every consumer has finished; retain capacities.

            [[maybe_unused]] const auto rebuilt_chunks = world.RebuildDirtyMeshes();
            SYMOCRAFT_PROFILE_PLOT("mesh.rebuilt [per frame]", rebuilt_chunks);

            [[maybe_unused]] const auto upload = state.renderer->Synchronize(world);
            SYMOCRAFT_PROFILE_PLOT("mesh.upload [bytes per frame]", upload.bytes);

            if (next_frame_id == (std::numeric_limits<std::uint64_t>::max)())
                throw std::overflow_error("game frame identity exhausted");
            const auto view = Simulation::DeriveView(result.pose, result.lens, window.GetAspectRatio());
            const auto frame = state.renderer->Render(next_frame_id++, view, Selection(result));
            const auto present_end = Clock::now();
            if (state.renderer->Modern()) {
                SYMOCRAFT_PROFILE_PLOT("renderer.draw [calls per frame]", frame.result.statistics.draw_calls);
                SYMOCRAFT_PROFILE_PLOT("renderer.upload [bytes per frame]", frame.result.statistics.upload_bytes);
            }
            state.debug.Observe(present_end, frame.result.status == Rendering::FrameStatus::Presented,
                result.statistics.physical_steps,
                std::array<double, 3>{result.pose.position.x, result.pose.position.y, result.pose.position.z}, state.debug_backend);

            ++rendered_frames;
            SYMOCRAFT_PROFILE_FRAME();
            if (options.frame_limit != 0 && rendered_frames >= options.frame_limit) window.Close();
        }
        const auto position = state.last_result->pose.position;
        if (!options.profile_scenario.empty())
            std::cout << "[scenario] " << (scenario_completed ? "completed" : "cancelled")
                      << "; name=" << options.profile_scenario << "; edits=" << state.scenario->AppliedEdits() << std::endl;
        std::cout << "[runtime] loop finished; rendered_frames=" << rendered_frames
                  << "; player=" << position.x << ',' << position.y << ',' << position.z << std::endl;
    }

    ShutdownReport Application::Shutdown() {
        SYMOCRAFT_PROFILE_ZONE("app.shutdown");
        auto& state = *impl_;
        state.RequireThread();
        if (state.shutdown_started) {
            if (!state.shutdown.Succeeded()) throw ShutdownError(state.shutdown);
            return state.shutdown;
        }
        state.shutdown_started = true;
        state.debug.Pause();
        auto& report = state.shutdown;
        const auto release = [&](ShutdownStep step, bool acquired, auto&& action) {
            report.Attempt(step, acquired, [&] { action(); Released(step); });
        };
        state.scenario.reset();
        if (state.renderer) {
            ShutdownReport renderer_report;
            try { renderer_report = state.renderer->Shutdown(); }
            catch (const ShutdownError& error) { renderer_report = error.Report(); }
            catch (...) {
                auto& item = renderer_report.items[static_cast<std::size_t>(ShutdownStep::Renderer)];
                item.fact = ReleaseFact::Failed; item.error = std::current_exception();
            }
            for (auto step : {ShutdownStep::MeshBindings, ShutdownStep::Texture, ShutdownStep::Renderer})
                report.items[static_cast<std::size_t>(step)] = renderer_report.items[static_cast<std::size_t>(step)];
        }
        release(ShutdownStep::Simulation, state.session != nullptr, [&] { state.session.reset(); });
        release(ShutdownStep::World, state.world != nullptr, [&] { state.world.reset(); });
        release(ShutdownStep::Registry, state.registry != nullptr, [&] { state.registry.reset(); });
        release(ShutdownStep::RendererObject, state.renderer != nullptr, [&] { state.renderer.reset(); });
        release(ShutdownStep::Window, state.window != nullptr, [&] { state.window->Destroy(); });
        state.window.reset();
        release(ShutdownStep::Runtime, state.runtime != nullptr, [&] { state.runtime->Shutdown(); });
        state.runtime.reset();
        if (!report.Succeeded()) throw ShutdownError(report);
        return report;
    }
}
