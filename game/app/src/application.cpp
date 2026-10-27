#include "application.h"
#include "application_probe.h"
#include "benchmark_scenario.h"
#include "game_renderer.h"
#include "foreground_readiness.h"
#include "startup_options.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/telemetry/document_io.h>
#include <symocraft/world/benchmark_workload.h>
#include <symocraft/platform/window.h>
#include <symocraft/platform/process_memory.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <symocraft/world/test_scene.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <iostream>
#include <limits>
#include <thread>
#include <utility>

namespace SymoCraft::Application {
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

        static PreparedWorld PrepareWorld(World::BlockDefinition definition, const StartupOptions& options,
                                           Performance::Session* performance = nullptr)
        {
            Generation::Settings settings;
            settings.seed = options.seed ? *options.seed :
                options.regression_scene ? TestScene::DefaultSeed : Generation::RandomSeed();
            Generation::Timings timings;
            auto instance = World::VoxelWorld::Create(std::move(definition), settings, performance ? &timings : nullptr);
            const auto report_start = Performance::Clock::now();
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
            if (performance) {
                performance->SetStartup("world_allocation", timings.allocation_ms);
                performance->SetStartup("terrain", timings.terrain_ms);
                performance->SetStartup("vegetation", timings.vegetation_ms);
                performance->SetStartup("fixtures_digests_and_metadata", Performance::Milliseconds(report_start));
                performance->SetMetadata("world", world.report);
            }
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
        std::array<std::string, 5> option_strings;
        StartupOptions options;
        Performance::Clock::time_point entry;
        std::thread::id owner = std::this_thread::get_id();
        std::unique_ptr<Performance::Session> performance;
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
        explicit Impl(const StartupOptions& value, Performance::Clock::time_point start)
            : option_strings{std::string(value.checkpoint), std::string(value.benchmark), std::string(value.output_directory),
                             std::string(value.focus_policy), std::string(value.renderer)}, options(value), entry(start) {
            options.checkpoint = option_strings[0]; options.benchmark = option_strings[1];
            options.output_directory = option_strings[2]; options.focus_policy = option_strings[3]; options.renderer = option_strings[4];
        }
        void RequireThread() const {
            if (owner != std::this_thread::get_id()) throw std::logic_error("Application requires its creation thread");
        }
    };

    Application::Application(const StartupOptions& options, Performance::Clock::time_point entry)
        : impl_(std::make_unique<Impl>(options, entry)) {}
    Application::~Application() noexcept {
        if (impl_->shutdown_started) return;
        try { Shutdown(); }
        catch (const std::exception& error) { std::fprintf(stderr, "Application destructor cleanup: %s\n", error.what()); }
        catch (...) { std::fprintf(stderr, "Application destructor cleanup: unknown failure\n"); }
    }
    Performance::Session* Application::Observation() noexcept { return impl_->performance.get(); }

