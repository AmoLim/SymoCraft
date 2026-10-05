#include <symocraft/platform/window.h>
#include <symocraft/platform/key_snapshot.h>
#include <symocraft/foundation/diagnostics.h>
#include "graphics_bridge/graphics_bridge.h"
#define GLFW_INCLUDE_NONE
#define NOMINMAX
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <algorithm>
#include <climits>
#include <cstdio>
#include <iostream>
#include <exception>
#include <stdexcept>
#include <string>
#include <system_error>

namespace SymoCraft {
    struct Window::Impl {
        GLFWwindow* native{};
        InputSnapshot input;
        float last_x{}, last_y{};
        bool first_cursor{true}, pending_focus_loss{}, benchmark{};
        std::vector<PointerEvent> pending_pointer_events;
        std::exception_ptr input_error;
        Impl() {
            input.pointer_events.reserve(64);
            pending_pointer_events.reserve(64);
        }
    };
    namespace {
        bool initialized{};
        void ErrorCallback(int code, const char* description) {
            std::fprintf(stderr, "GLFW error %d: %s\n", code, description ? description : "unknown");
        }
        std::runtime_error Failure(const char* operation) {
            const char* description{};
            glfwGetError(&description);
            return std::runtime_error(std::string(operation) + ": " +
                (description ? description : "no additional GLFW detail"));
        }
    }
    Window::Window() : impl_(std::make_unique<Impl>()) {}
    Window::~Window() { Destroy(); }
    void Window::Init() {
        if (initialized) return;
        glfwSetErrorCallback(ErrorCallback);
        if (!glfwInit()) throw Failure("Failed to initialize GLFW");
        initialized = true;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4);
#ifndef NDEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
#endif
    }
    void Window::Free() {
        if (initialized) { glfwTerminate(); initialized = false; }
    }
    Window* Window::Create(const char* title, int requested_width, int requested_height,
                           bool benchmark, bool allow_unfocused) {
        auto result = std::make_unique<Window>();
        result->impl_->benchmark = benchmark;
        auto* monitor = glfwGetPrimaryMonitor();
        if (!monitor) throw Failure("Failed to get primary monitor");
        const auto* mode = glfwGetVideoMode(monitor);
        if (!mode) throw Failure("Failed to get video mode of primary monitor");
        AmoLogger_Info("Monitor size: %d, %d", mode->width, mode->height);
        result->width = requested_width > 0 ? requested_width : std::clamp(mode->width / 2, 800, INT_MAX);
        result->height = requested_height > 0 ? requested_height : std::clamp(mode->height / 2, 600, INT_MAX);
        glfwWindowHint(GLFW_RESIZABLE, benchmark ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, benchmark ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_FALSE);
        glfwWindowHint(GLFW_VISIBLE, benchmark ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUSED, allow_unfocused ? GLFW_FALSE : GLFW_TRUE);
        glfwWindowHint(GLFW_FOCUS_ON_SHOW, allow_unfocused ? GLFW_FALSE : GLFW_TRUE);
        auto* native = glfwCreateWindow(result->width, result->height, title, nullptr, nullptr);
        result->impl_->native = native;
        if (!native) throw Failure("Failed to create an OpenGL 4.6 window");
        if (benchmark && !SetPropW(glfwGetWin32Window(native), L"NonRudeHWND",
                                   reinterpret_cast<HANDLE>(static_cast<INT_PTR>(TRUE))))
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(),
                                    "Failed to preserve the taskbar for the benchmark window");
        glfwSetWindowUserPointer(native, result.get());
        Platform::GraphicsBridge::MakeCurrent(*result);
        if (glfwGetCurrentContext() != native) throw Failure("Failed to make the OpenGL context current");
        int x{}, y{}, width{}, height{};
        glfwGetMonitorPos(monitor, &x, &y);
        glfwGetWindowSize(native, &width, &height);
        glfwSetWindowPos(native, x + (mode->width - width) / 2, y + (mode->height - height) / 2);
        Platform::GraphicsBridge::SetVsync(true);
        if (benchmark) glfwShowWindow(native);
        glfwGetFramebufferSize(native, &result->width, &result->height);
        glfwSetFramebufferSizeCallback(native, [](GLFWwindow* window, int w, int h) {
            auto& self = *static_cast<Window*>(glfwGetWindowUserPointer(window));
            self.width = w; self.height = h;
        });
        glfwSetWindowFocusCallback(native, [](GLFWwindow* window, int focused) {
            auto& state = static_cast<Window*>(glfwGetWindowUserPointer(window))->impl_;
            if (!focused) { state->pending_focus_loss = true; state->first_cursor = true; }
        });
        if (!benchmark) glfwSetCursorPosCallback(native, [](GLFWwindow* window, double x, double y) {
            auto& state = static_cast<Window*>(glfwGetWindowUserPointer(window))->impl_;
            if (!glfwGetWindowAttrib(window, GLFW_FOCUSED)) { state->first_cursor = true; return; }
            const auto current_x = static_cast<float>(x), current_y = static_cast<float>(y);
            if (!state->first_cursor) {
                try {
                    state->pending_pointer_events.push_back({current_x - state->last_x, state->last_y - current_y, 0});
                } catch (...) { state->input_error = std::current_exception(); }
            }
            state->last_x = current_x; state->last_y = current_y; state->first_cursor = false;
        });
        if (!benchmark) glfwSetScrollCallback(native, [](GLFWwindow* window, double, double y) {
            auto& state = static_cast<Window*>(glfwGetWindowUserPointer(window))->impl_;
            try { state->pending_pointer_events.push_back({0, 0, y}); }
            catch (...) { state->input_error = std::current_exception(); }
        });
        glfwSetInputMode(native, GLFW_STICKY_KEYS, GLFW_TRUE);
        AmoLogger_Info("Window created. ");
        if (benchmark)
            std::cout << "Benchmark window: borderless-windowed; topmost=false; taskbar_policy=NonRudeHWND; framebuffer="
                      << result->width << 'x' << result->height << std::endl;
        return result.release();
    }
    void Window::PollInt() {
        glfwPollEvents();
        if (impl_->input_error) std::rethrow_exception(impl_->input_error);
        auto& state = *impl_;
        state.input.pointer_events.clear();
        state.input.pointer_events.swap(state.pending_pointer_events);
        state.input.focus_lost = state.pending_focus_loss;
        state.pending_focus_loss = false;
    }
    void Window::CaptureInput() {
        constexpr std::array<int, static_cast<std::size_t>(Key::Count)> codes{
            GLFW_KEY_ESCAPE, GLFW_KEY_LEFT_SHIFT, GLFW_KEY_CAPS_LOCK, GLFW_KEY_LEFT_CONTROL,
            GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_D, GLFW_KEY_A, GLFW_KEY_SPACE, GLFW_KEY_E, GLFW_KEY_Q};
        auto& state = *impl_;
        static_assert(KeySampling::Count == codes.size());
        if (state.benchmark) {
            state.input.keys[0] = glfwGetKey(state.native, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        } else {
            state.input.keys = KeySampling::Capture([&](KeySampling::Control control) {
                return glfwGetKey(state.native, codes[static_cast<std::size_t>(control)]) == GLFW_PRESS;
            }).pressed;
            state.input.left_button = glfwGetMouseButton(state.native, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            state.input.right_button = glfwGetMouseButton(state.native, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
        }
    }
    const InputSnapshot& Window::Input() const { return impl_->input; }
    bool Window::Focused() const { return glfwGetWindowAttrib(impl_->native, GLFW_FOCUSED) != 0; }
    bool Window::Minimized() const { return glfwGetWindowAttrib(impl_->native, GLFW_ICONIFIED) != 0; }
    bool Window::ShouldClose() const { return !impl_->native || glfwWindowShouldClose(impl_->native); }
    void Window::Close() { if (impl_->native) glfwSetWindowShouldClose(impl_->native, true); }
    void Window::Destroy() {
        if (!impl_->native) return;
        RemovePropW(glfwGetWin32Window(impl_->native), L"NonRudeHWND");
        glfwDestroyWindow(impl_->native); impl_->native = nullptr;
    }
    void Window::ResetInput() {
        glfwSetInputMode(impl_->native, GLFW_STICKY_KEYS, GLFW_FALSE);
        glfwSetInputMode(impl_->native, GLFW_STICKY_KEYS, GLFW_TRUE);
        impl_->input.keys.fill(false);
        impl_->input.left_button = impl_->input.right_button = impl_->input.focus_lost = false;
        impl_->input.pointer_events.clear();
        impl_->pending_pointer_events.clear();
        impl_->first_cursor = true;
    }
    void Window::WaitEvents(double seconds) { glfwWaitEventsTimeout(seconds); }
    void Window::SetCursorMode(CursorMode mode) {
        glfwSetInputMode(impl_->native, GLFW_CURSOR, mode == CursorMode::Lock ? GLFW_CURSOR_DISABLED :
                         mode == CursorMode::Hidden ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
    }
    void Window::SetTitle(const char* title) { glfwSetWindowTitle(impl_->native, title); }
    void Window::SetSize(int w, int h) { glfwSetWindowSize(impl_->native, w, h); }
    float Window::GetAspectRatio() const { return static_cast<float>(std::max(width, 1)) / std::max(height, 1); }
    double Window::Time() { return glfwGetTime(); }
    void Platform::GraphicsBridge::MakeCurrent(Window& window) { glfwMakeContextCurrent(window.impl_->native); }
    bool Platform::GraphicsBridge::HasCurrentContext() { return glfwGetCurrentContext() != nullptr; }
    Platform::GraphicsBridge::Procedure Platform::GraphicsBridge::GetProcedure(const char* name) { return glfwGetProcAddress(name); }
    bool Platform::GraphicsBridge::ExtensionSupported(const char* name) { return glfwExtensionSupported(name) != 0; }
    void Platform::GraphicsBridge::SetVsync(bool enabled) { glfwSwapInterval(enabled ? 1 : 0); }
    void Platform::GraphicsBridge::Present(Window& window) { glfwSwapBuffers(window.impl_->native); }
}
