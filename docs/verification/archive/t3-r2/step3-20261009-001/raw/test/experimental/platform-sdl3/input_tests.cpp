#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <SDL3/SDL.h>
#include <symocraft/platform/window.h>
#include <native_bridge.h>
#include <graphics_bridge.h>
#include <SDL3/SDL_opengl.h>
#include "input_test_events.h"
#include "input_options.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <io.h>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    using SymoCraft::Window;
    using SymoCraft::WindowMode;
    using SymoCraft::Key;
    using SymoCraft::CursorMode;
    using SymoCraft::Platform::NativeBridge;
    using SymoCraft::Platform::GraphicsBridge;
    constexpr std::array<SDL_Scancode, 11> Controls{
        SDL_SCANCODE_ESCAPE, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_SPACE,
        SDL_SCANCODE_E, SDL_SCANCODE_Q};
    unsigned checks{};

    std::string JsonString(const std::string& value) {
        std::ostringstream out;
        out << '"';
        constexpr char digits[] = "0123456789abcdef";
        for (unsigned char byte : value) {
            if (byte == '"' || byte == '\\') out << '\\' << static_cast<char>(byte);
            else if (byte < 0x20) out << "\\u00" << digits[byte >> 4] << digits[byte & 15];
            else out << static_cast<char>(byte);
        }
        out << '"';
        return out.str();
    }
    struct Report {
        std::map<std::string, std::string> fields;
        void String(const std::string& key, const std::string& value) { fields[key] = JsonString(value); }
        void Boolean(const std::string& key, bool value) { fields[key] = value ? "true" : "false"; }
        template<typename Value> void Number(const std::string& key, Value value) {
            std::ostringstream out;
            out << value;
            fields[key] = out.str();
        }
        std::string Json() const {
            std::ostringstream out;
            out << "{\n";
            bool first = true;
            for (const auto& [key, value] : fields) {
                if (!first) out << ",\n";
                out << "  " << JsonString(key) << ": " << value;
                first = false;
            }
            out << "\n}\n";
            return out.str();
        }
    };
    void Check(bool condition, const char* diagnostic) {
        ++checks;
        if (!condition) throw std::runtime_error(diagnostic);
    }
    void Sdl(bool condition, const char* operation) {
        if (!condition) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
    void Push(SDL_Event event) { Sdl(SDL_PushEvent(&event), "SDL_PushEvent"); }
    void Sample(Window& window) { window.PollInt(); window.CaptureInput(); }
    template<typename Predicate> bool PumpUntil(Window& window, Predicate&& predicate, Uint64 timeout = 1500) {
        const Uint64 deadline = SDL_GetTicksNS() + timeout * 1000000ull;
        do {
            window.PollInt();
            if (predicate()) return true;
            SDL_Delay(2);
        } while (SDL_GetTicksNS() < deadline);
        return false;
    }
    SDL_Window* OwnedSdlWindow(HWND hwnd) {
        int count{};
        SDL_Window** windows = SDL_GetWindows(&count);
        Sdl(windows != nullptr, "SDL_GetWindows");
        SDL_Window* result{};
        for (int index = 0; index < count; ++index) {
            const auto properties = SDL_GetWindowProperties(windows[index]);
            if (SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr) == hwnd)
                result = windows[index];
        }
        SDL_free(windows);
        Check(result != nullptr, "The production HWND has no matching SDL window");
        return result;
    }
    bool FocusOwned(Window& window, SDL_Window* native, HWND hwnd, Report& report) {
        report.Boolean("owned_hwnd_visible_before_fixture_show", IsWindowVisible(hwnd) != FALSE);
        // Hidden process startup can override SDL's first ShowWindow call. Show only this owned fixture.
        const bool previously_visible = ShowWindow(hwnd, SW_SHOW) != FALSE;
        report.Boolean("owned_fixture_show_previously_visible", previously_visible);
        report.Boolean("owned_hwnd_visible_after_fixture_show", IsWindowVisible(hwnd) != FALSE);
        const bool request = SetForegroundWindow(hwnd) != FALSE;
        report.Boolean("owned_foreground_request_returned", request);
        const bool focused = PumpUntil(window, [&] {
            return GetForegroundWindow() == hwnd && window.Focused() && SDL_GetKeyboardFocus() == native;
        });
        report.Boolean("owned_foreground_observed", focused);
        if (!focused) report.String("partial_reason", "Windows did not grant foreground focus; focus-dependent assertions were not executed");
        if (focused) {
            Sample(window);
            window.ResetInput();
        }
        return focused;
    }
    void IsolateQueue(Window& window) {
        window.PollInt();
        // This process owns the only platform window; remove pre-test OS events, not another app's events.
        SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
        window.ResetInput();
    }
    void QueueChecks(Window& window, SDL_WindowID id, Report& report) {
        IsolateQueue(window);
        for (std::size_t index = 0; index < Controls.size(); ++index) {
            Push(InputTest::Key(SDL_EVENT_KEY_DOWN, Controls[index], id));
            Push(InputTest::Key(SDL_EVENT_KEY_UP, Controls[index], id));
        }
        Sample(window);
        for (bool key : window.Input().keys) Check(key, "Production Poll/Capture lost a same-drain scancode short press");
        Check(window.Input().Down(Key::W) && window.Input().Down(Key::W), "Snapshot reads consumed W");
        window.CaptureInput();
        for (bool key : window.Input().keys) Check(!key, "Short-press latch was consumed more than once");
        Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, id, true));
        Sample(window);
        Check(!window.Input().Down(Key::W), "An orphan key repeat manufactured a held control");
        const SDL_WindowID foreign = id + 1000;
        Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, foreign));
        Push(InputTest::Window(SDL_EVENT_WINDOW_CLOSE_REQUESTED, foreign));
        Push(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, foreign));
        Push(InputTest::Motion(99, 99, foreign));
        Push(InputTest::Wheel(99, foreign));
        Push(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, foreign));
        Sample(window);
        Check(!window.ShouldClose() && !window.Input().Down(Key::W) && !window.Input().focus_lost &&
              window.Input().pointer_events.empty(), "Production accepted a foreign window event");
        IsolateQueue(window);
        Push(InputTest::Motion(1000, 1000, id));
        Push(InputTest::Motion(2.5F, -3.25F, id));
        Push(InputTest::Wheel(0.5F, id));
        Push(InputTest::Motion(-4, 5, id));
        Push(InputTest::Wheel(2, id, SDL_MOUSEWHEEL_FLIPPED));
        Sample(window);
        const auto& pointer = window.Input().pointer_events;
        Check(pointer.size() == 4, "Production first-motion guard or event count changed");
        Check(pointer[0].mouse_dx == 2.5 && pointer[0].mouse_dy == 3.25, "Production relative motion signs/precision changed");
        Check(pointer[1].scroll_y == 0.5 && pointer[2].mouse_dx == -4 && pointer[2].mouse_dy == -5 &&
              pointer[3].scroll_y == -2, "Production motion/wheel order or flipped wheel normalization changed");
        window.CaptureInput();
        Check(window.Input().pointer_events.size() == 4, "Capture unexpectedly consumed published pointer events");
        Sample(window);
        Check(window.Input().pointer_events.empty(), "Published pointer sequence replayed next poll");
        Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, id));
        Push(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, id));
        Push(InputTest::Wheel(4, id));
        window.PollInt();
        window.ResetInput();
        window.CaptureInput();
        Check(!window.Input().Down(Key::Space) && window.Input().pointer_events.empty(), "Reset kept a queued press or wheel");
        Push(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, id));
        Push(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT, id));
        Sample(window);
        const auto buttons = SDL_GetMouseState(nullptr, nullptr);
        Check(window.Input().left_button == ((buttons & SDL_BUTTON_LMASK) != 0) &&
              window.Input().right_button == ((buttons & SDL_BUTTON_RMASK) != 0),
              "Capture adopted synthetic held buttons instead of resampling SDL state");
        report.Boolean("synthetic_buttons_resampled_not_hardware_proof", true);
        SDL_Event quit{};
        quit.type = SDL_EVENT_QUIT;
        Push(quit);
        window.PollInt();
        Check(window.ShouldClose(), "Production ignored global SDL quit");
        report.Boolean("global_quit_received", true);
    }
    void WaitChecks(Window& window, SDL_WindowID id, Report& report) {
        IsolateQueue(window);
        SDL_SetError("stale pre-timeout fixture error");
        const double timeout_start = Window::Time();
        window.WaitEvents(0.001);
        report.Number("bounded_idle_wait_seconds", Window::Time() - timeout_start);
        window.PollInt();
        window.CaptureInput();
        Check(!window.ShouldClose() && !window.Input().Down(Key::Space), "Bounded idle wait manufactured close or input from a stale SDL error");
        report.Boolean("bounded_idle_wait_no_false_input", true);
        // An OS event may legitimately wake this bounded wait; it is not proof of a forced timeout.
        report.Boolean("bounded_idle_wait_forced_timeout_proof", false);
        IsolateQueue(window);
        Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, id));
        Push(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, id));
        Push(InputTest::Wheel(0.5F, id));
        SDL_Event first{};
        Check(SDL_PeepEvents(&first, 1, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) == 1 &&
              first.type == SDL_EVENT_KEY_DOWN && first.key.windowID == id,
              "The pre-wait queue did not begin with the matching key-down fixture");
        SDL_SetError("stale pre-wait fixture error");
        window.WaitEvents(0.1);
        window.PollInt();
        window.CaptureInput();
        Check(window.Input().Down(Key::Space), "Wait lost its first key-down event or drained key-up erased the latch");
        Check(window.Input().pointer_events.size() == 1 && window.Input().pointer_events[0].scroll_y == 0.5,
              "Wait drain lost its ordered wheel event");
        window.CaptureInput();
        Check(!window.Input().Down(Key::Space), "Wait short-press latch was consumed twice");
        window.PollInt();
        Check(window.Input().pointer_events.empty(), "Wait pointer event replayed after publication");
        report.Boolean("wait_first_key_event_consumed_once", true);
    }
    void PostKey(HWND hwnd, UINT scan, bool down, bool repeat = false) {
        const UINT key = MapVirtualKeyExW(scan, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0));
        Check(key != 0, "A project scan code has no native virtual key in the current keyboard layout");
        LPARAM flags = 1 | (static_cast<LPARAM>(scan) << 16);
        if (repeat || !down) flags |= static_cast<LPARAM>(1ull << 30);
        if (!down) flags |= static_cast<LPARAM>(1ull << 31);
        Check(PostMessageW(hwnd, down ? WM_KEYDOWN : WM_KEYUP, key, flags) != FALSE,
              "PostMessageW owned keyboard fixture failed");
    }
    void NativeKeys(Window& window, SDL_Window* native, HWND hwnd, Report& report) {
        constexpr std::array<UINT, 11> scans{0x01, 0x2a, 0x3a, 0x1d, 0x11, 0x1f, 0x20, 0x1e, 0x39, 0x12, 0x10};
        for (std::size_t index = 0; index < scans.size(); ++index) {
            window.ResetInput();
            PostKey(hwnd, scans[index], true);
            PostKey(hwnd, scans[index], false);
            Sample(window);
            Check(window.Input().keys[index], "Owned Win32 same-drain short press did not reach the production mapping");
            window.CaptureInput();
            Check(!window.Input().keys[index], "Owned Win32 short press was consumed twice");
        }
        PostKey(hwnd, 0x11, true);
        Sample(window);
        int count{};
        const bool* keys = SDL_GetKeyboardState(&count);
        Sdl(keys != nullptr, "SDL_GetKeyboardState");
        const bool retained = count > SDL_SCANCODE_W && keys[SDL_SCANCODE_W] &&
                              SDL_GetKeyboardFocus() == native && window.Focused();
        report.Boolean("owned_message_held_w_retained_by_sdl", retained);
        if (retained) {
            Check(window.Input().Down(Key::W), "Production lost SDL's retained W state");
            window.ResetInput();
            window.CaptureInput();
            Check(window.Input().Down(Key::W), "Reset/Capture lost the still-held SDL W state");
        } else {
            report.String("held_key_fixture", "unavailable; PostMessage did not establish retained SDL keyboard state; held/reset assertions not executed");
        }
        PostKey(hwnd, 0x11, true, true);
        PostKey(hwnd, 0x11, false);
        Sample(window);
        window.CaptureInput();
        Check(!window.Input().Down(Key::W), "Owned Win32 repeat/release left W stuck");
        report.Boolean("all_11_owned_message_mappings_verified", true);
        if (!retained) throw std::runtime_error("PARTIAL: owned-message held W fixture unavailable");
    }
    void FocusChecks(Window& window, HWND hwnd, Report& report) {
        PostKey(hwnd, 0x11, true);
        Sample(window);
        Check(window.Input().Down(Key::W), "Real minimize fixture did not start with W captured");
        ShowWindow(hwnd, SW_MINIMIZE);
        Check(PumpUntil(window, [&] {
            return window.Minimized() && !window.Focused() && window.Input().focus_lost;
        }), "Owned real minimize did not publish a focus-loss pulse");
        window.CaptureInput();
        for (bool key : window.Input().keys) Check(!key, "Minimize retained a held project key");
        Check(!window.Input().left_button && !window.Input().right_button && window.Input().pointer_events.empty(),
              "Minimize retained buttons or pointer events");
        Sample(window);
        Check(!window.Input().focus_lost, "Minimize focus-loss pulse repeated next poll");
        ShowWindow(hwnd, SW_RESTORE);
        Check(PumpUntil(window, [&] { return !window.Minimized(); }), "Owned real restore did not update window state");
        window.CaptureInput();
        Check(!window.Input().Down(Key::W), "Restore resurrected the old W before fresh input");
        PostKey(hwnd, 0x11, false);
        Sample(window);
        report.Boolean("owned_real_minimize_restore_verified", true);
    }
    void CursorResize(Window& window, SDL_Window* native, SDL_WindowID id, Report& report) {
        const char* scale = SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE);
        Check(scale && std::string(scale) == "1", "Production relative system-scale policy differs from the agreed candidate");
        for (CursorMode mode : {CursorMode::Normal, CursorMode::Hidden, CursorMode::Lock, CursorMode::Normal}) {
            IsolateQueue(window);
            Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, id));
            Push(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, id));
            Push(InputTest::Wheel(2, id));
            window.PollInt();
            window.SetCursorMode(mode);
            Check(SDL_GetWindowRelativeMouseMode(native) == (mode == CursorMode::Lock), "Production cursor mode relative-state mismatch");
            Check(SDL_CursorVisible() == (mode == CursorMode::Normal), "Production cursor mode visibility mismatch");
            Check(!window.Input().Down(Key::Space) && window.Input().pointer_events.empty(), "Cursor mode change did not reset stale input");
        }
        window.SetSize(713, 527);
        int pixels_w{}, pixels_h{};
        Check(PumpUntil(window, [&] {
            Sdl(SDL_GetWindowSizeInPixels(native, &pixels_w, &pixels_h), "SDL_GetWindowSizeInPixels");
            return window.width == pixels_w && window.height == pixels_h && pixels_w > 0 && pixels_h > 0;
        }), "Production drawable dimensions did not match actual SDL pixels after resize");
        report.Number("resize_drawable_width", pixels_w);
        report.Number("resize_drawable_height", pixels_h);
        Check(window.GetAspectRatio() == static_cast<float>(pixels_w) / pixels_h, "Production aspect ratio is not drawable-pixel based");
        window.SetTitle("SymoCraft production SDL3 input regression - resized");
        Check(std::string(SDL_GetWindowTitle(native)) == "SymoCraft production SDL3 input regression - resized", "Production title update was not applied");
        report.Boolean("cursor_modes_and_real_pixel_resize_verified", true);
    }
    void BenchmarkChecks(Window& window, SDL_WindowID id, Report& report) {
        IsolateQueue(window);
        Push(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, id));
        for (SDL_Scancode code : Controls) {
            Push(InputTest::Key(SDL_EVENT_KEY_DOWN, code, id));
            Push(InputTest::Key(SDL_EVENT_KEY_UP, code, id));
        }
        Push(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, id));
        Push(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT, id));
        Push(InputTest::Motion(3, 4, id));
        Push(InputTest::Wheel(0.25F, id));
        Sample(window);
        Check(window.Input().Down(Key::Escape), "Allow-unfocused benchmark lost a matching Escape event latch");
        for (std::size_t index = 1; index < Controls.size(); ++index)
            Check(!window.Input().keys[index], "Benchmark accepted a gameplay key");
        Check(!window.Input().left_button && !window.Input().right_button && window.Input().pointer_events.empty(),
              "Benchmark accepted buttons or pointer events");
        window.CaptureInput();
        Check(!window.Input().Down(Key::Escape), "Benchmark Escape latch was consumed more than once");
        report.Boolean("benchmark_allow_unfocused_queue_isolation_verified", true);
        report.Boolean("benchmark_queue_focus_event_is_not_physical_focus_proof", true);
        for (bool use_wait : {false, true}) {
            for (bool loss_first : {false, true}) {
                IsolateQueue(window);
                if (loss_first) Push(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, id));
                Push(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, id));
                Push(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE, id));
                if (!loss_first) Push(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, id));
                if (use_wait) window.WaitEvents(0.1);
                Sample(window);
                Check(window.Input().Down(Key::Escape), "Poll/Wait focus-loss order erased a queued benchmark cancellation");
                window.CaptureInput();
                Check(!window.Input().Down(Key::Escape), "Poll/Wait focus-loss cancellation latch was consumed twice");
            }
        }
        report.Boolean("benchmark_cancel_focus_loss_orders_poll_and_wait_verified", true);
    }
    void WaitClose(Window& window, SDL_WindowID id, HWND hwnd, Report& report) {
        IsolateQueue(window);
        Check(PostMessageW(hwnd, WM_CLOSE, 0, 0) != FALSE, "PostMessageW owned WM_CLOSE failed");
        SDL_PumpEvents();
        SDL_Event first{};
        Check(SDL_PeepEvents(&first, 1, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) == 1 &&
              first.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && first.window.windowID == id,
              "Owned WM_CLOSE was not the first matching event queued before wait");
        window.WaitEvents(1.0);
        Check(window.ShouldClose(), "Wait lost its first owned window-close event");
        window.PollInt();
        Check(window.ShouldClose(), "Poll cleared the waited window-close request");
        report.Boolean("wait_first_owned_close_event_preserved", true);
    }
    struct ManualCommands {
        SDL_WindowID id{};
        std::atomic<unsigned> pending{};
        static bool SDLCALL Watch(void* context, SDL_Event* event) noexcept {
            auto& commands = *static_cast<ManualCommands*>(context);
            if (event->type != SDL_EVENT_KEY_DOWN || event->key.windowID != commands.id || event->key.repeat) return true;
            unsigned command{};
            if (event->key.scancode == SDL_SCANCODE_F5) command = 1;
            else if (event->key.scancode == SDL_SCANCODE_F6) command = 2;
            else if (event->key.scancode == SDL_SCANCODE_F7) command = 4;
            commands.pending.fetch_or(command, std::memory_order_relaxed);
            return true;
        }
        explicit ManualCommands(SDL_WindowID window_id) : id(window_id) {
            Sdl(SDL_AddEventWatch(Watch, this), "SDL_AddEventWatch(manual commands)");
        }
        ~ManualCommands() { SDL_RemoveEventWatch(Watch, this); }
    };
    const char* CursorName(CursorMode mode) {
        return mode == CursorMode::Lock ? "Lock" : mode == CursorMode::Hidden ? "Hidden" : "Normal";
    }
    void ManualInput(Window& window, SDL_Window* native, SDL_WindowID id, HWND hwnd,
                     const std::filesystem::path& output, Report& report) {
        const auto snapshots = output / L"manual-snapshots.jsonl";
        const auto console_path = output / L"manual-console.stderr.log";
        if (!std::filesystem::is_directory(output) || std::filesystem::exists(snapshots) || std::filesystem::exists(console_path))
            throw std::invalid_argument("Manual observations require an existing fresh output directory");
        std::ofstream log(snapshots);
        log.exceptions(std::ios::badbit | std::ios::failbit);
        std::ofstream console_log(console_path);
        console_log.exceptions(std::ios::badbit | std::ios::failbit);
        auto announce = [&](const std::string& text) {
            std::cerr << text;
            console_log << text;
            console_log.flush();
        };
        constexpr std::array<const char*, 11> names{"Escape","LeftShift","CapsLock","LeftControl","W","S","D","A","Space","E","Q"};
        announce("\nProduction SDL3 hardware observation tool (not automatic acceptance)\n"
                 "Use the visible color window; console changes and ordered snapshots are recorded.\n"
                 "Test physical W/A/S/D, Space, Shift, Ctrl, CapsLock, E/Q, both mouse buttons and wheel.\n"
                 "F5: ResetInput, then fresh CaptureInput. Hold a key/button while pressing F5.\n"
                 "F6: cycle Normal -> Hidden -> Lock -> Normal; each change also resets input.\n"
                 "F7: immediate second CaptureInput; short presses may clear, still-held controls should remain.\n"
                 "Alt+Tab/minimize/restore and release keys/buttons while away; inspect no stuck state on return.\n"
                 "Change keyboard layout and test the same physical key positions; layout IDs are logged.\n"
                 "Test Escape LAST: its production snapshot is logged before the tool closes. Window close also exits.\n"
                 "This tool does not establish game resume dt, gameplay, performance or human pass/fail.\n"
                 "Safety limit: 10 minutes; no injected SDL/Win32/global input is used in this case.\n\n");
        ManualCommands commands(id);
        const auto clear_color = reinterpret_cast<decltype(&glClearColor)>(GraphicsBridge::GetProcedure("glClearColor"));
        const auto clear = reinterpret_cast<decltype(&glClear)>(GraphicsBridge::GetProcedure("glClear"));
        const auto viewport = reinterpret_cast<decltype(&glViewport)>(GraphicsBridge::GetProcedure("glViewport"));
        Check(clear_color && clear && viewport, "Manual OpenGL observation procedures are missing");
        CursorMode cursor = CursorMode::Normal;
        window.SetCursorMode(cursor);
        std::array<bool, 11> seen{};
        bool left_seen{}, right_seen{}, previous_focus{}, previous_minimized{}, previous_left{}, previous_right{};
        std::array<bool, 11> previous_keys{};
        unsigned previous_dpi{};
        std::uintptr_t previous_layout{};
        int previous_width{}, previous_height{};
        unsigned observations{}, reset_count{}, mode_count{}, second_capture_count{}, focus_losses{}, presented_frames{};
        Uint64 pointer_count{}, motion_count{}, wheel_count{};
        double motion_dx{}, motion_dy{}, wheel_y{};
        const double start = Window::Time();
        auto record = [&](const char* label, bool console) {
            const auto& input = window.Input();
            const bool focused = window.Focused(), minimized = window.Minimized();
            int logical_width{}, logical_height{};
            Sdl(SDL_GetWindowSize(native, &logical_width, &logical_height), "SDL_GetWindowSize(manual observation)");
            const auto dpi = GetDpiForWindow(hwnd);
            const auto layout = reinterpret_cast<std::uintptr_t>(GetKeyboardLayout(0));
            std::string key_text;
            for (std::size_t index = 0; index < input.keys.size(); ++index) {
                seen[index] = seen[index] || input.keys[index];
                if (input.keys[index]) { if (!key_text.empty()) key_text += ','; key_text += names[index]; }
            }
            left_seen = left_seen || input.left_button;
            right_seen = right_seen || input.right_button;
            std::ostringstream row;
            row << "{\"seconds\":" << Window::Time() - start << ",\"observation\":" << JsonString(label)
                << ",\"cursor_mode\":" << JsonString(CursorName(cursor))
                << ",\"sdl_relative_mouse_mode\":" << (SDL_GetWindowRelativeMouseMode(native) ? "true" : "false")
                << ",\"sdl_cursor_visible\":" << (SDL_CursorVisible() ? "true" : "false")
                << ",\"focused\":" << (focused ? "true" : "false") << ",\"minimized\":" << (minimized ? "true" : "false")
                << ",\"focus_lost\":" << (input.focus_lost ? "true" : "false")
                << ",\"left_button\":" << (input.left_button ? "true" : "false")
                << ",\"right_button\":" << (input.right_button ? "true" : "false")
                << ",\"keys_down\":" << JsonString(key_text) << ",\"drawable_width\":" << window.width
                << ",\"drawable_height\":" << window.height
                << ",\"logical_window_width\":" << logical_width << ",\"logical_window_height\":" << logical_height
                << ",\"window_dpi\":" << dpi
                << ",\"keyboard_layout_id\":" << layout << ",\"pointer_events\":[";
            bool first = true;
            for (const auto& pointer : input.pointer_events) {
                if (!first) row << ',';
                row << "{\"mouse_dx\":" << pointer.mouse_dx << ",\"mouse_dy\":" << pointer.mouse_dy
                    << ",\"scroll_y\":" << pointer.scroll_y << '}';
                first = false;
            }
            row << "]}\n";
            log << row.str();
            log.flush();
            ++observations;
            if (console) {
                std::ostringstream message;
                message << label << " | mode=" << CursorName(cursor) << " focus=" << focused
                    << " minimized=" << minimized << " keys=" << (key_text.empty() ? "none" : key_text)
                    << " buttons=" << input.left_button << ',' << input.right_button << " pixels=" << window.width << 'x'
                    << window.height << " dpi=" << dpi << " layout=" << layout << '\n';
                announce(message.str());
            }
        };
        bool escaped{}, timed_out{};
        while (!window.ShouldClose()) {
            window.PollInt();
            window.CaptureInput();
            const auto& input = window.Input();
            const bool state_changed = input.keys != previous_keys || input.left_button != previous_left ||
                input.right_button != previous_right || window.Focused() != previous_focus ||
                window.Minimized() != previous_minimized || window.width != previous_width || window.height != previous_height ||
                GetDpiForWindow(hwnd) != previous_dpi || reinterpret_cast<std::uintptr_t>(GetKeyboardLayout(0)) != previous_layout;
            escaped = input.Down(Key::Escape);
            if (input.focus_lost) ++focus_losses;
            for (const auto& pointer : input.pointer_events) {
                ++pointer_count;
                if (pointer.mouse_dx != 0 || pointer.mouse_dy != 0) ++motion_count;
                if (pointer.scroll_y != 0) ++wheel_count;
                motion_dx += pointer.mouse_dx;
                motion_dy += pointer.mouse_dy;
                wheel_y += pointer.scroll_y;
            }
            if (state_changed || input.focus_lost || !input.pointer_events.empty()) record("frame", state_changed || input.focus_lost);
            previous_keys = input.keys;
            previous_left = input.left_button;
            previous_right = input.right_button;
            previous_focus = window.Focused();
            previous_minimized = window.Minimized();
            previous_width = window.width;
            previous_height = window.height;
            previous_dpi = GetDpiForWindow(hwnd);
            previous_layout = reinterpret_cast<std::uintptr_t>(GetKeyboardLayout(0));
            const unsigned actions = commands.pending.exchange(0, std::memory_order_relaxed);
            if (actions & 1) {
                record("F5-before-reset", true);
                window.ResetInput();
                record("F5-reset-cleared-snapshot", true);
                window.CaptureInput();
                record("F5-fresh-capture", true);
                ++reset_count;
            }
            if (actions & 2) {
                cursor = cursor == CursorMode::Normal ? CursorMode::Hidden : cursor == CursorMode::Hidden ? CursorMode::Lock : CursorMode::Normal;
                window.SetCursorMode(cursor);
                record("F6-mode-reset-cleared-snapshot", true);
                window.CaptureInput();
                record("F6-fresh-capture", true);
                ++mode_count;
            }
            if (actions & 4) {
                record("F7-first-capture", true);
                window.CaptureInput();
                record("F7-second-capture", true);
                ++second_capture_count;
            }
            if (window.Focused() && !window.Minimized() && window.width > 0 && window.height > 0) {
                const auto& visible = window.Input();
                viewport(0, 0, window.width, window.height);
                clear_color(visible.left_button ? 0.75F : 0.12F, visible.right_button ? 0.25F : 0.5F,
                            visible.Down(Key::W) ? 0.8F : 0.25F, 1.0F);
                clear(GL_COLOR_BUFFER_BIT);
                GraphicsBridge::Present(window);
                ++presented_frames;
                report.Number("gl_presented_frames", presented_frames);
            }
            if (escaped) { window.Close(); break; }
            if (Window::Time() - start > 600.0) { timed_out = true; break; }
            SDL_Delay(16);
        }
        window.SetCursorMode(CursorMode::Normal);
        report.String("manual_result", "observations recorded; human acceptance not supplied");
        report.String("manual_exit_reason", escaped ? "production Escape observed" : timed_out ? "10-minute safety limit" : "window close requested");
        report.Number("manual_observations", observations);
        report.Number("manual_reset_commands", reset_count);
        report.Number("manual_cursor_commands", mode_count);
        report.Number("manual_second_capture_commands", second_capture_count);
        report.Number("manual_focus_loss_pulses", focus_losses);
        report.Number("manual_pointer_events", pointer_count);
        report.Number("manual_motion_events", motion_count);
        report.Number("manual_wheel_events", wheel_count);
        report.Number("manual_total_motion_dx", motion_dx);
        report.Number("manual_total_motion_dy", motion_dy);
        report.Number("manual_total_scroll_y", wheel_y);
        report.Boolean("manual_left_button_observed", left_seen);
        report.Boolean("manual_right_button_observed", right_seen);
        for (std::size_t index = 0; index < seen.size(); ++index) report.Boolean(std::string("manual_key_observed_") + names[index], seen[index]);
        report.Boolean("human_acceptance_passed", false);
        report.String("manual_console_log_scope", "Tool instructions and state/action messages; lower-level production diagnostics remain visible in console and failures are recorded in summary metadata");
    }
    struct VideoOwner {
        VideoOwner() { Window::Init(); }
        ~VideoOwner() { try { Window::Free(); } catch (const std::exception& error) { std::cerr << "cleanup: " << error.what() << '\n'; } }
    };
    bool Run(const std::string& test_case, Report& report, const std::filesystem::path& manual_output) {
        const bool manual = test_case == "hardware-manual";
        report.Number("gl_presented_frames", 0);
        // Owned WM_KEY fixtures deliberately use the SDL translated Windows message path, not Raw Input.
        if (!manual) Sdl(SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "0"), "SDL_SetHint(windows_raw_keyboard)");
        else {
            Check(SDL_GetHintBoolean(SDL_HINT_WINDOWS_RAW_KEYBOARD, true), "Manual observations require the default enabled SDL Raw Input policy, not an inherited disabled override");
            const char* configured_raw = SDL_GetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD);
            report.String("manual_raw_keyboard_hint", configured_raw ? configured_raw : "SDL default");
            report.Boolean("manual_input_injection_used", false);
        }
        VideoOwner video;
        const bool benchmark = test_case == "benchmark";
        std::unique_ptr<Window> window(Window::Create("SymoCraft production SDL3 input regression", 640, 480,
            benchmark, benchmark, manual ? WindowMode::OpenGL : WindowMode::Native));
        const HWND hwnd = NativeBridge::GetHandle(*window);
        DWORD process{};
        GetWindowThreadProcessId(hwnd, &process);
        Check(process == GetCurrentProcessId(), "The production window is not owned by the test process");
        SDL_Window* native = OwnedSdlWindow(hwnd);
        const SDL_WindowID id = SDL_GetWindowID(native);
        report.Number("owned_window_id", id);
        report.Number("owned_window_dpi", GetDpiForWindow(hwnd));
        report.Number("current_keyboard_layout_id", reinterpret_cast<std::uintptr_t>(GetKeyboardLayout(0)));
        report.String("sdl_video_driver", SDL_GetCurrentVideoDriver());
        report.Number("sdl_runtime_version", SDL_GetVersion());
        bool complete = true;
        const bool requires_focus = !manual && test_case != "benchmark" && test_case != "wait-close";
        if (requires_focus && !FocusOwned(*window, native, hwnd, report)) complete = false;
        if (complete) {
            if (test_case == "queue") QueueChecks(*window, id, report);
            else if (test_case == "wait-input") WaitChecks(*window, id, report);
            else if (test_case == "native-keyboard") {
                try { NativeKeys(*window, native, hwnd, report); }
                catch (const std::exception& error) {
                    if (std::string(error.what()).starts_with("PARTIAL:")) {
                        complete = false;
                        report.String("partial_reason", error.what());
                    } else throw;
                }
            }
            else if (test_case == "focus") FocusChecks(*window, hwnd, report);
            else if (test_case == "cursor-resize") CursorResize(*window, native, id, report);
            else if (test_case == "benchmark") BenchmarkChecks(*window, id, report);
            else if (test_case == "wait-close") WaitClose(*window, id, hwnd, report);
            else if (manual) ManualInput(*window, native, id, hwnd, manual_output, report);
            else throw std::invalid_argument("Unknown production input test case");
        }
        window->SetCursorMode(CursorMode::Normal);
        window->Destroy();
        window->Destroy();
        Check(!IsWindow(hwnd), "Production input-test window survived repeated Destroy");
        window.reset();
        Window::Free();
        Window::Free();
        Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0, "Production input test left a video reference");
        report.Boolean("window_destroyed", true);
        report.Boolean("sdl_shutdown_completed", true);
        return complete;
    }
}

