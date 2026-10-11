#include <symocraft/platform/window.h>
#include <symocraft/foundation/diagnostics.h>
#include "graphics_bridge/graphics_bridge.h"
#include "graphics_bridge/native_bridge.h"
#include "graphics_bridge/vulkan_bridge.h"
#include "input_state.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <exception>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>

namespace SymoCraft {
    namespace {
        bool initialized{};
        std::size_t active_windows{};
        Uint64 timer_origin{}, timer_frequency{};

        std::runtime_error Failure(const char* operation) {
            const char* detail = SDL_GetError();
            return std::runtime_error(std::string(operation) + ": " +
                (detail && *detail ? detail : "no additional SDL detail"));
        }
        void Require(bool success, const char* operation) {
            if (!success) throw Failure(operation);
        }
        void ReportCleanupFailure(const std::exception_ptr& error) noexcept {
            try { std::rethrow_exception(error); }
            catch (const std::exception& detail) { std::fprintf(stderr, "Window secondary cleanup failure: %s\n", detail.what()); }
            catch (...) { std::fprintf(stderr, "Window secondary cleanup failure: unknown failure\n"); }
        }
        void RequireVideo(const char* operation) {
            if (!initialized) throw std::logic_error(std::string(operation) + ": Window::Init is required");
            if (!SDL_IsMainThread()) throw std::logic_error(std::string(operation) + ": main thread is required");
        }
        void RequireCurrentContext(const char* operation) {
            RequireVideo(operation);
            if (!SDL_GL_GetCurrentContext())
                throw std::logic_error(std::string(operation) + ": no current OpenGL context");
        }
        void ConfigureOpenGL() {
            SDL_GL_ResetAttributes();
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4), "SDL_GL_SetAttribute(major)");
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6), "SDL_GL_SetAttribute(minor)");
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE), "SDL_GL_SetAttribute(core)");
            Require(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1), "SDL_GL_SetAttribute(doublebuffer)");
            Require(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24), "SDL_GL_SetAttribute(depth)");
            Require(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1), "SDL_GL_SetAttribute(sample_buffers)");
            Require(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4), "SDL_GL_SetAttribute(samples)");
#ifndef NDEBUG
            Require(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG), "SDL_GL_SetAttribute(debug)");
#endif
        }
        void VerifyOpenGL() {
            int major{}, minor{}, profile{}, buffers{}, samples{}, flags{};
            Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &major), "SDL_GL_GetAttribute(major)");
            Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minor), "SDL_GL_GetAttribute(minor)");
            Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &profile), "SDL_GL_GetAttribute(profile)");
            Require(SDL_GL_GetAttribute(SDL_GL_MULTISAMPLEBUFFERS, &buffers), "SDL_GL_GetAttribute(sample_buffers)");
            Require(SDL_GL_GetAttribute(SDL_GL_MULTISAMPLESAMPLES, &samples), "SDL_GL_GetAttribute(samples)");
            Require(SDL_GL_GetAttribute(SDL_GL_CONTEXT_FLAGS, &flags), "SDL_GL_GetAttribute(flags)");
            if (major < 4 || (major == 4 && minor < 6) || profile != SDL_GL_CONTEXT_PROFILE_CORE ||
                buffers != 1 || samples != 4)
                throw std::runtime_error("Window::Create: OpenGL 4.6 Core / 4x MSAA requirements were not met");
#ifndef NDEBUG
            if (!(flags & SDL_GL_CONTEXT_DEBUG_FLAG))
                throw std::runtime_error("Window::Create: the requested Debug OpenGL context was not provided");
#endif
            const auto get_integer = reinterpret_cast<decltype(&glGetIntegerv)>(SDL_GL_GetProcAddress("glGetIntegerv"));
            const auto get_error = reinterpret_cast<decltype(&glGetError)>(SDL_GL_GetProcAddress("glGetError"));
            Require(get_integer && get_error, "Window::Create(actual GL attribute procedures)");
            get_integer(GL_MAJOR_VERSION, &major);
            get_integer(GL_MINOR_VERSION, &minor);
            get_integer(GL_CONTEXT_PROFILE_MASK, &profile);
            get_integer(GL_CONTEXT_FLAGS, &flags);
            get_integer(GL_SAMPLES, &samples);
            const auto error = get_error();
            if (error != GL_NO_ERROR)
                throw std::runtime_error("Window::Create: actual GL attribute query failed; GL error=" + std::to_string(error));
            if (major < 4 || (major == 4 && minor < 6) || !(profile & GL_CONTEXT_CORE_PROFILE_BIT) || samples != 4)
                throw std::runtime_error("Window::Create: driver-reported OpenGL 4.6 Core / 4x MSAA requirements were not met");