    void Application::Initialize() {
        auto& state = *impl_;
        state.RequireThread();
        if (state.initialization_started || state.shutdown_started)
            throw std::logic_error("Application Initialize requires a fresh instance");
        state.initialization_started = true;
        const auto& options = state.options;
        if (!options.benchmark.empty()) {
            Performance::SessionConfig config;
            config.output_directory = options.output_directory;
            config.focus_policy = options.focus_policy; config.benchmark = options.benchmark;
            config.width = options.width; config.height = options.height;
            config.warmup_seconds = options.warmup_seconds; config.sample_seconds = options.sample_seconds;
            config.vsync = options.vsync;
            state.performance = std::make_unique<Performance::Session>(config, state.entry);
            state.performance->SetStartup("argument_preflight_and_collector_setup", Performance::Milliseconds(state.entry));
            state.scenario = std::make_unique<Detail::BenchmarkScenario>(options.benchmark);
        }
        auto* performance = state.performance.get();
        Assembled(Detail::AssemblyStage::Observation);
        const auto config_start = Performance::Clock::now();
        auto definition = ReadBlockDefinition();
        Assembled(Detail::AssemblyStage::Definition);
        const double config_ms = Performance::Milliseconds(config_start);
        const auto window_mode = Detail::SelectWindowMode(options);
        const auto window_start = Performance::Clock::now();
        state.runtime = std::make_unique<Platform::Runtime>();
        Assembled(Detail::AssemblyStage::Runtime);
        state.window = Window::Create(*state.runtime, "SymoCraft", state.scenario ? options.width : 0,
            state.scenario ? options.height : 0, state.scenario != nullptr,
            state.scenario && options.focus_policy == "allow-unfocused", window_mode);
        Assembled(Detail::AssemblyStage::Window);
        Detail::AttachRendererContext(*state.window, options);
        if (performance) {
            performance->SetStartup(options.renderer == "opengl" ? "window_and_context" : "window", Performance::Milliseconds(window_start));
            performance->SetMetadata("framebuffer_width", state.window->Width());
            performance->SetMetadata("framebuffer_height", state.window->Height());
            performance->SetMetadata("window_mode", "borderless-windowed");
            performance->SetMetadata("taskbar_policy", "preserve-shell-z-order");
            if (state.window->Width() != static_cast<int>(options.width) || state.window->Height() != static_cast<int>(options.height))
                throw std::runtime_error("Actual framebuffer does not match requested benchmark resolution");
        }
        const auto renderer_start = Performance::Clock::now();
        state.renderer = std::make_unique<Detail::GameRenderer>(*state.window, options, performance != nullptr);
        Assembled(Detail::AssemblyStage::Renderer);
        if (performance) {
            performance->SetStartup("shaders_buffers_and_block_config", Performance::Milliseconds(renderer_start) + config_ms);
            state.renderer->DescribeSession(*performance);
        }
        const auto texture_start = Performance::Clock::now();
        definition.ValidateTextureLayers(state.renderer->LoadAtlas(Assets::Resolve("textures/texture_atlas.png")));
        Assembled(Detail::AssemblyStage::Atlas);
        if (performance) {
            performance->SetStartup("texture_decode_and_upload", Performance::Milliseconds(texture_start));
            if (state.renderer->Modern()) {
                const auto upload = state.renderer->AtlasUpload();
                performance->AddStartupUpload({upload.cpu_ms, upload.bytes, upload.calls});
            }
        }
        if (!state.scenario) state.window->SetCursorMode(CursorMode::Lock);
        auto prepared = PrepareWorld(std::move(definition), options, performance);
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
        const auto mesh_start = Performance::Clock::now();
        const auto count = state.world->RebuildDirtyMeshes();
        const auto mesh_ms = Performance::Milliseconds(mesh_start);
        Detail::MeshUpload initial_upload;
        if (state.renderer->Modern()) initial_upload = state.renderer->Synchronize(*state.world);
        if (performance) {
            performance->SetStartup("first_mesh", mesh_ms);
            performance->SetMetadata("first_mesh_chunks", count);
            if (state.renderer->Modern()) {
                performance->AddStartupUpload({initial_upload.cpu_ms, initial_upload.bytes, initial_upload.calls});
                performance->SetStartup("first_upload_cpu", initial_upload.cpu_ms);
                performance->SetMetadata("first_upload_bytes", initial_upload.bytes);
                performance->SetMetadata("camera_contract", "body pose + owned lens; degrees to radians; RH, near 0.1, far 2000; world-space identity model");
                performance->SetMetadata("atlas_contract", "512x512 RGBA8; 64 bottom-row-first layers of 64x64 top-left texels; mesh V=1-V exactly once; Linear");
            }
            performance->SetStartup("ready_from_main_entry", Performance::Milliseconds(performance->Entry()));
            performance->SetMetadata("edit_interval_seconds", Benchmark::EditInterval);
            performance->SetMetadata("edit_plan", "M2-T2 forward 16 writes, then reverse 16 writes; repeat");
            performance->SetMetadata("walk_plan", "real physics, x=-20.5..20.5, z=-6.5, speed=4.4, endpoint reversal; no input replay");
        }
        Assembled(Detail::AssemblyStage::Meshes);
        state.initialized = true;
    }

