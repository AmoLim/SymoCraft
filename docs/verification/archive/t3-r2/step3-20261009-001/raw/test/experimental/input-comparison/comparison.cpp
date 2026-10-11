#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "comparison.h"
#include <Windows.h>
#include <glad/glad.h>
#include <symocraft/simulation/camera.h>
#include <symocraft/simulation/component.h>
#include <symocraft/simulation/player.h>
#include <symocraft/simulation/player_math.h>
#include <symocraft/simulation/transform_system.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace SymoCraft::Experimental::Comparison {
    void Require(bool condition, const char* operation) {
        if (!condition) throw std::runtime_error(operation);
    }
    Options Parse(int argc, char** argv) {
        Options options;
        for (int index = 1; index < argc; ++index) {
            const std::string name(argv[index]);
            Require(index + 1 < argc, "Missing argument value");
            const std::string value(argv[++index]);
            if (name == "--output") {
                options.output = std::filesystem::path(std::u8string(value.begin(), value.end()));
            } else if (name == "--frames") {
                std::size_t used{};
                options.frames = std::stoi(value, &used);
                Require(used == value.size() && options.frames >= 0 && options.frames <= 10000, "Invalid --frames (0..10000)");
            } else if (name == "--cursor") {
                Require(value == "lock" || value == "normal" || value == "hidden", "Invalid --cursor");
                options.cursor = value == "lock" ? CursorMode::Lock : value == "hidden" ? CursorMode::Hidden : CursorMode::Normal;
            } else if (name == "--system-scale") {
                Require(value == "0" || value == "1", "Invalid --system-scale");
                options.system_scale = value == "1";
            } else {
                throw std::runtime_error("Unknown argument: " + name);
            }
        }
        Require(!options.output.empty(), "Pass a fresh --output directory");
        options.output = std::filesystem::absolute(options.output).lexically_normal();
        Require(!std::filesystem::exists(options.output), "Output directory already exists; previous evidence must not be overwritten");
        return options;
    }
    const char* CursorName(CursorMode mode) {
        return mode == CursorMode::Lock ? "lock" : mode == CursorMode::Hidden ? "hidden" : "normal";
    }
    CursorMode NextCursor(CursorMode mode) {
        return mode == CursorMode::Lock ? CursorMode::Normal : mode == CursorMode::Normal ? CursorMode::Hidden : CursorMode::Lock;
    }
    unsigned KeyMask(const InputSnapshot& input) {
        unsigned mask{};
        for (std::size_t index = 0; index < input.keys.size(); ++index)
            if (input.keys[index]) mask |= 1u << index;
        return mask;
    }
    Session::Session(const Options& options) : options_(options) {
        Require(std::filesystem::create_directories(options_.output), "Cannot create fresh output directory");
        frames_.open(options_.output / "frames.csv");
        events_.open(options_.output / "pointer-events.csv");
        Require(frames_.good() && events_.good(), "Cannot open input evidence files");
        frames_ << "frame,time_s,focused,minimized,focus_lost,cursor,key_mask,left,right,pointer_count,control_mask,yaw,pitch,fov,x,y,z\n";
        events_ << "frame,index,dx,dy,scroll_y\n";
        frames_ << std::setprecision(17);
        events_ << std::setprecision(17);
    }
    Session::~Session() {
        if (!finished_) {
            try { Finish(false, 0); } catch (...) {}
        }
    }
    void Session::Record(double time, const InputSnapshot& input, bool focused, bool minimized, CursorMode cursor, const Pose& pose, unsigned controls) {
        frames_ << frame_ << ',' << time << ',' << focused << ',' << minimized << ',' << input.focus_lost << ','
                << CursorName(cursor) << ',' << KeyMask(input) << ',' << input.left_button << ',' << input.right_button
                << ',' << input.pointer_events.size() << ',' << controls << ',' << pose.yaw << ',' << pose.pitch << ','
                << pose.fov << ',' << pose.x << ',' << pose.y << ',' << pose.z << '\n';
        for (std::size_t index = 0; index < input.pointer_events.size(); ++index) {
            const auto& event = input.pointer_events[index];
            events_ << frame_ << ',' << index << ',' << event.mouse_dx << ',' << event.mouse_dy << ',' << event.scroll_y << '\n';
        }
        Require(frames_.good() && events_.good(), "Input evidence write failed");
        ++frame_;
    }
    void Session::Finish(bool normal, unsigned rendered_frames) {
        frames_.flush(); events_.flush();
        Require(frames_.good() && events_.good(), "Input evidence flush failed");
        std::ofstream report(options_.output / "session.json");
        report << "{\n  \"backend\": \"" << SYMOCRAFT_COMPARISON_BACKEND << "\",\n"
               << "  \"normal_exit\": " << (normal ? "true" : "false") << ",\n"
               << "  \"recorded_frames\": " << frame_ << ",\n  \"rendered_frames\": " << rendered_frames << ",\n"
               << "  \"initial_cursor\": \"" << CursorName(options_.cursor) << "\",\n"
               << "  \"sdl_system_scale_requested\": " << (options_.system_scale ? "true" : "false") << ",\n"
               << "  \"camera_source\": \"frozen GLFW baseline Simulation::ApplyPointerInput and Camera\",\n"
               << "  \"production_platform\": false,\n  \"human_validation\": false\n}\n";
        report.flush();
        Require(report.good(), "Session report write failed");
        finished_ = true;
    }

    namespace {
        struct Vertex { glm::vec3 position, color; };
        GLuint Shader(GLenum type, const char* source) {
            const auto shader = glCreateShader(type);
            Require(shader != 0, "glCreateShader failed");
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint success{};
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                std::array<char, 1024> log{};
                glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
                glDeleteShader(shader);
                throw std::runtime_error(std::string("Comparison shader: ") + log.data());
            }
            return shader;
        }
        GLuint Program() {
            const auto vertex = Shader(GL_VERTEX_SHADER, "#version 460 core\nlayout(location=0) in vec3 p; layout(location=1) in vec3 c; uniform mat4 m; out vec3 color; void main(){color=c;gl_Position=m*vec4(p,1);}");
            GLuint fragment{}, program{};
            try {
                fragment = Shader(GL_FRAGMENT_SHADER, "#version 460 core\nin vec3 color; out vec4 o; void main(){o=vec4(color,1);}");
                program = glCreateProgram();
                Require(program != 0, "glCreateProgram failed");
                glAttachShader(program, vertex); glAttachShader(program, fragment); glLinkProgram(program);
                GLint linked{};
                glGetProgramiv(program, GL_LINK_STATUS, &linked);
                Require(linked != 0, "Comparison program link failed");
            } catch (...) {
                if (program) glDeleteProgram(program);
                if (fragment) glDeleteShader(fragment);
                glDeleteShader(vertex);
                throw;
            }
            glDeleteShader(vertex); glDeleteShader(fragment);
            return program;
        }
        void Cube(std::vector<Vertex>& vertices, glm::vec3 center, glm::vec3 size, glm::vec3 color) {
            constexpr int faces[6][6]{{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
            const std::array<glm::vec3,8> corners{{{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}}};
            for (int face = 0; face < 6; ++face)
                for (const auto corner : faces[face])
                    vertices.push_back({center + corners[corner] * size * 0.5f, color * (0.58f + 0.07f * face)});
        }
    }
    struct Scene::Impl {
        ECS::Registry registry;
        Camera camera;
        ECS::EntityId player{};
        GLuint program{}, vao{}, vbo{};
        GLsizei vertices{};
        GLint matrix{};
        Impl() : camera(PrepareRegistry(), 960, 640, {0, 2.2f, 8}) {
            player = Simulation::CreatePlayer(registry, camera);
            Reset();
            try { InitializeGpu(); }
            catch (...) { ReleaseGpu(); throw; }
        }
        void InitializeGpu() {
            program = Program();
            matrix = glGetUniformLocation(program, "m");
            Require(matrix >= 0, "Comparison matrix uniform missing");
            std::vector<Vertex> mesh;
            Cube(mesh, {0,-0.15f,0}, {50,0.2f,50}, {0.17f,0.24f,0.21f});
            for (int index = -24; index <= 24; index += 2) {
                Cube(mesh, {static_cast<float>(index),0,0}, {0.025f,0.025f,48}, {0.4f,0.48f,0.43f});
                Cube(mesh, {0,0,static_cast<float>(index)}, {48,0.025f,0.025f}, {0.4f,0.48f,0.43f});
            }
            Cube(mesh, {-5,2,-4}, {2,4,2}, {0.95f,0.34f,0.31f});
            Cube(mesh, {0,1,-4}, {2,2,2}, {0.32f,0.72f,0.91f});
            Cube(mesh, {5,3,-4}, {2,6,2}, {0.93f,0.83f,0.28f});
            Cube(mesh, {-10,1,6}, {1,2,1}, {0.83f,0.43f,0.77f});
            Cube(mesh, {10,2,6}, {1,4,1}, {0.43f,0.85f,0.68f});
            vertices = static_cast<GLsizei>(mesh.size());
            glGenVertexArrays(1, &vao); glGenBuffers(1, &vbo);
            Require(vao && vbo, "Comparison mesh allocation failed");
            glBindVertexArray(vao); glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.size() * sizeof(Vertex)), mesh.data(), GL_STATIC_DRAW);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(sizeof(glm::vec3))); glEnableVertexAttribArray(1);
            Require(glGetError() == GL_NO_ERROR, "Comparison scene initialization GL error");
        }
        ECS::Registry& PrepareRegistry() {
            registry.RegisterComponent<Transform>("Transform");
            registry.RegisterComponent<Physics::HitBox>("HitBox");
            registry.RegisterComponent<Physics::RigidBody>("RigidBody");
            registry.RegisterComponent<Character::CharacterComponent>("CharacterComponent");
            registry.RegisterComponent<Character::PlayerComponent>("PlayerComponent");
            return registry;
        }
        void ReleaseGpu() {
            if (vbo) glDeleteBuffers(1, &vbo);
            if (vao) glDeleteVertexArrays(1, &vao);
            if (program) glDeleteProgram(program);
        }
        ~Impl() { ReleaseGpu(); }
        void Reset() {
            camera.SetCameraPos({0,2.2f,8});
            auto& pose = registry.GetComponent<Transform>(player);
            pose.position = camera.GetCameraPos(); pose.yaw = -90; pose.pitch = 0;
            camera.SetYaw(-90); camera.SetPitch(0);
            camera.Scroll(-45);
            TransformSystem::Update(registry);
        }
    };
    Scene::Scene() : impl_(std::make_unique<Impl>()) {}
    Scene::~Scene() = default;
    void Scene::ResetCamera() { impl_->Reset(); }
    Pose Scene::CameraPose() const {
        const auto position = impl_->camera.GetCameraPos();
        return {impl_->camera.GetYaw(),impl_->camera.GetPitch(),impl_->camera.GetFov(),position.x,position.y,position.z};
    }
    void Scene::Update(const InputSnapshot& input, double delta) {
        for (const auto& event : input.pointer_events)
            Simulation::ApplyPointerInput(impl_->registry, impl_->player, impl_->camera, event.mouse_dx, event.mouse_dy, event.scroll_y);
        auto& pose = impl_->registry.GetComponent<Transform>(impl_->player);
        const glm::vec3 movement{input.Down(Key::W) ? 1.0f : input.Down(Key::S) ? -1.0f : 0.0f,
                                input.Down(Key::Space) ? 1.0f : input.Down(Key::LeftControl) ? -1.0f : 0.0f,
                                input.Down(Key::D) ? 1.0f : input.Down(Key::A) ? -1.0f : 0.0f};
        pose.position += PlayerMath::DesiredVelocity(movement, pose.yaw, input.Down(Key::LeftShift) ? 6.2f : 4.4f, false) * static_cast<float>(std::clamp(delta, 0.0, 0.1));
        impl_->camera.SetCameraPos(pose.position);
        impl_->camera.SetYaw(pose.yaw); impl_->camera.SetPitch(pose.pitch);
        TransformSystem::Update(impl_->registry);
    }
    void Scene::Draw(int width, int height, const InputSnapshot& input, CursorMode cursor) {
        glViewport(0,0,width,height);
        glDisable(GL_SCISSOR_TEST); glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f,0.13f,0.17f,1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        const auto matrix = impl_->camera.GetCameraProjMat(static_cast<float>(width) / static_cast<float>(height)) * impl_->camera.GetCameraViewMat();
        glUseProgram(impl_->program); glUniformMatrix4fv(impl_->matrix,1,GL_FALSE,glm::value_ptr(matrix));
        glBindVertexArray(impl_->vao); glDrawArrays(GL_TRIANGLES,0,impl_->vertices);
        glEnable(GL_SCISSOR_TEST);
        for (std::size_t index = 0; index < input.keys.size() + 2; ++index) {
            const bool on = index < input.keys.size() ? input.keys[index] : index == input.keys.size() ? input.left_button : input.right_button;
            glScissor(12 + static_cast<int>(index)*25,height - 22,18,10);
            glClearColor(on ? 0.35f : 0.16f,on ? 0.95f : 0.22f,on ? 0.65f : 0.27f,1);
            glClear(GL_COLOR_BUFFER_BIT);
        }
        glScissor(width/2-5,height/2-1,10,2); glClearColor(1,1,1,1); glClear(GL_COLOR_BUFFER_BIT);
        glScissor(width/2-1,height/2-5,2,10); glClear(GL_COLOR_BUFFER_BIT);
        glScissor(width-30,height-22,18,10); glClearColor(cursor == CursorMode::Lock ? 0.95f : 0.3f,0.72f,0.3f,1); glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
        Require(glGetError() == GL_NO_ERROR, "Comparison draw GL error");
    }
    std::string Scene::Title(const InputSnapshot& input, bool focused, bool minimized, CursorMode cursor) const {
        std::ostringstream title;
        title << SYMOCRAFT_COMPARISON_BACKEND << " | " << CursorName(cursor) << " | focus=" << focused << " min=" << minimized << " | keys=";
        constexpr const char* names[]{"Esc","Shift","Caps","Ctrl","W","S","D","A","Space","E","Q"};
        for (std::size_t index = 0; index < input.keys.size(); ++index)
            if (input.keys[index]) title << names[index] << ' ';
        title << "| L=" << input.left_button << " R=" << input.right_button << std::fixed << std::setprecision(1)
              << " | yaw=" << impl_->camera.GetYaw() << " pitch=" << impl_->camera.GetPitch() << " fov=" << impl_->camera.GetFov();
        return title.str();
    }
    void Scene::SaveFrame(const std::filesystem::path& path, int width, int height) const {
        Require(width > 0 && height > 0, "Invalid screenshot size");
        const int row_bytes = (width * 3 + 3) & ~3;
        std::vector<unsigned char> pixels(static_cast<std::size_t>(row_bytes) * height);
        glPixelStorei(GL_PACK_ALIGNMENT,4);
        glReadBuffer(GL_BACK); glReadPixels(0,0,width,height,GL_BGR,GL_UNSIGNED_BYTE,pixels.data());
        Require(glGetError() == GL_NO_ERROR, "Comparison frame readback failed");
        unsigned char lowest = 255, highest = 0;
        for (const auto value : pixels) { lowest = std::min(lowest,value); highest = std::max(highest,value); }
        Require(highest - lowest > 32, "Comparison scene readback is blank");
        std::ofstream file(path,std::ios::binary);
        const auto word = [&file](std::uint32_t value, unsigned bytes) {
            for (unsigned index = 0; index < bytes; ++index) file.put(static_cast<char>((value >> (index*8)) & 255));
        };
        file.put('B'); file.put('M'); word(static_cast<std::uint32_t>(54+pixels.size()),4); word(0,4); word(54,4);
        word(40,4); word(static_cast<std::uint32_t>(width),4); word(static_cast<std::uint32_t>(height),4);
        word(1,2); word(24,2); word(0,4); word(static_cast<std::uint32_t>(pixels.size()),4);
        word(0,4); word(0,4); word(0,4); word(0,4);
        file.write(reinterpret_cast<const char*>(pixels.data()),static_cast<std::streamsize>(pixels.size())); file.flush();
        Require(file.good(), "Comparison frame write failed");
    }
    bool Controls::Edge(int virtual_key, bool focused, bool& previous) {
        if (!focused) {
            previous = true; // After refocus, require release before another diagnostic command.
            return false;
        }
        const bool down = (GetKeyState(virtual_key) & 0x8000) != 0;
        const bool edge = down && !previous;
        previous = down;
        return edge;
    }
    bool Controls::ChangeCursor(bool focused) { return Edge(VK_F1,focused,f1_); }
    bool Controls::ResetCamera(bool focused) { return Edge(VK_F2,focused,f2_); }
    bool Controls::ResetInput(bool focused) { return Edge(VK_F3,focused,f3_); }
}