int wmain(int argc, wchar_t** argv) {
    Report report;
    int exit_code{};
    bool manual{};
    std::filesystem::path manual_output;
    const int saved_stdout = _dup(_fileno(stdout));
    if (saved_stdout >= 0) _dup2(_fileno(stderr), _fileno(stdout));
    try {
        std::vector<std::wstring_view> arguments;
        for (int index = 1; index < argc; ++index) arguments.emplace_back(argv[index]);
        const auto options = InputTest::ParseOptions(arguments);
        manual = options.test_case == "hardware-manual";
        if (manual) {
            if (!std::filesystem::is_directory(options.output_directory))
                throw std::invalid_argument("hardware-manual requires a fresh existing directory");
            for (const auto name : {L"manual-summary.json", L"manual.stdout.log", L"manual-snapshots.jsonl", L"manual-console.stderr.log"})
                if (std::filesystem::exists(options.output_directory / name))
                    throw std::invalid_argument("Manual output must not overwrite earlier evidence");
            manual_output = options.output_directory;
        }
        report.String("case", options.test_case);
        const bool complete = Run(options.test_case, report, manual_output);
        report.String("status", manual ? "observation" : complete ? "pass" : "partial");
        exit_code = complete ? 0 : 2;
    } catch (const std::exception& error) {
        report.String("status", "fail");
        report.String("error", error.what());
        report.Boolean("video_active_after_failure", (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0);
        std::cerr << "Production SDL3 input probe failed: " << error.what() << '\n';
        exit_code = 1;
    }
    report.Number("checks", checks);
    report.String("scope", manual ? "actual imported production platform and default SDL Raw Input policy; user hardware observations, not automatic human acceptance; no injected input" :
        "actual imported production platform; SDL_PushEvent queue adaptation and owned Win32 messages only; no global injected input");
    report.String("unverified", "physical hardware held buttons/motion, Raw Input default path, alternate layouts, acceleration/feel, DPI/multimonitor/taskbar and other machines require manual acceptance");
    std::fflush(stdout);
    if (saved_stdout >= 0) { _dup2(saved_stdout, _fileno(stdout)); _close(saved_stdout); }
    if (manual && !manual_output.empty()) {
        try {
            std::ofstream metadata(manual_output / L"manual-summary.json");
            metadata.exceptions(std::ios::badbit | std::ios::failbit);
            metadata << report.Json();
            metadata.flush();
            std::ofstream stdout_log(manual_output / L"manual.stdout.log");
            stdout_log.exceptions(std::ios::badbit | std::ios::failbit);
            stdout_log << report.Json();
            stdout_log.flush();
        } catch (const std::exception& error) {
            report.String("status", "fail");
            report.String("metadata_error", error.what());
            std::cerr << "Manual input metadata could not be written: " << error.what() << '\n';
            exit_code = 1;
        }
    }
    std::cout << report.Json();
    return exit_code;
}