#ifndef NDEBUG
            if (!(flags & GL_CONTEXT_FLAG_DEBUG_BIT))
                throw std::runtime_error("Window::Create: driver did not provide the requested Debug OpenGL context");
#endif
        }
    }

    struct Window::Impl {
        SDL_Window* native{};
        SDL_GLContext context{};
        WindowMode mode{WindowMode::OpenGL};
        bool benchmark{};
        HWND non_rude_window{};
        std::unique_ptr<Platform::Detail::InputState> input;
        std::exception_ptr input_error;

        SDL_Window* RequireNative(const char* operation) const {
            RequireVideo(operation);
            if (!native) throw std::logic_error(std::string(operation) + ": window has been destroyed");
            return native;
        }
        void RequireMode(WindowMode expected, const char* operation) const {
            RequireNative(operation);
            if (mode != expected) throw std::logic_error(std::string(operation) + ": incompatible window mode");
        }
        void Handle(Window& window, const SDL_Event& event) {
            if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST &&
                event.window.windowID == SDL_GetWindowID(native) &&
                (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type == SDL_EVENT_WINDOW_RESIZED ||
                 event.type == SDL_EVENT_WINDOW_MINIMIZED || event.type == SDL_EVENT_WINDOW_RESTORED))
                Require(SDL_GetWindowSizeInPixels(native, &window.width, &window.height), "SDL_GetWindowSizeInPixels(event)");
            try { input->Handle(event); }
            catch (...) {
                if (!input_error) {
                    try { std::throw_with_nested(std::runtime_error("Window input event adaptation failed")); }
                    catch (...) { input_error = std::current_exception(); }
                }
            }
            CheckInputError();
        }
        void CheckInputError() const {
            if (input_error) std::rethrow_exception(input_error);
        }
        void DrainEvents(Window& window, const char* operation) {
            SDL_Event event{};
            for (;;) {
                SDL_ClearError();
                if (!SDL_PollEvent(&event)) {
                    if (*SDL_GetError()) throw Failure(operation);
                    break;
                }
                Handle(window, event);
            }
            CheckInputError();
        }
        void SynchronizeWindowState() {
            const auto flags = SDL_GetWindowFlags(native);
            input->SynchronizeWindowState(SDL_GetWindowID(native),
                (flags & SDL_WINDOW_INPUT_FOCUS) != 0, (flags & SDL_WINDOW_MINIMIZED) != 0);
        }
        void SynchronizeInput() {
            Platform::Detail::PhysicalInputState physical;
            physical.window_id = SDL_GetWindowID(native);
            const auto flags = SDL_GetWindowFlags(native);
            physical.focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
            physical.minimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
            input->SynchronizeWindowState(physical.window_id, physical.focused, physical.minimized);
            if (physical.focused && !physical.minimized) {
                int count{};
                const bool* keys = SDL_GetKeyboardState(&count);
                Require(keys != nullptr, "SDL_GetKeyboardState");
                for (std::size_t index = 0; index < physical.keys.size(); ++index) {
                    const auto scancode = static_cast<int>(Platform::Detail::ControlScancodes[index]);
                    physical.keys[index] = scancode < count && keys[scancode];
                }
                const auto buttons = SDL_GetMouseState(nullptr, nullptr);
                physical.left_button = (buttons & SDL_BUTTON_LMASK) != 0;
                physical.right_button = (buttons & SDL_BUTTON_RMASK) != 0;
            }
            input->SynchronizePhysicalState(physical);
        }
    };

    Window::Window() : impl_(std::make_unique<Impl>()) {}
    Window::~Window() noexcept {
        try { Destroy(); }
        catch (const std::exception& error) { std::fprintf(stderr, "Window destructor cleanup: %s\n", error.what()); }
        catch (...) { std::fprintf(stderr, "Window destructor cleanup: unknown failure\n"); }
    }
    void Window::Init() {
        if (initialized) { RequireVideo("Window::Init"); return; }
        SDL_SetMainReady();
        if (!SDL_IsMainThread()) throw std::logic_error("Window::Init: main thread is required");
        Require(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL_InitSubSystem(video)");
        timer_origin = SDL_GetPerformanceCounter();
        timer_frequency = SDL_GetPerformanceFrequency();
        if (!timer_frequency) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            throw std::runtime_error("Window::Init: SDL performance timer frequency is zero");
        }
        initialized = true;
    }
    void Window::Free() {
        if (!initialized) return;
        RequireVideo("Window::Free");
        if (active_windows) throw std::logic_error("Window::Free: destroy platform windows before releasing SDL video");
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        initialized = false;
        timer_origin = timer_frequency = 0;
    }
    Window* Window::Create(const char* title, int requested_width, int requested_height,
                           bool benchmark, bool allow_unfocused, WindowMode mode) {
        RequireVideo("Window::Create");
        if (active_windows) throw std::logic_error("Window::Create: only one platform window is supported");
        if (!title) throw std::invalid_argument("Window::Create: title must not be null");
        if (mode != WindowMode::OpenGL && mode != WindowMode::Native && mode != WindowMode::Vulkan)
            throw std::invalid_argument("Window::Create: unknown window mode");
        std::unique_ptr<Window> result;
        try { result = std::make_unique<Window>(); }
        catch (const std::bad_alloc&) {
            std::throw_with_nested(std::runtime_error("Window::Create: Window/Impl allocation failed"));
        }
        auto& state = *result->impl_;
        state.mode = mode;
        state.benchmark = benchmark;
        const auto display = SDL_GetPrimaryDisplay();
        Require(display != 0, "SDL_GetPrimaryDisplay");
        const auto* display_mode = SDL_GetCurrentDisplayMode(display);
        Require(display_mode != nullptr, "SDL_GetCurrentDisplayMode");
        AmoLogger_Info("Monitor size: %d, %d", display_mode->w, display_mode->h);
        const int logical_width = requested_width > 0 ? requested_width : std::max(display_mode->w / 2, 800);
        const int logical_height = requested_height > 0 ? requested_height : std::max(display_mode->h / 2, 600);
        Require(SDL_SetHintWithPriority(SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN, allow_unfocused ? "0" : "1", SDL_HINT_OVERRIDE),
                "SDL_SetHint(window_activate_when_shown)");
        Require(SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, "1", SDL_HINT_OVERRIDE),
                "SDL_SetHint(mouse_relative_system_scale)");
        SDL_WindowFlags flags = SDL_WINDOW_HIDDEN | (benchmark ? SDL_WINDOW_BORDERLESS : SDL_WINDOW_RESIZABLE);
        if (mode == WindowMode::OpenGL) { ConfigureOpenGL(); flags |= SDL_WINDOW_OPENGL; }
        else if (mode == WindowMode::Vulkan) flags |= SDL_WINDOW_VULKAN;
        state.native = SDL_CreateWindow(title, logical_width, logical_height, flags);
        Require(state.native != nullptr, "SDL_CreateWindow");
        ++active_windows;
        Require(SDL_SetWindowPosition(state.native, static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(display)),
                static_cast<int>(SDL_WINDOWPOS_CENTERED_DISPLAY(display))), "SDL_SetWindowPosition(centered)");
        if (benchmark) {
            const HWND handle = Platform::NativeBridge::GetHandle(*result);
            if (!SetPropW(handle, L"NonRudeHWND", reinterpret_cast<HANDLE>(static_cast<INT_PTR>(TRUE))))
                throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "SetPropW(NonRudeHWND)");
            state.non_rude_window = handle;
        }
        if (mode == WindowMode::OpenGL) {
            state.context = SDL_GL_CreateContext(state.native);
            Require(state.context != nullptr, "SDL_GL_CreateContext");
            Platform::GraphicsBridge::MakeCurrent(*result);
            VerifyOpenGL();
            Platform::GraphicsBridge::SetVsync(true);
        }
        Require(SDL_ShowWindow(state.native), "SDL_ShowWindow");
        Require(SDL_GetWindowSizeInPixels(state.native, &result->width, &result->height), "SDL_GetWindowSizeInPixels(create)");
        if (benchmark && (result->width != logical_width || result->height != logical_height))
            throw std::runtime_error("Window::Create: benchmark drawable pixels do not match the requested workload dimensions");
        const bool focused = (SDL_GetWindowFlags(state.native) & SDL_WINDOW_INPUT_FOCUS) != 0;
        try { state.input = std::make_unique<Platform::Detail::InputState>(SDL_GetWindowID(state.native), benchmark, focused); }
        catch (...) { std::throw_with_nested(std::runtime_error("Window::Create: input buffers could not be allocated")); }
        AmoLogger_Info("Window created. ");
        if (benchmark)
            std::cout << "Benchmark window: borderless-windowed; topmost=false; taskbar_policy=NonRudeHWND; framebuffer="
                      << result->width << 'x' << result->height << std::endl;
        return result.release();
    }
    void Window::PollInt() {
        impl_->RequireNative("Window::PollInt");
        impl_->CheckInputError();
        impl_->DrainEvents(*this, "SDL_PollEvent");
        impl_->SynchronizeWindowState();
        impl_->input->PublishEvents();
    }
    void Window::CaptureInput() {
        impl_->RequireNative("Window::CaptureInput");
        impl_->CheckInputError();
        impl_->SynchronizeInput();
        impl_->input->CaptureKeys();
    }
    const InputSnapshot& Window::Input() const {
        impl_->RequireNative("Window::Input");
        return impl_->input->Input();
    }
    bool Window::Focused() const {
        return impl_->native && (SDL_GetWindowFlags(impl_->RequireNative("Window::Focused")) & SDL_WINDOW_INPUT_FOCUS) != 0;
    }
    bool Window::Minimized() const {
        return impl_->native && (SDL_GetWindowFlags(impl_->RequireNative("Window::Minimized")) & SDL_WINDOW_MINIMIZED) != 0;
    }
    bool Window::ShouldClose() const {
        if (!impl_->native) return true;
        impl_->RequireNative("Window::ShouldClose");
        return impl_->input->ShouldClose();
    }
    void Window::Close() {
        if (!impl_->native) return;
        impl_->RequireNative("Window::Close");
        impl_->input->Close();
    }
    void Window::Destroy() {
        if (!impl_->native) return;
        RequireVideo("Window::Destroy");
        std::exception_ptr first_error;
        if (impl_->context) {
            try { Require(SDL_GL_DestroyContext(impl_->context), "SDL_GL_DestroyContext"); }
            catch (...) { first_error = std::current_exception(); }
            impl_->context = nullptr;
        }
        if (impl_->non_rude_window) {
            try {
                SetLastError(ERROR_SUCCESS);
                if (!RemovePropW(impl_->non_rude_window, L"NonRudeHWND")) {
                    const DWORD code = GetLastError();
                    if (code)
                        throw std::system_error(static_cast<int>(code), std::system_category(), "RemovePropW(NonRudeHWND)");
                    throw std::runtime_error("RemovePropW(NonRudeHWND): owned window property was not present");
                }
            } catch (...) {
                const auto error = std::current_exception();
                if (!first_error) first_error = error;
                else ReportCleanupFailure(error);
            }
            impl_->non_rude_window = nullptr;
        }
        SDL_DestroyWindow(impl_->native);
        impl_->native = nullptr;
        impl_->input.reset();
        impl_->input_error = nullptr;
        width = height = 0;
        --active_windows;
        if (first_error) std::rethrow_exception(first_error);
    }
    void Window::ResetInput() {
        impl_->RequireNative("Window::ResetInput");
        impl_->CheckInputError();
        impl_->SynchronizeWindowState();
        impl_->input->ResetInput();
    }
    void Window::WaitEvents(double seconds) {
        impl_->RequireNative("Window::WaitEvents");
        if (!std::isfinite(seconds) || seconds < 0)
            throw std::invalid_argument("Window::WaitEvents: timeout must be finite and nonnegative");
        impl_->CheckInputError();
        const int milliseconds = static_cast<int>(std::ceil(std::min(seconds * 1000.0, static_cast<double>(INT_MAX))));
        SDL_Event event{};
        SDL_ClearError();
        if (SDL_WaitEventTimeout(&event, milliseconds)) impl_->Handle(*this, event);
        else if (*SDL_GetError()) throw Failure("SDL_WaitEventTimeout");
        impl_->DrainEvents(*this, "SDL_PollEvent(wait drain)");
        impl_->SynchronizeWindowState();
    }
    void Window::SetCursorMode(CursorMode mode) {
        auto* native = impl_->RequireNative("Window::SetCursorMode");
        if (mode != CursorMode::Lock && mode != CursorMode::Hidden && mode != CursorMode::Normal)
            throw std::invalid_argument("Window::SetCursorMode: unknown cursor mode");
        Require(SDL_SetWindowRelativeMouseMode(native, mode == CursorMode::Lock), "SDL_SetWindowRelativeMouseMode");
        Require(mode == CursorMode::Normal ? SDL_ShowCursor() : SDL_HideCursor(), "SDL_SetCursorVisibility");
        ResetInput();
    }
    void Window::SetTitle(const char* title) {
        auto* native = impl_->RequireNative("Window::SetTitle");
        if (!title) throw std::invalid_argument("Window::SetTitle: title must not be null");
        Require(SDL_SetWindowTitle(native, title), "SDL_SetWindowTitle");
    }
    void Window::SetSize(int w, int h) {
        auto* native = impl_->RequireNative("Window::SetSize");
        if (w <= 0 || h <= 0) throw std::invalid_argument("Window::SetSize: logical dimensions must be positive");
        Require(SDL_SetWindowSize(native, w, h), "SDL_SetWindowSize");
        Require(SDL_GetWindowSizeInPixels(native, &width, &height), "SDL_GetWindowSizeInPixels(resize)");
    }
    float Window::GetAspectRatio() const { return static_cast<float>(std::max(width, 1)) / std::max(height, 1); }
    double Window::Time() {
        if (!initialized) return 0;
        return static_cast<double>(SDL_GetPerformanceCounter() - timer_origin) / static_cast<double>(timer_frequency);
    }

    void Platform::GraphicsBridge::MakeCurrent(Window& window) {
        window.impl_->RequireMode(WindowMode::OpenGL, "GraphicsBridge::MakeCurrent");
        Require(SDL_GL_MakeCurrent(window.impl_->native, window.impl_->context), "SDL_GL_MakeCurrent");
        if (SDL_GL_GetCurrentContext() != window.impl_->context || SDL_GL_GetCurrentWindow() != window.impl_->native)
            throw std::runtime_error("GraphicsBridge::MakeCurrent: the requested context was not made current");
    }
    bool Platform::GraphicsBridge::HasCurrentContext() {
        if (!initialized) return false;
        RequireVideo("GraphicsBridge::HasCurrentContext");
        return SDL_GL_GetCurrentContext() != nullptr;
    }
    Platform::GraphicsBridge::Procedure Platform::GraphicsBridge::GetProcedure(const char* name) {
        RequireCurrentContext("GraphicsBridge::GetProcedure");
        if (!name || !*name) throw std::invalid_argument("GraphicsBridge::GetProcedure: name must not be empty");
        return reinterpret_cast<Procedure>(SDL_GL_GetProcAddress(name));
    }
    bool Platform::GraphicsBridge::ExtensionSupported(const char* name) {
        RequireCurrentContext("GraphicsBridge::ExtensionSupported");
        if (!name || !*name || std::strchr(name, ' '))
            throw std::invalid_argument("GraphicsBridge::ExtensionSupported: invalid extension name");
        SDL_ClearError();
        const auto get_string = reinterpret_cast<decltype(&glGetString)>(SDL_GL_GetProcAddress("glGetString"));
        const auto get_integer = reinterpret_cast<decltype(&glGetIntegerv)>(SDL_GL_GetProcAddress("glGetIntegerv"));
        const auto get_error = reinterpret_cast<decltype(&glGetError)>(SDL_GL_GetProcAddress("glGetError"));
        const auto get_string_index = reinterpret_cast<PFNGLGETSTRINGIPROC>(SDL_GL_GetProcAddress("glGetStringi"));
        if (!get_string || !get_integer || !get_error || !get_string_index)
            throw Failure("GraphicsBridge::ExtensionSupported(required GL procedures)");
        auto check_error = [&] {
            const auto error = get_error();
            if (error != GL_NO_ERROR)
                throw std::runtime_error("GraphicsBridge::ExtensionSupported: GL error=" + std::to_string(error));
        };
        check_error();
        const auto version = get_string(GL_VERSION);
        check_error();
        if (!version) throw std::runtime_error("GraphicsBridge::ExtensionSupported: missing GL version");
        const char* override_hint = SDL_GetHint(name);
        if (override_hint && *override_hint == '0') return false;
        GLint count{};
        get_integer(GL_NUM_EXTENSIONS, &count);
        check_error();
        if (count < 0) throw std::runtime_error("GraphicsBridge::ExtensionSupported: invalid extension count");
        for (GLint index = 0; index < count; ++index) {
            const auto extension = get_string_index(GL_EXTENSIONS, static_cast<GLuint>(index));
            check_error();
            if (!extension) throw std::runtime_error("GraphicsBridge::ExtensionSupported: extension enumeration failed");
            if (std::strcmp(reinterpret_cast<const char*>(extension), name) == 0) return true;
        }
        return false;
    }
    void Platform::GraphicsBridge::SetVsync(bool enabled) {
        RequireCurrentContext("GraphicsBridge::SetVsync");
        Require(SDL_GL_SetSwapInterval(enabled ? 1 : 0), "SDL_GL_SetSwapInterval");
        int actual{};
        Require(SDL_GL_GetSwapInterval(&actual), "SDL_GL_GetSwapInterval");
        if (actual != (enabled ? 1 : 0))
            throw std::runtime_error("GraphicsBridge::SetVsync: the actual swap interval does not match the request");
    }
    void Platform::GraphicsBridge::Present(Window& window) {
        window.impl_->RequireMode(WindowMode::OpenGL, "GraphicsBridge::Present");
        if (SDL_GL_GetCurrentContext() != window.impl_->context || SDL_GL_GetCurrentWindow() != window.impl_->native)
            throw std::logic_error("GraphicsBridge::Present: this window's OpenGL context is not current");
        Require(SDL_GL_SwapWindow(window.impl_->native), "SDL_GL_SwapWindow");
    }
    HWND Platform::NativeBridge::GetHandle(Window& window) {
        auto* native = window.impl_->RequireNative("NativeBridge::GetHandle");
        const auto properties = SDL_GetWindowProperties(native);
        Require(properties != 0, "SDL_GetWindowProperties");
        const auto handle = static_cast<HWND>(SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        if (!handle || !IsWindow(handle)) throw std::runtime_error("NativeBridge::GetHandle: no valid Win32 HWND");
        return handle;
    }
    PFN_vkGetInstanceProcAddr Platform::VulkanBridge::GetInstanceProcedureAddress(Window& window) {
        window.impl_->RequireMode(WindowMode::Vulkan, "VulkanBridge::GetInstanceProcedureAddress");
        const auto get = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
        Require(get != nullptr, "SDL_Vulkan_GetVkGetInstanceProcAddr");
        return get;
    }
    std::vector<std::string> Platform::VulkanBridge::GetRequiredExtensions(Window& window) {
        window.impl_->RequireMode(WindowMode::Vulkan, "VulkanBridge::GetRequiredExtensions");
        Uint32 count{};
        const auto names = SDL_Vulkan_GetInstanceExtensions(&count);
        Require(names != nullptr && count != 0, "SDL_Vulkan_GetInstanceExtensions");
        try {
            std::vector<std::string> result;
            result.reserve(count);
            for (Uint32 index = 0; index < count; ++index) result.emplace_back(names[index]);
            return result;
        } catch (...) { std::throw_with_nested(std::runtime_error("VulkanBridge::GetRequiredExtensions: extension copy allocation failed")); }
    }
    VkSurfaceKHR Platform::VulkanBridge::CreateSurface(Window& window, VkInstance instance,
                                                     const VkAllocationCallbacks* allocator) {
        window.impl_->RequireMode(WindowMode::Vulkan, "VulkanBridge::CreateSurface");
        if (!instance) throw std::invalid_argument("VulkanBridge::CreateSurface: instance must not be null");
        VkSurfaceKHR surface{};
        Require(SDL_Vulkan_CreateSurface(window.impl_->native, instance, allocator, &surface), "SDL_Vulkan_CreateSurface");
        return surface;
    }
    void Platform::VulkanBridge::DestroySurface(Window& window, VkInstance instance, VkSurfaceKHR surface,
                                               const VkAllocationCallbacks* allocator) {
        window.impl_->RequireMode(WindowMode::Vulkan, "VulkanBridge::DestroySurface");
        if (!surface) return;
        if (!instance) throw std::invalid_argument("VulkanBridge::DestroySurface: instance must not be null");
        SDL_Vulkan_DestroySurface(instance, surface, allocator);
    }
}
