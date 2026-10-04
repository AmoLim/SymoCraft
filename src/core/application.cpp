//
// Created by Amo on 2022/6/15.
//

#define STB_IMAGE_IMPLEMENTATION

#include "core/application.h"
#include "core/asset_paths.h"
#include "core/startup_options.h"
#include "core/performance.h"
#include "renderer/performance_memory.h"
#include "world/benchmark_workload.h"
#include "core.h"
#include "core/window.h"
#include "renderer/texture.h"
#include "renderer/renderer.h"
#include "world/chunk.h"
#include "core/ECS/registry.h"
#include "core/ECS/Systems/transform_system.h"
#include "core/ECS/Systems/character_system.h"
#include "core/ECS/Systems/physics_system.h"
#include "core/ECS/component.h"
#include "world/world.h"
#include "world/generation.h"
#include "world/test_scene.h"
#include "playercontroller/playercontroller.h"
#include "input/key_snapshot.h"
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace SymoCraft
{


    static bool first_enter = true;  // is first enter of cursor? initialized by true

    namespace Application
    {
        //irrklang::ISoundEngine *SoundEngine = irrklang::createIrrKlangDevice();

        // Block VAR
        int new_block_id = 0;
        const int kNumBlocks = 7;
        const float kBlockPlaceDebounceTime = 0.2f;
        float block_place_debounce = 0.0f;

        // Time variables
        const float kBlockChangeDebounceTime = 0.2f;
        float block_change_debounce = 0.0f;
        float delta_time = 0.016f;


        static std::unique_ptr<Window> runtime_window;
        static std::unique_ptr<Camera> camera;
        static std::unique_ptr<ECS::Registry> runtime_registry;
        static bool glfw_initialized = false;
        static bool renderer_started = false;
        static bool keyboard_reset_needed = false;

        static void FocusCallback(GLFWwindow*, int focused)
        {
            if (!focused)
            {
                keyboard_reset_needed = true;
                first_enter = true;
            }
        }

        static void ClearStickyKeys(GLFWwindow* window)
        {
            // Do this after event polling: GLFW emits synthetic releases after the focus callback.
            glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_FALSE);
            glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
            keyboard_reset_needed = false;
        }

        struct PreparedWorld {
            TestScene::Pose pose;
            YAML::Node report;
        };

        static PreparedWorld PrepareWorld(const StartupOptions& options, Performance::Session* performance = nullptr)
        {
            Generation::Settings settings;
            settings.seed = options.seed ? *options.seed :
                options.regression_scene ? TestScene::DefaultSeed : Generation::RandomSeed();
            Generation::Timings timings;
            Generation::Build(settings, performance ? &timings : nullptr);
            const auto report_start = Performance::Clock::now();
            PreparedWorld world;
            world.report["generation"] = Generation::Describe(settings);
            world.report["seed_source"] = options.seed ? "explicit" : options.regression_scene ? "scene-default" : "random";
            world.report["chunks"] = ChunkManager::GetAllChunks().size();
            world.report["terrain_digest"] = Generation::BlockDigest();
            if (options.regression_scene) {
                TestScene::Install();
                world.pose = TestScene::Checkpoint(options.checkpoint);
                world.report["scene"] = TestScene::Describe();
                world.report["scene_digest"] = Generation::BlockDigest();
            } else {
                world.pose = {"spawn", Generation::FindSpawnPosition(), -90.0f, 0.0f};
                world.report["scene"]["name"] = "terrain";
                world.report["scene_digest"] = world.report["terrain_digest"].as<std::string>();
            }
            world.report["test_edits_applied"] = options.test_edits;
            if (options.test_edits) TestScene::ApplyEdits();
            world.report["final_digest"] = options.test_edits ? Generation::BlockDigest() : world.report["scene_digest"].as<std::string>();
            auto pose = world.report["initial_pose"];
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
            LoadBlocks(Assets::Resolve("configs/blockFormats.yaml").string());
            const auto world = PrepareWorld(options);
            std::cout << YAML::Dump(world.report) << std::endl;
        }

        void Init(const StartupOptions& options, Performance::Session* performance)
        {
            if (glfw_initialized)
                throw std::logic_error("Application is already initialized");
            const auto window_start = Performance::Clock::now();
            Window::Init();
            glfw_initialized = true;
            if (performance) {
                glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
                if (options.focus_policy == "allow-unfocused") {
                    glfwWindowHint(GLFW_FOCUSED, GLFW_FALSE);
                    glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_FALSE);
                }
            }
            runtime_window.reset(Window::Create("SymoCraft", performance ? options.width : 0,
                                               performance ? options.height : 0, performance != nullptr));
            if (!runtime_window || !runtime_window->window_ptr)
                throw std::runtime_error("Cannot create the game window");
            if (performance) {
                performance->startup["window_and_context"] = Performance::Milliseconds(window_start);
                runtime_window->SetVsync(options.vsync);
                performance->metadata["gl_vendor"] = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
                performance->metadata["gl_renderer"] = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
                performance->metadata["gl_version"] = reinterpret_cast<const char*>(glGetString(GL_VERSION));
                performance->metadata["framebuffer_width"] = runtime_window->width;
                performance->metadata["framebuffer_height"] = runtime_window->height;
                performance->metadata["window_mode"] = "borderless-windowed";
                performance->metadata["taskbar_policy"] = "preserve-shell-z-order";
                GLint samples = 0; glGetIntegerv(GL_SAMPLES, &samples);
                performance->metadata["msaa_samples"] = samples;
                if (runtime_window->width != options.width || runtime_window->height != options.height)
                    throw std::runtime_error("Actual framebuffer " + std::to_string(runtime_window->width) + "x" +
                        std::to_string(runtime_window->height) + " does not match requested benchmark resolution " +
                        std::to_string(options.width) + "x" + std::to_string(options.height));
            }
            runtime_registry = std::make_unique<ECS::Registry>();

            // Initialize all other subsystems.
            ECS::Registry &registry = GetRegistry();
            registry.RegisterComponent<Transform>("Transform");
            registry.RegisterComponent<Physics::RigidBody>("RigidBody");
            registry.RegisterComponent<Physics::HitBox>("HigBox");
            registry.RegisterComponent<Character::CharacterComponent>("CharacterComponent");
            registry.RegisterComponent<Character::PlayerComponent>("PlayerComponent");

            renderer_started = true;
            const auto renderer_start = Performance::Clock::now();
            Renderer::Init();
            if (performance) {
                performance->startup["shaders_buffers_and_block_config"] = Performance::Milliseconds(renderer_start);
                performance->metadata["allocated_vbo_bytes"] = Renderer::AllocatedBufferBytes();
            }
            World::Init();
            first_enter = true;
            keyboard_reset_needed = false;
        }

        void Run(const StartupOptions& options, Performance::Session* performance)
        {
            const auto frame_limit = options.frame_limit;
            Window& window = GetWindow();
            auto* native_window = static_cast<GLFWwindow*>(window.window_ptr);
            const double loading_start = glfwGetTime();
            const auto texture_start = Performance::Clock::now();

            stbi_set_flip_vertically_on_load(true);
            const std::string texture_path = Assets::Resolve("textures/texture_atlas.png").string();
            TextureArray texture_array;
            texture_array = texture_array.CreateAtlasSlice(texture_path, true);
            ValidateBlockTextures(texture_array.layer_amount);
            if (performance) performance->startup["texture_decode_and_upload"] = Performance::Milliseconds(texture_start);

            if (!performance) {
                glfwSetScrollCallback(native_window, MouseScrollCallBack);
                glfwSetCursorPosCallback(native_window, MouseMovementCallBack);
                glfwSetInputMode(native_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            glfwSetInputMode(native_window, GLFW_STICKY_KEYS, GLFW_TRUE);
            glfwSetWindowFocusCallback(native_window, FocusCallback);

            const auto world = PrepareWorld(options, performance);
            std::cout << "[world] summary-begin\n" << YAML::Dump(world.report)
                      << "\n[world] summary-end" << std::endl;

            ECS::Registry &registry = GetRegistry();
            const glm::vec3 start_pos = world.pose.position;
            auto &transform = registry.GetComponent<Transform>(World::GetPlayer());
            transform.position = start_pos;
            transform.yaw = world.pose.yaw;
            transform.pitch = world.pose.pitch;
            TransformSystem::Update(registry);
            Character::Player::SyncCamera(registry);
            const auto first_mesh_start = Performance::Clock::now();
            const auto first_mesh_count = ChunkManager::UpdateAllChunks();
            if (performance) {
                performance->startup["first_mesh"] = Performance::Milliseconds(first_mesh_start);
                performance->metadata["first_mesh_chunks"] = first_mesh_count;
                performance->startup["ready_from_main_entry"] = Performance::Milliseconds(performance->entry);
            }
            std::unique_ptr<GpuTimer> gpu_timer;
            bool nvx_memory = false;
            if (performance) {
                gpu_timer = std::make_unique<GpuTimer>();
                nvx_memory = glfwExtensionSupported("GL_NVX_gpu_memory_info") == GLFW_TRUE;
                performance->metadata["gpu_timer_supported"] = gpu_timer->Supported();
                performance->metadata["nvx_memory_supported"] = nvx_memory;
                performance->metadata["edit_interval_seconds"] = Benchmark::EditInterval;
                performance->metadata["edit_plan"] = "M2-T2 forward 16 writes, then reverse 16 writes; repeat";
                performance->metadata["walk_plan"] = "real physics, x=-20.5..20.5, z=-6.5, speed=4.4, endpoint reversal; no input replay";
            }

            // Loading is not simulation time. Start the frame clock only after the initial mesh is ready.
            Physics::ResetTiming();
            double previous_frame_time = glfwGetTime();
            bool paused = false;
            unsigned int rendered_frames = 0;
            std::cout << "[runtime] ready; chunks=" << ChunkManager::GetAllChunks().size()
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
                const double current_frame_time = glfwGetTime();
                auto& character = registry.GetComponent<Character::CharacterComponent>(World::GetPlayer());
                auto& body = registry.GetComponent<Physics::RigidBody>(World::GetPlayer());
                const bool inactive = !glfwGetWindowAttrib(native_window, GLFW_FOCUSED) ||
                                      glfwGetWindowAttrib(native_window, GLFW_ICONIFIED) ||
                                      window.width <= 0 || window.height <= 0;
                sample.focused = !inactive;
                if (performance && (window.width != options.width || window.height != options.height ||
                                    glfwGetWindowAttrib(native_window, GLFW_ICONIFIED))) {
                    performance->Invalidate("framebuffer-changed-or-minimized");
                    break;
                }
                if (inactive && frame_limit == 0 && !performance)
                {
                    ClearStickyKeys(native_window);
                    character.movement_axis = glm::vec3(0.0f);
                    character.apply_jump_force = false;
                    body.velocity.x = body.velocity.z = 0.0f;
                    Physics::ResetTiming();
                    first_enter = true;
                    paused = true;
                    previous_frame_time = current_frame_time;
                    glfwWaitEventsTimeout(0.05);
                    continue;
                }
                if (performance) keyboard_reset_needed = false;
                if (paused || keyboard_reset_needed)
                {
                    ClearStickyKeys(native_window);
                    character.movement_axis = glm::vec3(0.0f);
                    character.apply_jump_force = false;
                    body.velocity.x = body.velocity.z = 0.0f;
                    Physics::ResetTiming();
                    first_enter = true;
                    paused = true;
                }
                delta_time = paused ? 0.0f : static_cast<float>(std::clamp(current_frame_time - previous_frame_time, 0.0, 0.1));
                paused = false;
                previous_frame_time = current_frame_time;
                sample.simulation_delta_ms = delta_time * 1000.0;
                const auto simulation_start = Performance::Clock::now();

                block_place_debounce -= delta_time;
                block_change_debounce -= delta_time;

                // Temporary Input Process Function
                if (!performance) processInput(native_window);
                else {
                    if (glfwGetKey(native_window, GLFW_KEY_ESCAPE) == GLFW_PRESS) window.Close();
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
                            auto* chunk = ChunkManager::GetChunk(glm::vec3(edit.position));
                            if (!chunk || chunk->GetWorldBlock(glm::vec3(edit.position)).block_id != edit.before ||
                                !chunk->SetWorldBlock(glm::vec3(edit.position), edit.after))
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
                Physics::Update(registry, delta_time);
                if (transform.position.y < -8.0f)
                {
                    transform.position = Generation::FindSpawnPosition();
                    body.zero_forces();
                    body.on_ground = false;
                    character.is_jumping = false;
                    Physics::ResetTiming();
                    std::cout << "[runtime] returned player to safe spawn" << std::endl;
                }
                Character::Player::SyncCamera(registry);
                PlayerController::DoRayCast(registry, window, !performance);
                sample.simulation_ms = Performance::Milliseconds(simulation_start);

                const auto mesh_start = Performance::Clock::now();
                sample.rebuilt_chunks = ChunkManager::UpdateAllChunks();
                sample.mesh_ms = Performance::Milliseconds(mesh_start);
                const auto pack_start = Performance::Clock::now();
                ChunkManager::LoadAllChunks();
                sample.pack_ms = Performance::Milliseconds(pack_start);

                if (performance) {
                    const auto instrumentation_start = Performance::Clock::now();
                    for (const auto& result : gpu_timer->Poll()) performance->SetGpu(result.frame, result.milliseconds);
                    if (sample.elapsed >= next_memory_time) {
                        performance->Add(Performance::SampleMemory(sample.elapsed, nvx_memory));
                        next_memory_time = sample.elapsed + 1.0;
                    }
                    gpu_timer->BeginFrame(performance->NextFrame());
                    sample.instrumentation_ms = Performance::Milliseconds(instrumentation_start);
                }

                const auto render_start = Performance::Clock::now();
                glBindTextureUnit(0, texture_array.m_texture_Id);
                RenderStats stats;
                Renderer::Render(performance ? &stats : nullptr, gpu_timer.get());
                if (gpu_timer) gpu_timer->EndFrame();
                sample.render_cpu_ms = Performance::Milliseconds(render_start);

                const auto present_start = Performance::Clock::now();
                window.SwapBuffers();
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
                performance->metadata["final_world_digest"] = Generation::BlockDigest();
                performance->metadata["total_scheduled_edits_applied"] = applied_edits;
                // Diagnostic readback is outside all warmup/sample timing, never per-frame.
                if (performance->completed) {
                    PlayerController::DoRayCast(registry, window, false);
                    ChunkManager::LoadAllChunks();
                    Renderer::Render();
                    Performance::CaptureFramebuffer(window.width, window.height, performance->Directory() / "final-frame.png");
                    performance->metadata["screenshot"] = "final-frame.png; extra render/readback after measurement";
                }
            }
            std::cout << "[runtime] loop finished; rendered_frames=" << rendered_frames
                      << "; player=" << transform.position.x << ',' << transform.position.y << ',' << transform.position.z
                      << std::endl;
        }

        void Free()
        {
            ChunkManager::FreeAllChunks();
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
            if (glfw_initialized)
            {
                Window::Free();
                glfw_initialized = false;
            }
        }

        Window& GetWindow()
        {
            if (!runtime_window)
                throw std::logic_error("Game window is not initialized");
            return *runtime_window;
        }

        Camera* GetCamera()
        {
            if (!camera)
                camera = std::make_unique<Camera>(static_cast<float>(GetWindow().width), static_cast<float>(GetWindow().height));
            return camera.get();
        }

        ECS::Registry &GetRegistry()
        {
            if (!runtime_registry)
                throw std::logic_error("Game registry is not initialized");
            return *runtime_registry;
        }
/*
        GlobalThreadPool& GetGlobalThreadPool()
        {
            return *global_thread_pool;
        }
*/
        void MouseMovementCallBack(GLFWwindow* window, double xpos_in, double ypos_in)
        {
            if (!glfwGetWindowAttrib(window, GLFW_FOCUSED))
            {
                first_enter = true;
                return;
            }
            static float last_x = 0;       // last x position of cursor
            static float last_y = 0;       // last y position of cursor
            ECS::Registry &registry = Application::GetRegistry();
            //Transform &transform = registry.GetComponent<Transform>(camera->entity_id);
            auto &transform = registry.GetComponent<Transform>(World::GetPlayer());
            auto &player_com = registry.GetComponent<Character::PlayerComponent>(World::GetPlayer());
            auto xpos = static_cast<float>(xpos_in);
            auto ypos = static_cast<float>(ypos_in);

            // modify the first enter of the mouse
            if (first_enter)
            {
                last_x = xpos;
                last_y = ypos;
                first_enter = false;
            }
            float xoffset = xpos - last_x;
            float yoffset = last_y - ypos;   // reversed since y-coordinates range from bottom to top
            last_x = xpos;
            last_y = ypos;

            const float sensitivity = 0.05f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;


            transform.pitch += yoffset;
            transform.yaw += xoffset;
            transform.pitch = glm::clamp(transform.pitch, -89.0f, 89.0f);


        }

        void MouseScrollCallBack(GLFWwindow* window, double x_pos_in, double y_pos_in)
        {
            GetCamera()->InsMouseScrollCallBack(window, x_pos_in, y_pos_in);
        }

        void processInput(GLFWwindow* window)
        {
            using KeySampling::Control;
            constexpr std::array<int, KeySampling::Count> native_keys{
                GLFW_KEY_ESCAPE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_CAPS_LOCK, GLFW_KEY_LEFT_CONTROL,
                GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_A, GLFW_KEY_SPACE, GLFW_KEY_E, GLFW_KEY_Q
            };
            const auto keys = KeySampling::Capture([window, &native_keys](Control control)
            {
                return glfwGetKey(window, native_keys[static_cast<std::size_t>(control)]) == GLFW_PRESS;
            });
            if (keys.Down(Control::Exit))
                glfwSetWindowShouldClose(window, true);

            ECS::Registry &registry = GetRegistry();
            auto &player_com = registry.GetComponent<Character::CharacterComponent>(World::GetPlayer());
            auto &rigid_body = registry.GetComponent<Physics::RigidBody>(World::GetPlayer());

            // --------------------------------------------------------------------------------------------
            // process input for camera moving


            player_com.is_running = keys.Down(Control::Run);
            player_com.movement_axis.y = 0.0f;

            if (keys.Down(Control::Sensor))
            {
                rigid_body.is_sensor = true;
                rigid_body.use_gravity = false;
                player_com.movement_axis.y =
                        keys.Down(Control::Descend)
                        ? -1.0f
                        : 0.0f;
            }
            else
            {
                rigid_body.is_sensor =false;
                rigid_body.use_gravity = true;
            }

            player_com.movement_axis.x =
                    keys.Down(Control::Forward)
                    ? 1.0f
                    :keys.Down(Control::Backward)
                      ? -1.0f
                      : 0.0f;
            player_com.movement_axis.z =
                    keys.Down(Control::Right)
                    ? 1.0f
                    :
                    keys.Down(Control::Left)
                      ? -1.0f
                      : 0.0f;

            if (keys.Down(Control::Jump))
            {
                if (!player_com.is_jumping && rigid_body.on_ground)
                {
                    player_com.apply_jump_force = true;
                }
            }

            if (keys.Down(Control::NextBlock) && block_change_debounce <= 0.0f)
            {
                new_block_id++;
                if (new_block_id > kNumBlocks)
                    new_block_id = 0;
                block_change_debounce = kBlockChangeDebounceTime;
                PlayerController::DisplayCurrentBlockName();
            }

            if (keys.Down(Control::PreviousBlock) && block_change_debounce <= 0.0f)
            {
                new_block_id--;
                if (new_block_id < 0)
                    new_block_id = kNumBlocks;
                block_change_debounce = kBlockChangeDebounceTime;
                PlayerController::DisplayCurrentBlockName();
            }

        }

    }

}
