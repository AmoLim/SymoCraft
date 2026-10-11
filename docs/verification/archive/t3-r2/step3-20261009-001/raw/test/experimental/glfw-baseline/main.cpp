#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <symocraft/platform/window.h>
#include <graphics_bridge.h>
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {
    int checks{};
    void Require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
        ++checks;
    }
    HWND FindWindowForThisProcess() {
        HWND found{};
        EnumWindows([](HWND window, LPARAM result) -> BOOL {
            DWORD process{};
            GetWindowThreadProcessId(window, &process);
            if (process == GetCurrentProcessId() && GetWindow(window, GW_OWNER) == nullptr) {
                *reinterpret_cast<HWND*>(result) = window;
                return FALSE;
            }
            return TRUE;
        }, reinterpret_cast<LPARAM>(&found));
        Require(found && IsWindow(found), "No valid HWND for the real platform window");
        return found;
    }
    void PostKey(HWND window, UINT message, WPARAM key, bool repeat = false) {
        const auto scan = MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC);
        LPARAM flags = 1 | (static_cast<LPARAM>(scan) << 16);
        if (repeat || message == WM_KEYUP) flags |= static_cast<LPARAM>(1ull << 30);
        if (message == WM_KEYUP) flags |= static_cast<LPARAM>(1ull << 31);
        Require(PostMessageW(window, message, key, flags) != FALSE, "PostMessageW keyboard failed");
    }
    void* GlProcedure(const char* name) {
        return reinterpret_cast<void*>(SymoCraft::Platform::GraphicsBridge::GetProcedure(name));
    }
    struct Runtime {
        Runtime() { SymoCraft::Window::Init(); }
        ~Runtime() { SymoCraft::Window::Free(); }
    };
}

int main() {
    try {
        Runtime runtime;
        std::unique_ptr<SymoCraft::Window> window(SymoCraft::Window::Create("GLFW baseline API probe", 640, 480));
        Require(gladLoadGLLoader(GlProcedure) != 0 && GLAD_GL_VERSION_4_6, "GLAD/GL 4.6 failed");
        GLint samples{}, profile{};
        glGetIntegerv(GL_SAMPLES, &samples);
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profile);
        Require(samples == 4 && (profile & GL_CONTEXT_CORE_PROFILE_BIT), "Actual GL Core/4x MSAA mismatch");
        std::cout << "actual_gl=" << glGetString(GL_VERSION) << "; samples=" << samples << '\n';
        const auto hwnd = FindWindowForThisProcess();
        window->PollInt();
        window->ResetInput();
        PostKey(hwnd, WM_KEYDOWN, 'W');
        PostKey(hwnd, WM_KEYUP, 'W');
        window->PollInt(); window->CaptureInput();
        Require(window->Input().Down(SymoCraft::Key::W), "A short press was not latched");
        window->CaptureInput();
        Require(!window->Input().Down(SymoCraft::Key::W), "A short press was consumed twice");
        PostKey(hwnd, WM_KEYDOWN, 'W');
        window->PollInt(); window->CaptureInput();
        Require(window->Input().Down(SymoCraft::Key::W), "Held key not sampled");
        window->ResetInput(); window->CaptureInput();
        Require(window->Input().Down(SymoCraft::Key::W), "GLFW Reset cleared the already-held keyboard state");
        PostKey(hwnd, WM_KEYDOWN, 'W', true);
        PostKey(hwnd, WM_KEYUP, 'W');
        window->PollInt(); window->CaptureInput(); window->CaptureInput();
        Require(!window->Input().Down(SymoCraft::Key::W), "Released key remained down");

        window->ResetInput();
        Require(PostMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(100, 100)), "First motion post failed");
        Require(PostMessageW(hwnd, WM_MOUSEMOVE, 0, MAKELPARAM(104, 97)), "Second motion post failed");
        Require(PostMessageW(hwnd, WM_MOUSEWHEEL, MAKEWPARAM(0, WHEEL_DELTA / 2), 0), "Wheel post failed");
        window->PollInt();
        bool motion{}, wheel{};
        for (const auto& event : window->Input().pointer_events) {
            motion |= event.mouse_dx == 4 && event.mouse_dy == 3;
            wheel |= std::abs(event.scroll_y - 0.5) < 0.0001;
        }
        Require(motion && wheel, "Motion sign or fractional wheel mapping differs");
        ShowWindow(hwnd, SW_MINIMIZE);
        window->PollInt(); window->CaptureInput();
        Require(window->Input().focus_lost && !window->Focused() && window->Minimized(), "Native minimize/focus loss fact not published");
        window->PollInt();
        Require(!window->Input().focus_lost, "Focus loss pulse repeated");
        ShowWindow(hwnd, SW_RESTORE);
        window->PollInt();
        Require(!window->Minimized(), "Native restore fact not published");
        Require(PostMessageW(hwnd, WM_CLOSE, 0, 0), "Close post failed");
        window->WaitEvents(0.1);
        Require(window->ShouldClose(), "WaitEvents lost WM_CLOSE");
        window->Destroy(); window->Destroy(); window.reset();

        window.reset(SymoCraft::Window::Create("GLFW benchmark baseline probe", 1920, 1080, true, true));
        Require(window->width == 1920 && window->height == 1080, "GLFW benchmark pixels differ from 1920x1080");
        const auto benchmark_hwnd = FindWindowForThisProcess();
        Require(GetPropW(benchmark_hwnd, L"NonRudeHWND") != nullptr, "GLFW NonRudeHWND missing");
        const auto style = GetWindowLongPtrW(benchmark_hwnd, GWL_STYLE);
        const auto extended = GetWindowLongPtrW(benchmark_hwnd, GWL_EXSTYLE);
        Require(!(style & WS_CAPTION) && !(extended & WS_EX_TOPMOST), "Benchmark decoration/topmost changed");
        window->Destroy(); window.reset();
        std::cout << "GLFW real platform API baseline: " << checks << " checks passed.\n"
                  << "Keyboard/pointer messages are synthetic; physical layout, acceleration and human play remain unvalidated.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "GLFW baseline probe failed: " << error.what() << '\n';
        return 1;
    }
}
