#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace SymoCraft {
    namespace Platform {
        struct GraphicsBridge; struct NativeBridge; struct VulkanBridge;
        class Runtime final {
        public:
            Runtime();
            ~Runtime() noexcept;
            Runtime(const Runtime&) = delete;
            Runtime& operator=(const Runtime&) = delete;
            void Shutdown();
            bool Active() const noexcept;
        private:
            bool acquired_ = false;
        };
    }
    enum class CursorMode : std::uint8_t { Hidden, Lock, Normal };
    enum class WindowMode : std::uint8_t { OpenGL, Native, Vulkan };
    enum class Key : std::size_t {
        Escape, LeftShift, CapsLock, LeftControl, W, S, D, A, Space, E, Q, Count
    };
    struct PointerEvent {
        double mouse_dx{}, mouse_dy{}, scroll_y{};
    };
    struct InputFrame {
        std::array<bool, static_cast<std::size_t>(Key::Count)> held{}, pressed{};
        bool left_button{}, right_button{}, focus_lost{};
        // The count constructor can propagate MSVC Debug iterator-proxy allocation failure.
        std::vector<PointerEvent> pointer_events = std::vector<PointerEvent>(0);
        bool Down(Key key) const { const auto index = static_cast<std::size_t>(key); return held[index] || pressed[index]; }
        void Clear() {
            held.fill(false); pressed.fill(false);
            left_button = right_button = focus_lost = false;
            pointer_events.clear();
        }
        bool Empty() const {
            for (std::size_t index = 0; index < held.size(); ++index) if (held[index] || pressed[index]) return false;
            return !left_button && !right_button && !focus_lost && pointer_events.empty();
        }
    };
    class Window {
    public:
        ~Window() noexcept;
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        int Width() const noexcept { return width_; }
        int Height() const noexcept { return height_; }
        void PollInt();
        void CaptureInput(InputFrame& destination);
        bool Focused() const;
        bool Minimized() const;
        bool ShouldClose() const;
        void Close();
        void Destroy();
        void ResetInput();
        void WaitEvents(double timeout_seconds);
        void SetCursorMode(CursorMode mode);
        void SetTitle(const char* title);
        void SetSize(int width, int height);
        float GetAspectRatio() const;
        static double Time();
        static std::unique_ptr<Window> Create(Platform::Runtime& runtime, const char* title,
            int width = 0, int height = 0, bool benchmark_window = false,
            bool allow_unfocused = false, WindowMode mode = WindowMode::OpenGL);
    private:
        Window();
        static void Init();
        static void Free();
        static Window* Create(const char* title, int width, int height,
                              bool benchmark_window, bool allow_unfocused, WindowMode mode);
        struct Impl;
        std::unique_ptr<Impl> impl_;
        int width_{}, height_{};
        friend struct Platform::GraphicsBridge;
        friend struct Platform::NativeBridge;
        friend struct Platform::VulkanBridge;
        friend class Platform::Runtime;
    };
}
