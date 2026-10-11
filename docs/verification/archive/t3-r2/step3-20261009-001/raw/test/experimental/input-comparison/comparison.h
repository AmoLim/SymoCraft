#pragma once

#include <symocraft/platform/window.h>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

namespace SymoCraft::Experimental::Comparison {
    struct Options {
        std::filesystem::path output;
        int frames{};
        CursorMode cursor{CursorMode::Lock};
        bool system_scale{true};
    };
    Options Parse(int argc, char** argv);
    void Require(bool condition, const char* operation);
    const char* CursorName(CursorMode mode);
    CursorMode NextCursor(CursorMode mode);
    unsigned KeyMask(const InputSnapshot& input);
    struct Pose { float yaw{}, pitch{}, fov{}, x{}, y{}, z{}; };

    class Session {
    public:
        explicit Session(const Options& options);
        ~Session();
        void Record(double time, const InputSnapshot& input, bool focused, bool minimized, CursorMode cursor, const Pose& pose, unsigned controls);
        void Finish(bool normal, unsigned rendered_frames);
        const Options& Settings() const { return options_; }
        unsigned Frame() const { return frame_; }
    private:
        Options options_;
        std::ofstream frames_, events_;
        unsigned frame_{};
        bool finished_{};
    };

    class Scene {
    public:
        Scene();
        ~Scene();
        Scene(const Scene&) = delete;
        Scene& operator=(const Scene&) = delete;
        void Update(const InputSnapshot& input, double delta);
        void ResetCamera();
        void Draw(int width, int height, const InputSnapshot& input, CursorMode cursor);
        std::string Title(const InputSnapshot& input, bool focused, bool minimized, CursorMode cursor) const;
        Pose CameraPose() const;
        void SaveFrame(const std::filesystem::path& path, int width, int height) const;
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    struct Controls {
        bool ChangeCursor(bool focused);
        bool ResetCamera(bool focused);
        bool ResetInput(bool focused);
    private:
        bool Edge(int virtual_key, bool focused, bool& previous);
        bool f1_{}, f2_{}, f3_{};
    };
}
