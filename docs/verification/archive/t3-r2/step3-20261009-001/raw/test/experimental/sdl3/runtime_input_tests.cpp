#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <Windows.h>
#include "input_candidate.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {
    using SymoCraft::Key;
    using SymoCraft::Experimental::Sdl3::ControlScancodes;
    using SymoCraft::Experimental::Sdl3::InputCandidate;
    using SymoCraft::Experimental::Sdl3::PhysicalInputState;

    int checks{};
    bool input_fixture_available{true};
    SDL_WindowID fixture_window_id{};
    std::array<unsigned, 2> button_down_events{}, button_up_events{};

    void Check(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
        ++checks;
    }

    void RequireSdl(bool condition, const char* operation) {
        if (!condition) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }

    class VideoOwner {
    public:
        ~VideoOwner() { Release(); }
        void Acquire() {
            if (acquired_) return;
            RequireSdl(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL_InitSubSystem(SDL_INIT_VIDEO)");
            acquired_ = true;
        }
        void Release() noexcept {
            if (!acquired_) return;
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            acquired_ = false;
        }
        VideoOwner() = default;
        VideoOwner(const VideoOwner&) = delete;
        VideoOwner& operator=(const VideoOwner&) = delete;
    private:
        bool acquired_{};
    };

    class NativeWindow {
    public:
        NativeWindow() {
            window_ = SDL_CreateWindow("SDL3 synthetic runtime input fixture", 640, 480,
                SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
            RequireSdl(window_ != nullptr, "SDL_CreateWindow(native)");
        }
        ~NativeWindow() { Destroy(); }
        void Destroy() noexcept {
            if (!window_) return;
            SDL_DestroyWindow(window_);
            window_ = nullptr;
        }
        SDL_Window* Get() const { return window_; }
        HWND Handle() const {
            const auto properties = SDL_GetWindowProperties(window_);
            RequireSdl(properties != 0, "SDL_GetWindowProperties");
            const auto hwnd = static_cast<HWND>(SDL_GetPointerProperty(
                properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
            Check(hwnd && IsWindow(hwnd), "The fixture has no valid owned HWND");
            DWORD process{};
            GetWindowThreadProcessId(hwnd, &process);
            Check(process == GetCurrentProcessId(), "The fixture HWND belongs to another process");
            return hwnd;
        }
        NativeWindow(const NativeWindow&) = delete;
        NativeWindow& operator=(const NativeWindow&) = delete;
    private:
        SDL_Window* window_{};
    };

    void Drain(InputCandidate& input) {
        SDL_Event event{};
        int drained{};
        while (SDL_PollEvent(&event)) {
            if (++drained > 8192) throw std::runtime_error("The fixture event queue exceeded its drain budget");
            if ((event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) &&
                event.button.windowID == fixture_window_id &&
                (event.button.button == SDL_BUTTON_LEFT || event.button.button == SDL_BUTTON_RIGHT)) {
                const std::size_t index = event.button.button == SDL_BUTTON_LEFT ? 0 : 1;
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) ++button_down_events[index];
                else ++button_up_events[index];
            }
            input.Handle(event);
        }
    }

    PhysicalInputState PhysicalState(SDL_Window* window) {
        PhysicalInputState physical;
        physical.window_id = SDL_GetWindowID(window);
        const auto flags = SDL_GetWindowFlags(window);
        physical.focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
        physical.minimized = (flags & SDL_WINDOW_MINIMIZED) != 0;
        int count{};
        const bool* keyboard = SDL_GetKeyboardState(&count);
        RequireSdl(keyboard != nullptr, "SDL_GetKeyboardState");
        for (std::size_t index = 0; index < ControlScancodes.size(); ++index) {
            const auto code = static_cast<int>(ControlScancodes[index]);
            if (code < 0 || code >= count)
                throw std::runtime_error("A project scancode is outside the SDL keyboard state");
            physical.keys[index] = keyboard[code];
        }
        const auto buttons = SDL_GetMouseState(nullptr, nullptr);
        physical.left_button = (buttons & SDL_BUTTON_LMASK) != 0;
        physical.right_button = (buttons & SDL_BUTTON_RMASK) != 0;
        return physical;
    }

    void Capture(InputCandidate& input, SDL_Window* window) {
        input.SynchronizePhysicalState(PhysicalState(window));
        input.CaptureKeys();
    }

    void Sample(InputCandidate& input, SDL_Window* window) {
        Drain(input);
        input.PublishEvents();
        Capture(input, window);
    }

    template<typename Predicate>
    bool PumpUntil(InputCandidate& input, Predicate&& predicate, Uint64 milliseconds = 1000) {
        const auto deadline = SDL_GetTicksNS() + milliseconds * 1000000ull;
        do {
            Drain(input);
            if (predicate()) return true;
            SDL_Delay(2);
        } while (SDL_GetTicksNS() < deadline);
        return false;
    }

    void PostKey(HWND hwnd, UINT scan, bool down, bool repeat = false) {
        const UINT key = MapVirtualKeyExW(scan, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));
        Check(key != 0, "A fixture scan code has no virtual key in the current layout");
        LPARAM flags = 1 | (static_cast<LPARAM>(scan) << 16);
        if (repeat || !down) flags |= static_cast<LPARAM>(1ull << 30);
        if (!down) flags |= static_cast<LPARAM>(1ull << 31);
        Check(PostMessageW(hwnd, down ? WM_KEYDOWN : WM_KEYUP, key, flags) != FALSE,
            "PostMessageW keyboard input failed");
    }

    void PostButton(HWND hwnd, UINT message, WPARAM state) {
        Check(PostMessageW(hwnd, message, state, MAKELPARAM(100, 100)) != FALSE,
            "PostMessageW mouse button input failed");
    }

    void KeyboardChecks(InputCandidate& input, SDL_Window* window, HWND hwnd) {
        constexpr UINT w_scan = 0x11;
        input.ResetInput();
        PostKey(hwnd, w_scan, true);
        PostKey(hwnd, w_scan, false);
        Sample(input, window);
        Check(input.Input().Down(Key::W), "A same-drain W down/up was not latched");
        Capture(input, window);
        Check(!input.Input().Down(Key::W), "The W short press was consumed twice");

        PostKey(hwnd, w_scan, true);
        Sample(input, window);
        Check(input.Input().Down(Key::W), "The held W state was not captured");
        Check(PhysicalState(window).keys[static_cast<std::size_t>(Key::W)],
            "SDL keyboard state did not retain the synthetic held W");
        input.ResetInput();
        Capture(input, window);
        Check(input.Input().Down(Key::W), "Reset lost the still-held SDL keyboard state");
        PostKey(hwnd, w_scan, true, true);
        PostKey(hwnd, w_scan, false);
        Sample(input, window);
        Capture(input, window);
        Check(!input.Input().Down(Key::W), "A repeated and released W remained down");

        constexpr std::array<UINT, ControlScancodes.size()> native_scans{
            0x01, 0x2a, 0x3a, 0x1d, 0x11, 0x1f, 0x20, 0x1e, 0x39, 0x12, 0x10};
        for (std::size_t index = 0; index < native_scans.size(); ++index) {
            input.ResetInput();
            PostKey(hwnd, native_scans[index], true);
            PostKey(hwnd, native_scans[index], false);
            Sample(input, window);
            Check(input.Input().keys[index], "A project scancode short press was not mapped");
            Capture(input, window);
            Check(!input.Input().keys[index], "A project scancode short press was consumed twice");
        }
        std::cout << "keyboard_scope=owned HWND PostMessage; current layout only; no hardware or alternate-layout validation\n";
    }

    void ButtonChecks(InputCandidate& input, SDL_Window* window, HWND hwnd) {
        for (std::size_t index = 0; index < 2; ++index) {
            const bool left = index == 0;
            const char* name = left ? "left" : "right";
            const auto down_before = button_down_events[index], up_before = button_up_events[index];
            input.ResetInput();
            PostButton(hwnd, left ? WM_LBUTTONDOWN : WM_RBUTTONDOWN, left ? MK_LBUTTON : MK_RBUTTON);
            Sample(input, window);
            const auto physical = PhysicalState(window);
            const bool physically_held = left ? physical.left_button : physical.right_button;
            const bool candidate_held = left ? input.Input().left_button : input.Input().right_button;
            const bool reliable = physically_held && physical.focused && !physical.minimized && input.Focused();
            if (reliable) {
                Check(candidate_held, "A retained SDL button state was not captured");
                input.ResetInput();
                Capture(input, window);
                Check(left ? input.Input().left_button : input.Input().right_button,
                    "Reset lost the still-held SDL button state");
            } else {
                input_fixture_available = false;
                std::cout << "button_fixture=" << name << ":unavailable; sdl_held_after_drain="
                          << (physically_held ? "true" : "false") << "; focused="
                          << (physical.focused ? "true" : "false") << "; minimized="
                          << (physical.minimized ? "true" : "false")
                          << "; held/reset assertions not executed; native messages do not set Windows physical button state\n";
            }
            PostButton(hwnd, left ? WM_LBUTTONUP : WM_RBUTTONUP, 0);
            Sample(input, window);
            if (reliable)
                Check(!(left ? input.Input().left_button : input.Input().right_button),
                    "A released fixture button remained held");
            std::cout << "button_observation=" << name << "; matching_sdl_down_events="
                      << button_down_events[index] - down_before << "; matching_sdl_up_events="
                      << button_up_events[index] - up_before << '\n';
        }
        std::cout << "mouse_scope=owned HWND message observations only; unavailable held fixtures remain partial; no hardware buttons/motion/acceleration validation\n";
    }

    void FocusChecks(InputCandidate& input, SDL_Window* window, HWND hwnd) {
        input.ResetInput();
        PostKey(hwnd, 0x11, true);
        Sample(input, window);
        Check(input.Input().Down(Key::W), "The focus-loss fixture did not begin with held W");
        ShowWindow(hwnd, SW_MINIMIZE);
        Check(PumpUntil(input, [&] { return input.Minimized() && !input.Focused(); }),
            "Real minimize did not produce SDL minimize and focus-loss facts");
        input.PublishEvents();
        Capture(input, window);
        Check(input.Input().focus_lost, "The real focus-loss pulse was not published");
        Check(!input.Input().Down(Key::W), "Minimized input retained the old held W");
        Sample(input, window);
        Check(!input.Input().focus_lost, "The focus-loss pulse was published twice");
        ShowWindow(hwnd, SW_RESTORE);
        Check(PumpUntil(input, [&] { return !input.Minimized(); }),
            "Real restore did not produce the SDL restored fact");
        input.PublishEvents();
        Capture(input, window);
        Check(!input.Input().Down(Key::W), "Restore resurrected W after SDL focus-loss release");
        PostKey(hwnd, 0x11, false);
        Sample(input, window);
        std::cout << "focus_scope=real ShowWindow minimize/restore of the owned HWND; no manipulation of other windows\n";
    }

    void WaitChecks(InputCandidate& input, SDL_Window* window, HWND hwnd) {
        Drain(input);
        SDL_SetError("stale fixture error");
        SDL_ClearError();
        Check(*SDL_GetError() == '\0', "SDL_ClearError did not remove the stale fixture error");
        SDL_Event timeout_event{};
        const bool received = SDL_WaitEventTimeout(&timeout_event, 1);
        if (received) input.Handle(timeout_event);
        Check(*SDL_GetError() == '\0', "A bounded event wait reused the stale fixture error");
        std::cout << "wait_timeout_observed=" << (received ? "false; another real event arrived" : "true") << '\n';
        Drain(input);
        Check(PostMessageW(hwnd, WM_CLOSE, 0, 0) != FALSE, "PostMessageW(WM_CLOSE) failed");
        SDL_PumpEvents();
        Uint32 first_type{};
        SDL_WindowID first_window{};
        const bool waited = input.WaitAndDrain([&](SDL_Event& event) {
            SDL_ClearError();
            const bool got = SDL_WaitEventTimeout(&event, 1000);
            if (!got && *SDL_GetError())
                throw std::runtime_error(std::string("SDL_WaitEventTimeout(close): ") + SDL_GetError());
            if (got) {
                first_type = event.type;
                if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) first_window = event.window.windowID;
            }
            return got;
        }, [](SDL_Event& event) { return SDL_PollEvent(&event); });
        Check(waited, "The close fixture wait did not receive an event");
        Check(first_type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && first_window == SDL_GetWindowID(window),
            "WM_CLOSE was not the matching first event returned by wait");
        Check(input.ShouldClose(), "The reducer lost the first close event returned by wait");
        Sample(input, window);
        Check(input.ShouldClose(), "The next publish cleared the close request");
    }

    void Run() {
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0,
            "The standalone fixture unexpectedly inherited a video owner");
        RequireSdl(SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0"),
            "SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD)");
        VideoOwner first;
        first.Acquire();
        first.Acquire();
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0, "The first video owner failed to initialize");
        VideoOwner second;
        second.Acquire();
        NativeWindow window;
        const auto hwnd = window.Handle();
        fixture_window_id = SDL_GetWindowID(window.Get());
        Check(!(SDL_GetWindowFlags(window.Get()) & (SDL_WINDOW_OPENGL | SDL_WINDOW_VULKAN)),
            "The native fixture unexpectedly owns an OpenGL or Vulkan window");
        first.Release();
        first.Release();
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
            "Releasing the first owner destroyed the second owner's video subsystem");
        Check(IsWindow(hwnd) != FALSE, "Releasing the first owner destroyed the second owner's window");
        RequireSdl(SDL_ShowWindow(window.Get()), "SDL_ShowWindow");
        InputCandidate input(SDL_GetWindowID(window.Get()), false,
            (SDL_GetWindowFlags(window.Get()) & SDL_WINDOW_INPUT_FOCUS) != 0);
        const bool foreground_request = SetForegroundWindow(hwnd) != FALSE;
        const bool focused = PumpUntil(input, [&] {
            return GetForegroundWindow() == hwnd && input.Focused() && SDL_GetKeyboardFocus() == window.Get();
        });
        std::cout << "owned_foreground_request_returned=" << (foreground_request ? "true" : "false")
                  << "; owned_foreground_observed=" << (GetForegroundWindow() == hwnd ? "true" : "false") << '\n';
        input.PublishEvents();
        Capture(input, window.Get());
        if (focused) {
            KeyboardChecks(input, window.Get(), hwnd);
            ButtonChecks(input, window.Get(), hwnd);
            FocusChecks(input, window.Get(), hwnd);
        } else {
            input_fixture_available = false;
            std::cout << "input_fixture=unavailable; Windows did not grant foreground focus; keyboard/buttons/focus checks not executed\n";
        }
        WaitChecks(input, window.Get(), hwnd);
        window.Destroy();
        window.Destroy();
        Check(IsWindow(hwnd) == FALSE, "The owned HWND survived repeated window cleanup");
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
            "Window cleanup unexpectedly released the second owner's video subsystem");
        second.Release();
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0,
            "Video references remained after both owners released; duplicate Acquire may have incremented them");
        second.Release();
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0, "Repeated subsystem cleanup changed video state");
    }
}

int main() {
    SDL_SetMainReady();
    int exit_code{};
    try {
        Run();
        exit_code = input_fixture_available ? 0 : 2;
        std::cout << "SDL3 real runtime input/lifecycle fixture: " << checks << " checks passed.\n"
                  << "status=" << (input_fixture_available ? "pass" : "partial; foreground or synthetic held-button fixture unavailable") << '\n'
                  << "scope=independent SDL runtime and candidate reducer; not production platform or human acceptance\n";
    } catch (const std::exception& error) {
        std::cerr << "SDL3 runtime input/lifecycle fixture failed after " << checks << " checks: " << error.what() << '\n';
        exit_code = 1;
    }
    // This standalone process owns final SDL globals; module owners only release video references.
    SDL_Quit();
    return exit_code;
}
