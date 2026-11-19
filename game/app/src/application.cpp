#include "application.h"
#include "application_probe.h"
#include "benchmark_scenario.h"
#include "game_renderer.h"
#include "demo_control.h"
#include "demo_overlay.h"
#include "startup_options.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/foundation/document_io.h>
#include <symocraft/world/benchmark_workload.h>
#include <symocraft/platform/window.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/session.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/npc_ai.h>
#include <symocraft/world/test_scene.h>
#include <symocraft/profiling/profiling.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cmath>
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
        thread_local Detail::DemoProbe demo_probe = nullptr;
        thread_local void* demo_context = nullptr;
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

        static World::BlockDefinition ReadBlockDefinition(bool npc_scene = false)
        {
            try {
                const auto bytes = Assets::ReadBytes(Assets::Resolve("configs/blockFormats.yaml"));
                std::string text(bytes.begin(), bytes.end());
                if (npc_scene) {
                    const auto extra = Assets::ReadBytes(Assets::Resolve("configs/npc_blocks.yaml"));
                    text += '\n';
                    text.append(extra.begin(), extra.end());
                }
                return World::BlockDefinition::FromConfig(text);
            } catch (const std::exception& error) {
                throw std::runtime_error(std::string("Failed to load block configuration: ") + error.what());
            }
        }

        static glm::vec3 ScenePosition(const Data::Value& value)
        {
            if (!value.IsSequence() || value.size() != 3) throw std::invalid_argument("NPC position must contain three coordinates");
            glm::vec3 result{value[0].as<float>(), value[1].as<float>(), value[2].as<float>()};
            for (int axis = 0; axis < 3; ++axis)
                if (!std::isfinite(result[axis])) throw std::invalid_argument("NPC position is non-finite");
            return result;
        }
        static World::BlockCoord SceneCell(const Data::Value& value)
        {
            const auto position = ScenePosition(value);
            const auto cell = World::TryToBlockCoord(position);
            if (!cell || glm::vec3(*cell) != position) throw std::invalid_argument("NPC target must be an integer block coordinate");
            return *cell;
        }
        static void InstallNpcTerrain(World::VoxelWorld& world, const Data::Value& scene)
        {
            const int y = scene["floor_y"].as<int>();
            const int radius = scene["floor_radius"].as<int>();
            if (y < 0 || y > 250 || radius < 1 || radius > 16)
                throw std::invalid_argument("NPC scene floor bounds are invalid");
            for (int x = -radius; x <= radius; ++x) {
                for (int z = -radius; z <= radius; ++z) {
                    world.TryEdit({World::EditOperation::Set, {x, y, z}, 5});
                    for (int height = y + 1; height <= y + 4; ++height)
                        world.TryEdit({World::EditOperation::Remove, {x, height, z}});
                }
            }
            const auto& targets = scene["targets"];
            if (!targets.IsSequence() || targets.size() != 2) throw std::invalid_argument("NPC scene requires two targets");
            for (std::size_t i = 0; i < targets.size(); ++i) {
                if (!world.TryEdit({World::EditOperation::Set, SceneCell(targets[i]), 12}).Accepted())
                    throw std::invalid_argument("NPC target lies outside the configured world");
            }
        }
        static PreparedWorld PrepareWorld(World::BlockDefinition definition, const StartupOptions& options,
                                           const Data::Value* npc_scene = nullptr)
        {
            SYMOCRAFT_PROFILE_ZONE("world.prepare");
            Generation::Settings settings;
            settings.seed = options.seed ? *options.seed :
                options.regression_scene ? TestScene::DefaultSeed : Generation::RandomSeed();
            auto instance = World::VoxelWorld::Create(std::move(definition), settings, nullptr, npc_scene != nullptr);
            PreparedWorld world;
            world.instance = std::move(instance);
            world.report["generation"] = world.instance->Describe();
            world.report["seed_source"] = options.seed ? "explicit" : options.regression_scene ? "scene-default" : "random";
            world.report["chunks"] = world.instance->ChunkCount();
            world.report["terrain_digest"] = world.instance->Digest();
            if (npc_scene) {
                InstallNpcTerrain(*world.instance, *npc_scene);
                world.pose = {"npc-mvp", ScenePosition((*npc_scene)["player_position"]), -90.0f, -15.0f};
                if (options.npc_scene == "demo") {
                    const auto& observer = (*npc_scene)["demo"];
                    world.pose = {"npc-demo", ScenePosition(observer["observer_position"]),
                        observer["observer_yaw"].as<float>(), observer["observer_pitch"].as<float>()};
                }
                world.report["scene"]["name"] = std::string(options.npc_scene);
                world.report["scene_digest"] = world.instance->Digest();
            } else if (options.regression_scene) {
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

    struct RunContext {
        // Destruction is reverse declaration order: brains -> session -> registry -> World.
        std::unique_ptr<World::VoxelWorld> world;
        std::unique_ptr<ECS::Registry> registry;
        std::unique_ptr<Simulation::SimulationSession> session;
        std::vector<std::unique_ptr<Simulation::NpcBrain>> brains;
        std::optional<Simulation::AdvanceResult> last_result;
        Simulation::PlayerPose observer;
        ECS::EntityId player = ECS::null_entity;
    };

    void Detail::SetDemoProbe(DemoProbe probe, void* context) noexcept { demo_probe = probe; demo_context = context; }

    struct Application::Impl {
        std::array<std::string, 4> option_strings;
        StartupOptions options;
        Clock::time_point entry;
        std::thread::id owner = std::this_thread::get_id();
        Telemetry::DebugObserver debug;
        Telemetry::DebugBackend debug_backend = Telemetry::DebugBackend::Unknown;
        std::unique_ptr<Platform::Runtime> runtime;
        std::unique_ptr<Window> window;
        std::unique_ptr<Detail::GameRenderer> renderer;
        std::unique_ptr<RunContext> run = std::make_unique<RunContext>();
        std::optional<World::BlockDefinition> npc_definition;
        std::vector<std::string> npc_tree_sources;
        Detail::DemoControl demo;
        Data::Value npc_config;
        std::unique_ptr<Detail::BenchmarkScenario> scenario;
        InputFrame input;
        std::vector<Simulation::PointerInput> pointers = std::vector<Simulation::PointerInput>(0);
        bool initialization_started = false, initialized = false, run_started = false, shutdown_started = false;
        ShutdownReport shutdown;
        explicit Impl(const StartupOptions& value, Clock::time_point start)
            : option_strings{std::string(value.checkpoint), std::string(value.renderer), std::string(value.profile_scenario), std::string(value.npc_scene)},
              options(value), entry(start) {
            options.checkpoint = option_strings[0]; options.renderer = option_strings[1];
            options.profile_scenario = option_strings[2];
            options.npc_scene = option_strings[3];
        }
        bool DemoMode() const noexcept { return options.npc_scene == "demo"; }
        void AssembleRun(PreparedWorld& prepared) {
            run->world = std::move(prepared.instance);
            Assembled(Detail::AssemblyStage::World);
            std::cout << "[world] summary-begin\n" << Data::DumpYaml(prepared.report) << "\n[world] summary-end" << std::endl;
            run->registry = std::make_unique<ECS::Registry>();
            RegisterComponents(*run->registry);
            Assembled(Detail::AssemblyStage::Registry);
            run->session = Simulation::SimulationSession::Create(*run->registry, *run->world,
                {prepared.pose.position, prepared.pose.yaw, prepared.pose.pitch});
            Assembled(Detail::AssemblyStage::Session);
            if (npc_config.IsDefined()) {
                const auto& npcs = npc_config["npcs"];
                if (!npcs.IsSequence() || npcs.size() != 3) throw std::invalid_argument("NPC scene requires three entities");
                for (std::size_t i = 0; i < npcs.size(); ++i) {
                    if (DemoMode() && !demo.Contest() && i == 1) continue;
                    const auto& item = npcs[i];
                    const auto kind = item["kind"].as<std::string>();
                    if (kind != "human" && kind != "zombie") throw std::invalid_argument("Unknown NPC kind");
                    Simulation::NpcConfig config;
                    config.kind = kind == "human" ? Simulation::NpcKind::Human : Simulation::NpcKind::Zombie;
                    config.position = ScenePosition(item["position"]);
                    if (DemoMode() && config.kind == Simulation::NpcKind::Zombie)
                        config.position = ScenePosition(npc_config["demo"]["zombie_position"]);
                    config.order_key = item["order"].as<std::uint64_t>();
                    const auto entity = run->session->AddNpc(config);
                    const auto target_index = (options.npc_scene == "contest" || (DemoMode() && demo.Contest())) ? std::size_t{0} : item["target"].as<std::size_t>();
                    const auto target = SceneCell(npc_config["targets"][target_index]);
                    run->brains.push_back(Simulation::NpcBrain::FromText(npc_tree_sources.at(i),
                        item["tree_id"].as<std::string>(), entity, target, Simulation::SessionNpcGateway(*run->session, entity)));
                }
            }
            run->registry->Each<const Character::PlayerComponent>([&](ECS::EntityId entity, const Character::PlayerComponent&) {
                if (run->player != ECS::null_entity) throw std::logic_error("Application requires one assembled player");
                run->player = entity;
            });
            const auto& transform = run->registry->GetComponent<Transform>(run->player);
            run->last_result = Simulation::AdvanceResult{
                {transform.position, transform.yaw, transform.pitch, transform.front, transform.up}, {}, {}, {}, {}};
            run->observer = run->last_result->pose;
            if (DemoMode()) run->session->Pause();
        }
        void RestartDemo(bool change_scene) {
            // Called between frames, after every previous frame batch was committed.
            renderer->SetCharacterInstances({});
            renderer->ClearDemoOverlay();
            if (run && run->world) renderer->ReleaseWorld(run->world->Identity());
            auto fresh = std::make_unique<RunContext>();
            run = std::move(fresh);
            demo.Restart(change_scene);
            auto prepared = PrepareWorld(*npc_definition, options, &npc_config);
            AssembleRun(prepared);
            run->world->RebuildDirtyMeshes();
            renderer->Synchronize(*run->world);
            Assembled(Detail::AssemblyStage::Meshes);
            std::cout << "[demo] ready; round=" << demo.Round() << "; world=" << run->world->Identity()
                      << "; scene=" << (demo.Contest() ? "contest" : "single") << std::endl;
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

        if (!options.npc_scene.empty()) {
            state.npc_config = Data::LoadYamlFile(Assets::Resolve("configs/npc_scene.yaml"));
            const auto& npcs = state.npc_config["npcs"];
            if (!npcs.IsSequence() || npcs.size() != 3) throw std::invalid_argument("NPC scene requires three entities");
            for (std::size_t i = 0; i < npcs.size(); ++i) {
                const auto bytes = Assets::ReadBytes(Assets::Resolve(npcs[i]["tree"].as<std::string>()));
                state.npc_tree_sources.emplace_back(bytes.begin(), bytes.end());
            }
        }
        auto definition = ReadBlockDefinition(!options.npc_scene.empty());
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
        if (state.DemoMode()) {
            state.npc_definition = definition;
            state.window->SetCursorMode(CursorMode::Normal);
        } else if (!state.scenario) state.window->SetCursorMode(CursorMode::Lock);
        auto prepared = PrepareWorld(std::move(definition), options, state.npc_config.IsDefined() ? &state.npc_config : nullptr);
        state.AssembleRun(prepared);
        if (state.npc_config.IsDefined()) {
            state.renderer->LoadCharacter("human", Assets::Resolve("characters/human.glb"));
            state.renderer->LoadCharacter("zombie", Assets::Resolve("characters/zombie.glb"));
        }
        const auto& transform = state.run->registry->GetComponent<Transform>(state.run->player);
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
                      << "; fov=" << state.run->last_result->lens.fov << std::endl;

        state.run->world->RebuildDirtyMeshes();

        if (state.renderer->Modern()) state.renderer->Synchronize(*state.run->world);
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
        const auto& options = state.options;
        double previous_frame_time = Window::Time();
        bool paused = false, paused_minimized = false;
        unsigned rendered_frames = 0;
        std::uint64_t next_frame_id = 1;
        std::cout << "[runtime] ready; chunks=" << state.run->world->ChunkCount()
                  << "; loading_ms=" << Milliseconds(state.entry)
                  << "; spawn=" << state.run->last_result->pose.position.x << ',' << state.run->last_result->pose.position.y
                  << ',' << state.run->last_result->pose.position.z << std::endl;
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
                state.run->session->Pause(); paused = true;
                state.debug.Pause();
                previous_frame_time = current_frame_time;
                window.WaitEvents(0.05); continue;
            }
            bool resumed = !state.scenario && (paused || state.input.focus_lost);
            if (resumed) {
                state.debug.Pause();
                state.input.Clear(); state.pointers.clear(); window.ResetInput();
                state.run->session->Pause();
                if (!state.DemoMode() || state.demo.Playing()) state.run->session->Resume();
                window.CaptureInput(state.input);
                std::cout << "[runtime] resumed; extent=" << window.Width() << ',' << window.Height()
                          << "; rendered_frames=" << rendered_frames << std::endl;
            }
            paused = false;
            const double delta = current_frame_time - previous_frame_time;
            previous_frame_time = current_frame_time;
            if (state.input.Down(Key::Escape)) { window.Close(); state.input.Clear(); break; }
            if (state.DemoMode()) {
                const auto pressed = [&](Key key) { return state.input.pressed[static_cast<std::size_t>(key)]; };
                if (pressed(Key::E) || pressed(Key::Q)) {
                    state.RestartDemo(pressed(Key::E));
                    state.input.Clear(); state.pointers.clear(); window.ResetInput();
                    resumed = true;
                    previous_frame_time = Window::Time();
                } else if (pressed(Key::Space)) {
                    state.demo.Toggle();
                    state.input.Clear(); state.pointers.clear(); window.ResetInput();
                }
                if (state.demo.Playing() && state.run->session->State() == Simulation::SessionState::Paused) {
                    state.run->session->Resume(); resumed = true;
                } else if (!state.demo.Playing() && state.run->session->State() == Simulation::SessionState::Ready) {
                    state.run->session->Pause();
                }
            }
            auto& world = *state.run->world;
            const bool advance = !state.DemoMode() || state.demo.Playing();
            Simulation::AdvanceStatistics frame_statistics;
            if (advance) {
                Simulation::AdvanceInput intent;
                if (state.scenario) {
                    const auto scenario = state.scenario->BeforeAdvance(elapsed, state.run->last_result->pose, world);
                    intent = scenario.input;

                    if (scenario.pose_override) {
                        auto& transform = state.run->registry->GetComponent<Transform>(state.run->player);
                        transform.position = scenario.pose_override->position;
                        transform.yaw = scenario.pose_override->yaw;
                        transform.pitch = scenario.pose_override->pitch;
                    }
                } else if (!state.DemoMode()) {
                    intent = MapInput(state.input);
                    state.pointers.reserve(state.input.pointer_events.size());
                    for (const auto& event : state.input.pointer_events)
                        state.pointers.push_back({event.mouse_dx, event.mouse_dy, event.scroll_y});
                }
                if (state.DemoMode()) intent.allow_interaction = false;
                const float effective_seconds = resumed ? 0.0f : static_cast<float>(std::min(delta, 0.1));
                for (const auto& brain : state.run->brains) brain->Tick(effective_seconds);
                state.run->last_result = state.run->session->Advance(delta, intent, state.pointers);
                const auto& result = *state.run->last_result;
                SYMOCRAFT_PROFILE_PLOT("physics.steps [per frame]", result.statistics.physical_steps);
                if (state.npc_config.IsDefined() && !state.run->session->NpcDisplays().empty()) {
                    state.run->session->CommitFrame(world, result);
                    for (std::size_t i = 0; i < state.run->brains.size(); ++i) {
                        const auto transitions = state.run->brains[i]->DrainTransitions();
                        if (transitions.empty()) continue;
                        const auto snapshot = state.run->brains[i]->Inspect();
                        for (const auto& transition : transitions)
                            std::cout << "[npc] index=" << i << "; node=" << transition.node
                                      << "; state=" << transition.previous << "->" << transition.current << '\n';
                        const auto& action = snapshot.npc.action;
                        std::cout << "[npc] index=" << i << "; action=" << action.id
                                  << "; phase=" << Simulation::ActionPhaseName(action.phase)
                                  << "; target=" << action.target.x << ',' << action.target.y << ',' << action.target.z
                                  << "; elapsed=" << action.elapsed_seconds << '/' << action.duration_seconds
                                  << "; failure=" << Simulation::ActionFailureName(action.failure);
                        for (const auto& resource : snapshot.npc.resources)
                            std::cout << "; resource=" << resource.resource_id << ':' << resource.quantity;
                        std::cout << '\n';
                    }
                } else if (result.edit) world.TryEdit(*result.edit);

                frame_statistics = result.statistics;
                if (result.statistics.recovered) std::cout << "[runtime] returned player to safe spawn" << std::endl;
                if (state.DemoMode()) {
                    bool finished = true;
                    for (const auto& brain : state.run->brains) {
                        const auto snapshot = brain->Inspect();
                        if (snapshot.npc.kind != Simulation::NpcKind::Human) continue;
                        const auto phase = snapshot.npc.action.phase;
                        if ((phase != Simulation::ActionPhase::Succeeded && phase != Simulation::ActionPhase::Failed &&
                             phase != Simulation::ActionPhase::Cancelled) ||
                            !snapshot.idling) finished = false;
                    }
                    if (finished) {
                        state.demo.Complete();
                        state.run->session->Pause();
                        std::cout << "[demo] completed; round=" << state.demo.Round() << std::endl;
                    }
                }
            }
            const auto& result = *state.run->last_result;
            state.input.Clear(); state.pointers.clear(); // Every consumer has finished; retain capacities.

            [[maybe_unused]] const auto rebuilt_chunks = world.RebuildDirtyMeshes();
            SYMOCRAFT_PROFILE_PLOT("mesh.rebuilt [per frame]", rebuilt_chunks);

            [[maybe_unused]] const auto upload = state.renderer->Synchronize(world);
            SYMOCRAFT_PROFILE_PLOT("mesh.upload [bytes per frame]", upload.bytes);
            if (state.npc_config.IsDefined()) {
                std::vector<Detail::CharacterInstance> characters;
                const auto displays = state.run->session->NpcDisplays();
                for (const auto& display : displays)
                    characters.push_back({display.kind == Simulation::NpcKind::Human ? "human" : "zombie",
                                          display.position - glm::vec3(0.0f, 0.9f, 0.0f), display.yaw,
                                          display.session_id, display.advance_id, display.entity,
                                          display.horizontal_distance, display.action});
                state.renderer->SetCharacterInstances(characters);
                if (state.DemoMode()) {
                    Detail::DemoOverlayData overlay;
                    overlay.title = state.demo.Contest() ? "HARVEST DEMO - CONTEST" : "HARVEST DEMO - SINGLE";
                    overlay.state = Detail::DemoStateName(state.demo.State());
                    std::size_t human = 0;
                    for (const auto& display : displays) {
                        if (display.kind != Simulation::NpcKind::Human) continue;
                        const auto& action = display.action;
                        Detail::DemoOverlayRow row;
                        row.label = "HUMAN " + std::to_string(++human) + " - " + Simulation::ActionPhaseName(action.phase);
                        if (action.id != 0)
                            row.label += " TARGET " + std::to_string(action.target.x) + "," +
                                std::to_string(action.target.y) + "," + std::to_string(action.target.z);
                        if (std::isfinite(action.duration_seconds) && action.duration_seconds > 0 &&
                            std::isfinite(action.elapsed_seconds) && action.elapsed_seconds >= 0 &&
                            (action.phase == Simulation::ActionPhase::Harvesting || action.phase == Simulation::ActionPhase::Ready ||
                             action.phase == Simulation::ActionPhase::Succeeded))
                            row.progress = static_cast<float>(std::clamp(action.elapsed_seconds / action.duration_seconds, 0.0, 1.0));
                        overlay.rows.push_back(std::move(row));
                        std::string resources = "RESOURCES:";
                        for (const auto& resource : display.resources)
                            resources += " " + resource.resource_id + " " + std::to_string(resource.quantity);
                        if (display.resources.empty()) resources += " NONE";
                        if (action.failure != Simulation::ActionFailure::None)
                            resources += std::string(" - REASON: ") + Simulation::ActionFailureName(action.failure);
                        overlay.rows.push_back({std::move(resources), {}});
                    }
                    state.renderer->SetDemoOverlay(overlay);
                }
            }

            if (next_frame_id == (std::numeric_limits<std::uint64_t>::max)())
                throw std::overflow_error("game frame identity exhausted");
            auto display_lens = result.lens;
            if (state.DemoMode()) display_lens.fov = 60.0f;
            const auto view = Simulation::DeriveView(state.DemoMode() ? state.run->observer : result.pose,
                display_lens, window.GetAspectRatio());
            const auto frame_id = next_frame_id++;
            const auto frame = state.renderer->Render(frame_id, view, state.DemoMode() ? std::nullopt : Selection(result));
            const auto present_end = Clock::now();
            if (state.renderer->Modern()) {
                SYMOCRAFT_PROFILE_PLOT("renderer.draw [calls per frame]", frame.result.statistics.draw_calls);
                SYMOCRAFT_PROFILE_PLOT("renderer.upload [bytes per frame]", frame.result.statistics.upload_bytes);
            }
            state.debug.Observe(present_end, frame.result.status == Rendering::FrameStatus::Presented,
                frame_statistics.physical_steps,
                std::array<double, 3>{result.pose.position.x, result.pose.position.y, result.pose.position.z}, state.debug_backend);

            if (state.DemoMode() && demo_probe) {
                Detail::DemoSnapshot snapshot;
                snapshot.round = state.demo.Round(); snapshot.world_id = world.Identity(); snapshot.frame_id = frame_id;
                snapshot.state = state.demo.State(); snapshot.contest = state.demo.Contest(); snapshot.advanced = advance;
                snapshot.session_paused = state.run->session->State() == Simulation::SessionState::Paused;
                snapshot.statistics = frame_statistics; snapshot.player = result.pose;
                snapshot.npcs = state.run->session->NpcDisplays();
                const auto& targets = state.npc_config["targets"];
                for (std::size_t i = 0; i < targets.size(); ++i)
                    snapshot.target_blocks.push_back(world.QueryBlock(SceneCell(targets[i])).block.block_id);
                demo_probe(demo_context, snapshot);
            }
            ++rendered_frames;
            SYMOCRAFT_PROFILE_FRAME();
            if (options.frame_limit != 0 && rendered_frames >= options.frame_limit) window.Close();
        }
        const auto position = state.run->last_result->pose.position;
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
        release(ShutdownStep::Simulation, state.run->session != nullptr, [&] {
            state.run->brains.clear();
            state.run->session.reset();
        });
        release(ShutdownStep::World, state.run->world != nullptr, [&] { state.run->world.reset(); });
        release(ShutdownStep::Registry, state.run->registry != nullptr, [&] { state.run->registry.reset(); });
        release(ShutdownStep::RendererObject, state.renderer != nullptr, [&] { state.renderer.reset(); });
        release(ShutdownStep::Window, state.window != nullptr, [&] { state.window->Destroy(); });
        state.window.reset();
        release(ShutdownStep::Runtime, state.runtime != nullptr, [&] { state.runtime->Shutdown(); });
        state.runtime.reset();
        if (!report.Succeeded()) throw ShutdownError(report);
        return report;
    }
}