    void Application::Run() {
        auto& state = *impl_;
        state.RequireThread();
        if (!state.initialized || state.run_started || state.shutdown_started)
            throw std::logic_error("Application Run requires a fresh successful assembly");
        state.run_started = true;
        auto& window = *state.window;
        auto& world = *state.world;
        auto* performance = state.performance.get();
        const auto& options = state.options;
        if (options.foreground_ready_timeout_seconds && (!performance || options.focus_policy != "strict" ||
                                                        options.foreground_ready_timeout_seconds > 120))
            throw std::invalid_argument("Foreground readiness requires a strict benchmark and a bounded timeout");
        double previous_frame_time = Window::Time();
        bool paused = false, paused_minimized = false, sampling_announced = false;
        unsigned rendered_frames = 0;
        std::uint64_t next_frame_id = 1;
        double next_memory_time = 0;
        std::cout << "[runtime] ready; chunks=" << world.ChunkCount()
                  << "; loading_ms=" << Performance::Milliseconds(state.entry)
                  << "; spawn=" << state.last_result->pose.position.x << ',' << state.last_result->pose.position.y
                  << ',' << state.last_result->pose.position.z << std::endl;
        if (options.foreground_ready_timeout_seconds) {
            const auto preparation_start = Performance::Clock::now();
            Detail::ForegroundReadiness readiness(preparation_start,
                std::chrono::seconds(options.foreground_ready_timeout_seconds));
            performance->SetMetadata("foreground_ready_timeout_seconds", options.foreground_ready_timeout_seconds);
            performance->SetMetadata("foreground_ready_stable_seconds", 1);
            performance->Status("awaiting-foreground");
            std::cout << "[benchmark] awaiting-foreground; timeout_seconds="
                      << options.foreground_ready_timeout_seconds << "; stable_seconds=1" << std::endl;
            state.session->Pause();
            for (;;) {
                window.PollInt();
                window.CaptureInput(state.input);
                if (window.ShouldClose() || state.input.Down(Key::Escape)) {
                    performance->Invalidate("foreground-readiness-cancelled");
                    throw std::runtime_error("Foreground readiness cancelled before warmup");
                }
                const auto readiness_state = readiness.Observe(Performance::Clock::now(),
                    !state.input.focus_lost && window.Focused() && !window.Minimized() &&
                    window.Width() == static_cast<int>(options.width) && window.Height() == static_cast<int>(options.height));
                state.input.Clear();
                if (readiness_state == Detail::ForegroundReadiness::State::Ready) break;
                if (readiness_state == Detail::ForegroundReadiness::State::TimedOut) {
                    performance->Invalidate("foreground-readiness-timeout");
                    throw std::runtime_error("Foreground readiness timed out before warmup");
                }
                window.WaitEvents(.02);
            }
            performance->SetStartup("foreground_preparation", Performance::Milliseconds(preparation_start));
            window.ResetInput();
            state.session->Resume();
            previous_frame_time = Window::Time();
            std::cout << "[benchmark] foreground-ready; preparation_ms="
                      << Performance::Milliseconds(preparation_start) << std::endl;
        }
        if (performance) performance->Status("warmup");
        const auto benchmark_start = Performance::Clock::now();
        auto previous_present = benchmark_start;
        while (!window.ShouldClose()) {
            const auto frame_start = Performance::Clock::now();
            Performance::Frame sample;
            sample.elapsed = Performance::Milliseconds(benchmark_start, frame_start) / 1000.0;
            if (performance && sample.elapsed >= options.warmup_seconds + options.sample_seconds) {
                performance->Complete(); window.Close(); break;
            }
            sample.measured = sample.elapsed >= options.warmup_seconds;
            if (performance && sample.measured && !sampling_announced) {
                performance->Status("sampling", sample.elapsed); sampling_announced = true;
            }
            window.PollInt();
            sample.event_ms = Performance::Milliseconds(frame_start);
            if (window.ShouldClose()) break;
            const double current_frame_time = Window::Time();
            const bool non_drawable = window.Minimized() || window.Width() <= 0 || window.Height() <= 0;
            const bool inactive = !window.Focused() || non_drawable;
            state.renderer->Resize();
            sample.focused = !inactive;
            if (performance && (window.Width() != static_cast<int>(options.width) ||
                window.Height() != static_cast<int>(options.height) || window.Minimized())) {
                performance->Invalidate("framebuffer-changed-or-minimized"); break;
            }
            const auto simulation_start = Performance::Clock::now();
            window.CaptureInput(state.input);
            if (!state.scenario && (non_drawable || (inactive && options.frame_limit == 0))) {
                if (!paused || paused_minimized != window.Minimized())
                    std::cout << "[runtime] suspended; minimized=" << window.Minimized() << "; extent="
                              << window.Width() << ',' << window.Height() << "; rendered_frames=" << rendered_frames << std::endl;
                paused_minimized = window.Minimized();
                state.input.Clear(); state.pointers.clear(); window.ResetInput();
                state.session->Pause(); paused = true;
                previous_frame_time = current_frame_time;
                window.WaitEvents(0.05); continue;
            }
            if (!state.scenario && (paused || state.input.focus_lost)) {
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
                const auto scenario = state.scenario->BeforeAdvance(sample.elapsed, state.last_result->pose, world);
                intent = scenario.input;
                sample.edits = scenario.edits; sample.edit_lateness_ms = scenario.lateness_ms;
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
            if (result.edit) world.TryEdit(*result.edit); // Ordinary edits remain after the advance.
            sample.simulation_delta_ms = result.statistics.rule_seconds * 1000.0;
            if (result.statistics.recovered) std::cout << "[runtime] returned player to safe spawn" << std::endl;
            state.input.Clear(); state.pointers.clear(); // Every consumer has finished; retain capacities.
            sample.simulation_ms = Performance::Milliseconds(simulation_start);
            const auto mesh_start = Performance::Clock::now();
            sample.rebuilt_chunks = world.RebuildDirtyMeshes();
            sample.mesh_ms = Performance::Milliseconds(mesh_start);
            const auto pack_start = Performance::Clock::now();
            const auto upload = state.renderer->Synchronize(world);
            sample.pack_ms = Performance::Milliseconds(pack_start);
            if (performance) {
                const auto instrumentation_start = Performance::Clock::now();
                state.renderer->PollGpu(*performance);
                if (sample.elapsed >= next_memory_time) {
                    const auto process = Platform::SampleProcessMemory();
                    Performance::Memory memory;
                    memory.elapsed = sample.elapsed; memory.working_set_bytes = process.working_set_bytes;
                    memory.private_bytes = process.private_bytes;
                    state.renderer->SampleDeviceMemory(memory);
                    performance->Add(memory); next_memory_time = sample.elapsed + 1.0;
                }
                sample.instrumentation_ms = Performance::Milliseconds(instrumentation_start);
            }
            if (next_frame_id == (std::numeric_limits<std::uint64_t>::max)())
                throw std::overflow_error("game frame identity exhausted");
            const auto view = Simulation::DeriveView(result.pose, result.lens, window.GetAspectRatio());
            const auto frame = state.renderer->Render(next_frame_id++, view, Selection(result), performance);
            const auto present_end = Performance::Clock::now();
            sample.render_cpu_ms = frame.render_cpu_ms; sample.present_ms = frame.present_ms;
            sample.frame_ms = Performance::Milliseconds(previous_present, present_end); previous_present = present_end;
            if (performance) {
                const auto& stats = frame.result.statistics;
                sample.vertices = stats.drawn_vertices; sample.upload_bytes = stats.upload_bytes;
                sample.draw_calls = stats.draw_calls; sample.upload_cpu_ms = frame.upload_cpu_ms;
                if (state.renderer->Modern()) {
                    sample.frame_id = frame.result.frame_id;
                    sample.cpu_prepare_ms = std::chrono::duration<double, std::milli>(stats.cpu_prepare).count();
                    sample.cpu_submit_ms = std::chrono::duration<double, std::milli>(stats.cpu_submit).count();
                    sample.resource_upload_bytes = upload.bytes; sample.render_upload_bytes = stats.upload_bytes;
                    sample.upload_calls = upload.calls; sample.upload_cpu_ms = upload.cpu_ms;
                    if (upload.bytes > (std::numeric_limits<std::uint64_t>::max)() - stats.upload_bytes)
                        throw std::overflow_error("frame upload byte count exhausted");
                    sample.upload_bytes += upload.bytes;
                    if (frame.result.status != Rendering::FrameStatus::Presented) performance->Invalidate("renderer-frame-skipped");
                }
                sample.x = result.pose.position.x; sample.y = result.pose.position.y;
                sample.z = result.pose.position.z; sample.yaw = result.pose.yaw;
                performance->Add(sample);
                if (rendered_frames == 0) {
                    if (!state.renderer->Modern()) {
                        performance->SetStartup("first_upload_cpu", frame.upload_cpu_ms);
                        performance->SetMetadata("first_upload_bytes", stats.upload_bytes);
                    }
                    performance->SetStartup("first_frame_swap_return_from_main_entry", Performance::Milliseconds(performance->Entry(), present_end));
                }
            }
            ++rendered_frames;
            if (options.frame_limit != 0 && rendered_frames >= options.frame_limit) window.Close();
        }
        if (performance) {
            state.renderer->PollGpu(*performance);
            performance->SetMetadata("final_world_digest", world.Digest());
            performance->SetMetadata("total_scheduled_edits_applied", state.scenario ? state.scenario->AppliedEdits() : 0);
            if (performance->Completed()) {
                state.renderer->Synchronize(world);
                const auto view = Simulation::DeriveView(state.last_result->pose, state.last_result->lens, window.GetAspectRatio());
                state.renderer->Capture(next_frame_id++, view, Selection(*state.last_result),
                    performance->Directory() / "final-frame.png", *performance);
                performance->SetMetadata("screenshot", "final-frame.png; extra render/readback after measurement");
            }
        }
        const auto position = state.last_result->pose.position;
        std::cout << "[runtime] loop finished; rendered_frames=" << rendered_frames
                  << "; player=" << position.x << ',' << position.y << ',' << position.z << std::endl;
    }

    ShutdownReport Application::Shutdown() {
        auto& state = *impl_;
        state.RequireThread();
        if (state.shutdown_started) {
            if (!state.shutdown.Succeeded()) throw ShutdownError(state.shutdown);
            return state.shutdown;
        }
        state.shutdown_started = true;
        auto& report = state.shutdown;
        const auto release = [&](ShutdownStep step, bool acquired, auto&& action) {
            report.Attempt(step, acquired, [&] { action(); Released(step); });
        };
        state.scenario.reset();
        if (state.renderer && state.performance) {
            if (state.renderer->CanCollectGpu())
                report.Attempt(ShutdownStep::GpuSamples, true, [&] { state.renderer->PollGpu(*state.performance); });
            else report.Unavailable(ShutdownStep::GpuSamples);
        }
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
