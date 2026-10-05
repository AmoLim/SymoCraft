#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace SymoCraft {
    namespace Platform { struct GraphicsBridge; }
    enum class CursorMode : std::uint8_t { Hidden, Lock, Normal };
    enum class Key : std::size_t {
        Escape, LeftShift, CapsLock, LeftControl, W, S, D, A, Space, E, Q, Count
    };
    struct PointerEvent {
        double mouse_dx{}, mouse_dy{}, scroll_y{};
    };
    struct InputSnapshot {
        std::array<bool, static_cast<std::size_t>(Key::Count)> keys{};
        bool left_button{}, right_button{}, focus_lost{};
        std::vector<PointerEvent> pointer_events;
        bool Down(Key key) const { return keys[static_cast<std::size_t>(key)]; }
    };
    class Window {
    public:
        Window();
        ~Window();
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        int width{}, height{};
        void PollInt();
        void CaptureInput();
        const InputSnapshot& Input() const;
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
        static Window* Create(const char* title, int width = 0, int height = 0,
                              bool benchmark_window = false, bool allow_unfocused = false);
        static void Init();
        static void Free();
    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
        friend struct Platform::GraphicsBridge;
    };
}
