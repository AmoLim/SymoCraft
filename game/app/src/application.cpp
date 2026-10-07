//
// Created by Amo on 2022/6/15.
//

#include "application.h"
#include "startup_options.h"
#include <symocraft/assets/asset_paths.h>
#include <symocraft/assets/image.h>
#include <symocraft/telemetry/performance.h>
#include <symocraft/telemetry/document_io.h>
#include <symocraft/world/benchmark_workload.h>
#include <symocraft/platform/window.h>
#include <symocraft/platform/process_memory.h>
#include <symocraft/renderer/renderer.h>
#include <symocraft/renderer/gpu_timer.h>
#include <symocraft/ecs/registry.h>
#include <symocraft/simulation/transform_system.h>
#include <symocraft/simulation/character_system.h>
#include <symocraft/simulation/physics_system.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/player.h>
#include <symocraft/world/world.h>
#include <symocraft/world/generation.h>
#include <symocraft/world/test_scene.h>
#include <symocraft/simulation/playercontroller.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>

namespace SymoCraft
{


    namespace Application
    {
        static float block_place_debounce = 0.0f;
        static Simulation::PlayerInputState input_state;
        static ECS::EntityId player_id{};
        static std::unique_ptr<Window> runtime_window;
        static std::unique_ptr<Camera> camera;
        static std::unique_ptr<ECS::Registry> runtime_registry;
        static std::optional<World::BlockDefinition> pending_block_definition;
        static bool platform_initialized = false;
        static bool renderer_started = false;

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
                performance->startup["world_allocation"] = timings.allocation_ms;
                performance->startup["terrain"] = timings.terrain_ms;
                performance->startup["vegetation"] = timings.vegetation_ms;
                performance->startup["fixtures_digests_and_metadata"] = Performance::Milliseconds(report_start);
                performance->metadata["world"] = world.report;
            }
            return world;
        }

        void PrintWorldSummary(const StartupOptions& options)
        {
            const auto world = PrepareWorld(ReadBlockDefinition(), options);
            std::cout << Data::DumpYaml(world.report) << std::endl;
        }

        void Init(const StartupOptions& options, Performance::Session* performance)
        {
            if (platform_initialized)
                throw std::logic_error("Application is already initialized");
            const auto window_start = Performance::Clock::now();
            Window::Init();
            platform_initialized = true;
            runtime_window.reset(Window::Create("SymoCraft", performance ? options.width : 0,
                                               performance ? options.height : 0, performance != nullptr,
                                               performance && options.focus_policy == "allow-unfocused"));
            if (!runtime_window)
                throw std::runtime_error("Cannot create the game window");
            Renderer::AttachContext(*runtime_window);
            if (performance) {
                performance->startup["window_and_context"] = Performance::Milliseconds(window_start);
                Renderer::SetVsync(options.vsync);
                const auto& device = Renderer::Device();
                performance->metadata["gl_vendor"] = device.vendor;
                performance->metadata["gl_renderer"] = device.renderer;
                performance->metadata["gl_version"] = device.version;
                performance->metadata["msaa_samples"] = device.msaa_samples;
                performance->metadata["framebuffer_width"] = runtime_window->width;
                performance->metadata["framebuffer_height"] = runtime_window->height;
                performance->metadata["window_mode"] = "borderless-windowed";
                performance->metadata["taskbar_policy"] = "preserve-shell-z-order";
                if (runtime_window->width != options.width || runtime_window->height != options.height)
                    throw std::runtime_error("Actual framebuffer " + std::to_string(runtime_window->width) + "x" +
                        std::to_string(runtime_window->height) + " does not match requested benchmark resolution " +
                        std::to_string(options.width) + "x" + std::to_string(options.height));
            }
            runtime_registry = std::make_unique<ECS::Registry>();

            // Initialize all other subsystems.
            ECS::Registry &registry = *runtime_registry;
            registry.RegisterComponent<Transform>("Transform");
            registry.RegisterComponent<Physics::RigidBody>("RigidBody");
            registry.RegisterComponent<Physics::HitBox>("HigBox");
            registry.RegisterComponent<Character::CharacterComponent>("CharacterComponent");
            registry.RegisterComponent<Character::PlayerComponent>("PlayerComponent");

            renderer_started = true;
            const auto renderer_start = Performance::Clock::now();
            camera = std::make_unique<Camera>(registry, static_cast<float>(runtime_window->width),
                                              static_cast<float>(runtime_window->height));
            Renderer::Init();
            pending_block_definition = ReadBlockDefinition();
            if (performance) {
                performance->startup["shaders_buffers_and_block_config"] = Performance::Milliseconds(renderer_start);
                performance->metadata["allocated_vbo_bytes"] = Renderer::AllocatedBufferBytes();
            }
            player_id = Simulation::CreatePlayer(registry, *camera);
            input_state = {};
            block_place_debounce = 0.0f;
        }

        static Simulation::PlayerIntent MapInput(const InputSnapshot& input)
        {
            Simulation::PlayerIntent intent;
            intent.run = input.Down(Key::LeftShift); intent.sensor = input.Down(Key::CapsLock);
            intent.descend = input.Down(Key::LeftControl); intent.forward = input.Down(Key::W);
            intent.backward = input.Down(Key::S); intent.right = input.Down(Key::D);
            intent.left = input.Down(Key::A); intent.jump = input.Down(Key::Space);
            intent.next_block = input.Down(Key::E); intent.previous_block = input.Down(Key::Q);
            return intent;
        }

        static CameraView CurrentCamera(const Window& window)
        {
            return {camera->GetCameraProjMat(window.GetAspectRatio()), camera->GetCameraViewMat()};
        }

        static void UpdateInteraction(ECS::Registry& registry, World::VoxelWorld& world,
                                       const InputSnapshot& input, bool allow_input)
        {
            const PlayerController::InteractionInput interaction{
                allow_input, input.right_button, input.left_button, input_state.selected_block};
            const auto result = PlayerController::DoRayCast(registry, world, player_id, interaction, block_place_debounce);
            if (result.edit) world.TryEdit(*result.edit);
            Renderer::SetSelection(result.selection);
        }

        void Run(const StartupOptions& options, Performance::Session* performance)
        {
            if (!runtime_window || !runtime_registry || !camera || !pending_block_definition)
                throw std::logic_error("Application Run requires a fresh successful Init");
            const auto frame_limit = options.frame_limit;
            Window& window = *runtime_window;
            const double loading_start = Window::Time();
            const auto texture_start = Performance::Clock::now();

            const std::string texture_path = Assets::Resolve("textures/texture_atlas.png").string();
            pending_block_definition->ValidateTextureLayers(Renderer::LoadTextureAtlas(texture_path));
            if (performance) performance->startup["texture_decode_and_upload"] = Performance::Milliseconds(texture_start);

            if (!performance) window.SetCursorMode(CursorMode::Lock);

            const auto prepared_world = PrepareWorld(std::move(*pending_block_definition), options, performance);
            pending_block_definition.reset();
            World::VoxelWorld& world = *prepared_world.instance;
            std::cout << "[world] summary-begin\n" << Data::DumpYaml(prepared_world.report)
                      << "\n[world] summary-end" << std::endl;

            ECS::Registry &registry = *runtime_registry;
            const glm::vec3 start_pos = prepared_world.pose.position;
            auto &transform = registry.GetComponent<Transform>(player_id);
            transform.position = start_pos;
            transform.yaw = prepared_world.pose.yaw;
            transform.pitch = prepared_world.pose.pitch;
            TransformSystem::Update(registry);
            Character::Player::SyncCamera(registry, camera->entity_id);
            const auto first_mesh_start = Performance::Clock::now();
            const auto first_mesh_count = world.RebuildDirtyMeshes();
            if (performance) {
                performance->startup["first_mesh"] = Performance::Milliseconds(first_mesh_start);
                performance->metadata["first_mesh_chunks"] = first_mesh_count;
                performance->startup["ready_from_main_entry"] = Performance::Milliseconds(performance->entry);
            }
            std::unique_ptr<GpuTimer> gpu_timer;
            if (performance) {
                gpu_timer = std::make_unique<GpuTimer>();
                performance->metadata["gpu_timer_supported"] = gpu_timer->Supported();
                performance->metadata["nvx_memory_supported"] = Renderer::Device().nvx_memory_supported;
                performance->metadata["edit_interval_seconds"] = Benchmark::EditInterval;
                performance->metadata["edit_plan"] = "M2-T2 forward 16 writes, then reverse 16 writes; repeat";
                performance->metadata["walk_plan"] = "real physics, x=-20.5..20.5, z=-6.5, speed=4.4, endpoint reversal; no input replay";
            }

            // Loading is not simulation time. Start the frame clock only after the initial mesh is ready.
            Physics::ResetTiming();
            double previous_frame_time = Window::Time();
            bool paused = false;
            unsigned int rendered_frames = 0;
            std::cout << "[runtime] ready; chunks=" << world.ChunkCount()
                      << "; loading_ms=" << (previous_frame_time - loading_start) * 1000.0
                      << "; spawn=" << start_pos.x << ',' << start_pos.y << ',' << start_pos.z << std::endl;
            if (performance) performance->Status("warmup");
            const auto benchmark_start = Performance::Clock::now();
            bool sampling_announced = false;
            auto previous_present = benchmark_start;
            double next_memory_time = 0.0;
            std::uint64_t applied_edits = 0;
            bool walk_forward = true;

            // -------------------------------------------------------------------
            // Render Loop
            while (!window.ShouldClose())
            {
                const auto frame_start = Performance::Clock::now();
                Performance::Frame sample;
                sample.elapsed = Performance::Milliseconds(benchmark_start, frame_start) / 1000.0;
                if (performance && sample.elapsed >= options.warmup_seconds + options.sample_seconds) {
                    performance->completed = true;
                    window.Close();
                    break;
                }
                sample.measured = sample.elapsed >= options.warmup_seconds;
                if (performance && sample.measured && !sampling_announced) {
                    performance->Status("sampling", sample.elapsed);
                    sampling_announced = true;
                }
                window.PollInt();
                sample.event_ms = Performance::Milliseconds(frame_start);
                if (window.ShouldClose())
                    break;
                const double current_frame_time = Window::Time();
                auto& character = registry.GetComponent<Character::CharacterComponent>(player_id);
                auto& body = registry.GetComponent<Physics::RigidBody>(player_id);
                const bool inactive = !window.Focused() || window.Minimized() ||
                                      window.width <= 0 || window.height <= 0;
                sample.focused = !inactive;
                if (performance && (window.width != options.width || window.height != options.height ||
                                    window.Minimized())) {
                    performance->Invalidate("framebuffer-changed-or-minimized");
                    break;
                }
                if (inactive && frame_limit == 0 && !performance)
                {
                    window.ResetInput();
                    character.movement_axis = glm::vec3(0.0f);
                    character.apply_jump_force = false;
                    body.velocity.x = body.velocity.z = 0.0f;
                    Physics::ResetTiming();
                    paused = true;
                    previous_frame_time = current_frame_time;
                    window.WaitEvents(0.05);
                    continue;
                }
                if (paused || (!performance && window.Input().focus_lost))
                {
                    window.ResetInput();
                    character.movement_axis = glm::vec3(0.0f);
                    character.apply_jump_force = false;
                    body.velocity.x = body.velocity.z = 0.0f;
                    Physics::ResetTiming();
                    paused = true;
                }
                const float delta_time = paused ? 0.0f : static_cast<float>(std::clamp(current_frame_time - previous_frame_time, 0.0, 0.1));
                paused = false;
                previous_frame_time = current_frame_time;
                sample.simulation_delta_ms = delta_time * 1000.0;
                const auto simulation_start = Performance::Clock::now();

                block_place_debounce -= delta_time;
                window.CaptureInput();
                if (window.Input().Down(Key::Escape)) window.Close();
                if (!performance) {
                    for (const auto& event : window.Input().pointer_events)
                        Simulation::ApplyPointerInput(registry, player_id, *camera,
                                                      event.mouse_dx, event.mouse_dy, event.scroll_y);
                    Simulation::ApplyInput(registry, player_id, *camera,
                                           MapInput(window.Input()), input_state, delta_time);
                }
                else {
                    character.is_running = false;
                    character.apply_jump_force = false;
                    character.movement_axis = glm::vec3(0.0f);
                    if (options.benchmark == "walk") {
                        if (walk_forward && transform.position.x >= 20.5f) { transform.position.x = 20.5f; walk_forward = false; }
                        if (!walk_forward && transform.position.x <= -20.5f) { transform.position.x = -20.5f; walk_forward = true; }
                        transform.yaw = walk_forward ? 0.0f : 180.0f;
                        character.movement_axis.x = 1.0f;
                    }
                    if (options.benchmark == "edit") {
                        const auto due = Benchmark::DueEdits(sample.elapsed);
                        while (applied_edits < due) {
                            const auto edit = Benchmark::CycleEdit(applied_edits);
                            const auto before = world.QueryBlock(edit.position);
                            if (before.status != World::BlockQueryStatus::Found || before.block.block_id != edit.before ||
                                !world.TryEdit({World::EditOperation::Set, edit.position, edit.after}).Accepted())
                                throw std::runtime_error("Benchmark edit precondition failed");
                            sample.edit_lateness_ms = std::max(sample.edit_lateness_ms,
                                (sample.elapsed - (applied_edits + 1) * Benchmark::EditInterval) * 1000.0);
                            ++applied_edits; ++sample.edits;
                        }
                    }
                }
                if (window.ShouldClose())
                    break;
                TransformSystem::Update(registry);
                Character::Player::Update(registry);
                Physics::Update(registry, world, delta_time);
                if (transform.position.y < -8.0f)
                {
                    transform.position = world.FindSpawn();
                    body.zero_forces();
                    body.on_ground = false;
                    character.is_jumping = false;
                    Physics::ResetTiming();
                    std::cout << "[runtime] returned player to safe spawn" << std::endl;
                }
                Character::Player::SyncCamera(registry, camera->entity_id);
                UpdateInteraction(registry, world, window.Input(), !performance);
                sample.simulation_ms = Performance::Milliseconds(simulation_start);

                const auto mesh_start = Performance::Clock::now();
                sample.rebuilt_chunks = world.RebuildDirtyMeshes();
                sample.mesh_ms = Performance::Milliseconds(mesh_start);
                const auto pack_start = Performance::Clock::now();
                world.VisitMeshes([](const World::WorldMeshRecord& record) { Renderer::AppendMesh(record.vertices); });
                sample.pack_ms = Performance::Milliseconds(pack_start);

                if (performance) {
                    const auto instrumentation_start = Performance::Clock::now();
                    for (const auto& result : gpu_timer->Poll()) performance->SetGpu(result.frame, result.milliseconds);
                    if (sample.elapsed >= next_memory_time) {
                        const auto process = Platform::SampleProcessMemory();
                        const auto device = Renderer::SampleDeviceMemory();
                        Performance::Memory memory;
                        memory.elapsed = sample.elapsed;
                        memory.working_set_bytes = process.working_set_bytes;
                        memory.private_bytes = process.private_bytes;
                        memory.device_dedicated_kib = device.dedicated_kib;
                        memory.device_available_kib = device.available_kib;
                        performance->Add(memory);
                        next_memory_time = sample.elapsed + 1.0;
                    }
                    gpu_timer->BeginFrame(performance->NextFrame());
                    sample.instrumentation_ms = Performance::Milliseconds(instrumentation_start);
                }

                const auto render_start = Performance::Clock::now();
                RenderStats stats;
                Renderer::SetViewport(window.width, window.height);
                Renderer::Render(CurrentCamera(window), performance ? &stats : nullptr, gpu_timer.get());
                if (gpu_timer) gpu_timer->EndFrame();
                sample.render_cpu_ms = Performance::Milliseconds(render_start);

                const auto present_start = Performance::Clock::now();
                Renderer::Present(window);
                const auto present_end = Performance::Clock::now();
                sample.present_ms = Performance::Milliseconds(present_start, present_end);
                sample.frame_ms = Performance::Milliseconds(previous_present, present_end);
                previous_present = present_end;
                if (performance) {
                    sample.vertices = stats.vertices; sample.upload_bytes = stats.upload_bytes;
                    sample.draw_calls = stats.draw_calls; sample.upload_cpu_ms = stats.upload_cpu_ms;
                    sample.x = transform.position.x; sample.y = transform.position.y; sample.z = transform.position.z; sample.yaw = transform.yaw;
                    performance->Add(sample);
                    if (rendered_frames == 0) {
                        performance->startup["first_upload_cpu"] = stats.upload_cpu_ms;
                        performance->metadata["first_upload_bytes"] = stats.upload_bytes;
                        performance->startup["first_frame_swap_return_from_main_entry"] = Performance::Milliseconds(performance->entry, present_end);
                    }
                }
                ++rendered_frames;
                if (frame_limit != 0 && rendered_frames >= frame_limit)
                    window.Close();
            }
            if (performance) {
                for (const auto& result : gpu_timer->Poll()) performance->SetGpu(result.frame, result.milliseconds);
                performance->metadata["final_world_digest"] = world.Digest();
                performance->metadata["total_scheduled_edits_applied"] = applied_edits;
                // Diagnostic readback is outside all warmup/sample timing, never per-frame.
                if (performance->completed) {
                    UpdateInteraction(registry, world, window.Input(), false);
                    world.VisitMeshes([](const World::WorldMeshRecord& record) { Renderer::AppendMesh(record.vertices); });
                    Renderer::Render(CurrentCamera(window));
                    Renderer::CaptureFramebuffer(window.width, window.height, performance->Directory() / "final-frame.png");
                    performance->metadata["screenshot"] = "final-frame.png; extra render/readback after measurement";
                }
            }
            std::cout << "[runtime] loop finished; rendered_frames=" << rendered_frames
                      << "; player=" << transform.position.x << ',' << transform.position.y << ',' << transform.position.z
                      << std::endl;
        }

        void Free()
        {
            pending_block_definition.reset();
            if (renderer_started)
            {
                Renderer::Free();
                renderer_started = false;
            }
            camera.reset();
            if (runtime_registry)
            {
                runtime_registry->Clear();
                runtime_registry.reset();
            }
            if (runtime_window)
            {
                runtime_window->Destroy();
                runtime_window.reset();
            }
            if (platform_initialized)
            {
                Window::Free();
                platform_initialized = false;
            }
        }

    }
}
