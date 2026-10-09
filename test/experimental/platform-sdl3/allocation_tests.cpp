#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <symocraft/platform/window.h>
#include <native_bridge.h>
#include "input_test_events.h"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <io.h>
#include <iostream>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string>

namespace AllocationFixture {
    std::atomic_size_t armed_size{};
    std::atomic_uint failures{};
    bool Fail(std::size_t size) noexcept {
        auto expected = size;
        if (size && armed_size.compare_exchange_strong(expected, 0)) {
            ++failures;
            return true;
        }
        return false;
    }
}

// Only this explicit probe overrides C++ allocation; SDL still uses its real allocator.
void* operator new(std::size_t size) {
    if (AllocationFixture::Fail(size)) throw std::bad_alloc();
    if (void* value = std::malloc(size ? size : 1)) return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

namespace {
    using SymoCraft::Window;
    using SymoCraft::InputSnapshot;
    using SymoCraft::PointerEvent;
    using SymoCraft::Platform::NativeBridge;
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
    void Sdl(bool success, const char* operation) {
        if (!success) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
    bool NestedBadAlloc(const std::exception& error) {
        if (dynamic_cast<const std::bad_alloc*>(&error)) return true;
        try { std::rethrow_if_nested(error); }
        catch (const std::exception& nested) { return NestedBadAlloc(nested); }
        catch (...) { return false; }
        return false;
    }
    template<typename Action> std::string ExpectFailure(Action&& action, const char* operation) {
        try { action(); }
        catch (const std::exception& error) {
            Check(!dynamic_cast<const std::bad_alloc*>(&error), "The allocation failure lacked an owning operation diagnostic");
            Check(std::string(error.what()).find(operation) != std::string::npos, "The expected operation was absent from the diagnostic");
            Check(NestedBadAlloc(error), "The original allocation failure was not retained as a nested exception");
            return error.what();
        }
        throw std::runtime_error("The armed production allocation failure did not propagate");
    }
    struct WindowTrace {
        SDL_WindowID id{};
        HWND hwnd{};
        unsigned shown{}, destroyed{};
        bool context_observed{}, context_gone_before_window{};
    };
    bool SDLCALL WatchWindow(void* userdata, SDL_Event* event) noexcept {
        auto& trace = *static_cast<WindowTrace*>(userdata);
        if (!SDL_IsMainThread()) return true;
        if (event->type == SDL_EVENT_WINDOW_SHOWN) {
            if (auto* native = SDL_GetWindowFromID(event->window.windowID)) {
                trace.id = event->window.windowID;
                const auto properties = SDL_GetWindowProperties(native);
                trace.hwnd = static_cast<HWND>(SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
                trace.context_observed = SDL_GL_GetCurrentContext() != nullptr;
                ++trace.shown;
            }
        } else if (event->type == SDL_EVENT_WINDOW_DESTROYED && event->window.windowID == trace.id) {
            ++trace.destroyed;
            trace.context_gone_before_window = SDL_GL_GetCurrentContext() == nullptr;
        }
        return true;
    }
    struct VideoOwner {
        bool external{}, platform{};
        VideoOwner() {
            SDL_SetMainReady();
            Sdl(SDL_InitSubSystem(SDL_INIT_VIDEO), "external SDL_InitSubSystem(video)");
            external = true;
            try { Window::Init(); platform = true; }
            catch (...) { SDL_QuitSubSystem(SDL_INIT_VIDEO); external = false; throw; }
        }
        ~VideoOwner() {
            if (platform) {
                try { Window::Free(); }
                catch (const std::exception& error) { std::cerr << "cleanup: " << error.what() << '\n'; }
            }
            if (external) SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }
        void Finish(Report& report) {
            Window::Free();
            platform = false;
            Window::Free();
            Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0, "Window::Free released the external video owner");
            report.Boolean("external_video_owner_preserved", true);
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
            external = false;
            Check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) == 0, "A video ownership reference leaked");
            report.Boolean("sdl_shutdown_completed", true);
        }
    };
    struct EventWatch {
        WindowTrace& trace;
        explicit EventWatch(WindowTrace& value) : trace(value) { Sdl(SDL_AddEventWatch(WatchWindow, &trace), "SDL_AddEventWatch"); }
        ~EventWatch() { SDL_RemoveEventWatch(WatchWindow, &trace); }
    };
    void NoWindows() {
        int count{};
        auto** windows = SDL_GetWindows(&count);
        Sdl(windows != nullptr, "SDL_GetWindows");
        SDL_free(windows);
        Check(count == 0, "A native SDL window survived production cleanup");
        Check(SDL_GL_GetCurrentContext() == nullptr, "A current OpenGL context survived production cleanup");
    }
    bool SameSnapshot(const InputSnapshot& left, const InputSnapshot& right) {
        if (left.keys != right.keys || left.left_button != right.left_button || left.right_button != right.right_button ||
            left.focus_lost != right.focus_lost || left.pointer_events.size() != right.pointer_events.size()) return false;
        for (std::size_t index = 0; index < left.pointer_events.size(); ++index) {
            const auto& a = left.pointer_events[index];
            const auto& b = right.pointer_events[index];
            if (a.mouse_dx != b.mouse_dx || a.mouse_dy != b.mouse_dy || a.scroll_y != b.scroll_y) return false;
        }
        return true;
    }
    std::size_t GrowthCapacity() {
        std::vector<PointerEvent> calibration;
        calibration.reserve(64);
        for (unsigned index = 0; index < 65; ++index) calibration.push_back({});
        return calibration.capacity();
    }
    void Run(const std::string& test_case, Report& report) {
        const bool object = test_case == "window-object";
        const bool reserve = test_case == "input-reserve";
        const bool wait = test_case == "event-growth-wait";
        Check(object || reserve || wait || test_case == "event-growth-poll", "Unknown allocation test case");
        VideoOwner video;
        WindowTrace trace;
        {
            EventWatch watch(trace);
            std::unique_ptr<Window> window;
            if (object || reserve) {
                const std::size_t size = object ? sizeof(Window) : 64 * sizeof(PointerEvent);
                report.Number("armed_allocation_bytes", size);
                AllocationFixture::armed_size = size;
                report.String("expected_diagnostic", ExpectFailure([&] {
                    window.reset(Window::Create("SymoCraft production allocation failure", 640, 480));
                }, object ? "Window::Create: Window/Impl allocation failed" : "Window::Create: input buffers could not be allocated"));
                Check(!window, "Failed Window::Create published a window");
                if (reserve) {
                    Check(trace.shown == 1 && trace.context_observed, "Input reserve failure did not follow a real shown window and GL context");
                    Check(trace.destroyed == 1 && trace.context_gone_before_window, "Input reserve failure did not release context before window");
                    Check(trace.hwnd && !IsWindow(trace.hwnd), "The reserve-failed HWND survived factory RAII cleanup");
                } else Check(trace.shown == 0 && trace.destroyed == 0, "Window object failure unexpectedly created an SDL window");
            } else {
                const std::size_t capacity = GrowthCapacity();
                report.Number("calibrated_growth_capacity", capacity);
                report.Number("armed_allocation_bytes", capacity * sizeof(PointerEvent));
                window.reset(Window::Create("SymoCraft production event allocation failure", 640, 480));
                const HWND hwnd = NativeBridge::GetHandle(*window);
                const SDL_WindowID id = trace.id;
                Check(id != 0 && trace.context_observed && IsWindow(hwnd), "Growth fixture did not create a real window and GL context");
                report.Boolean("owned_native_visible_before_show", IsWindowVisible(hwnd) != FALSE);
                ShowWindow(hwnd, SW_SHOW);
                report.Boolean("owned_native_visible_after_show", IsWindowVisible(hwnd) != FALSE);
                report.Boolean("owned_foreground_request_returned", SetForegroundWindow(hwnd) != FALSE);
                for (unsigned attempt = 0; attempt < 100; ++attempt) {
                    window->PollInt();
                    if (window->Focused() && GetForegroundWindow() == hwnd) break;
                    SDL_Delay(2);
                }
                report.Boolean("owned_foreground_observed", window->Focused() && GetForegroundWindow() == hwnd);
                window->PollInt();
                window->CaptureInput();
                SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
                window->ResetInput();
                auto prime_focus = InputTest::Window(SDL_EVENT_WINDOW_FOCUS_GAINED, id);
                auto prime_wheel = InputTest::Wheel(0.75F, id);
                Sdl(SDL_PushEvent(&prime_focus), "SDL_PushEvent(prime focus)");
                Sdl(SDL_PushEvent(&prime_wheel), "SDL_PushEvent(prime wheel)");
                window->PollInt();
                window->CaptureInput();
                const InputSnapshot previous = window->Input();
                report.Number("previous_published_pointer_count", previous.pointer_events.size());
                report.Boolean("previous_nonempty_snapshot_observed", !previous.pointer_events.empty());
                if (previous.pointer_events.empty())
                    report.String("snapshot_proof_limitation", "No published pointer survived authoritative focus convergence; unchanged empty baseline cannot prove preservation of a nonempty snapshot");
                SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);
                // Queued focus only enters the reducer; this is not proof of OS foreground focus.
                auto focus = InputTest::Window(SDL_EVENT_WINDOW_FOCUS_GAINED, id);
                Sdl(SDL_PushEvent(&focus), "SDL_PushEvent(focus fixture)");
                for (unsigned index = 0; index < 65; ++index) {
                    auto event = InputTest::Wheel(0.25F, id);
                    Sdl(SDL_PushEvent(&event), "SDL_PushEvent(wheel fixture)");
                }
                AllocationFixture::armed_size = capacity * sizeof(PointerEvent);
                report.String("expected_diagnostic", ExpectFailure([&] {
                    if (wait) window->WaitEvents(0.1); else window->PollInt();
                }, "Window input event adaptation failed"));
                Check(SameSnapshot(previous, window->Input()), "Failed event adaptation published a changed or empty-success snapshot");
                report.Boolean("previous_snapshot_preserved", true);
                report.String("capture_rethrow_diagnostic", ExpectFailure([&] { window->CaptureInput(); }, "Window input event adaptation failed"));
                report.Boolean("capture_rethrows_saved_failure", true);
                window->Destroy();
                window->Destroy();
                Check(!IsWindow(hwnd), "Growth-failed HWND survived repeated Destroy");
                Check(trace.destroyed == 1 && trace.context_gone_before_window, "Growth cleanup did not release context before window");
                window.reset();
            }
            Check(AllocationFixture::failures == 1 && AllocationFixture::armed_size == 0, "The allocation failure was not an exact-size one-shot injection");
            NoWindows();
            report.Boolean("window_destroyed", true);
            report.Boolean("gl_context_released", true);
            report.Number("windows_shown", trace.shown);
            report.Number("windows_destroyed", trace.destroyed);
            report.Boolean("actual_context_observed", trace.context_observed);
            report.Boolean("context_gone_before_window_destroy_event", trace.context_gone_before_window);
        }
        video.Finish(report);
    }
}

int main(int argc, char** argv) {
    Report report;
    int exit_code{};
    const int saved_stdout = _dup(_fileno(stdout));
    const bool redirected = saved_stdout >= 0 && _dup2(_fileno(stderr), _fileno(stdout)) == 0;
    try {
        Check(redirected, "Could not isolate logger output from the JSON result stream");
        if (argc != 3 || std::string(argv[1]) != "--case")
            throw std::invalid_argument("Use --case window-object|input-reserve|event-growth-poll|event-growth-wait");
        report.String("case", argv[2]);
        Run(argv[2], report);
        report.String("status", "pass");
    } catch (const std::exception& error) {
        AllocationFixture::armed_size = 0;
        report.String("status", "fail");
        report.String("error", error.what());
        report.Boolean("video_active_after_failure", (SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0);
        std::cerr << "Production SDL3 allocation probe failed: " << error.what() << '\n';
        exit_code = 1;
    }
    report.Number("checks", checks);
    report.Number("failures_injected", AllocationFixture::failures.load());
    report.String("scope", "actual imported production platform; test-only exact-size C++ allocation override; queued synthetic reducer fault route only");
    report.String("unverified", "OS allocation exhaustion, hardware focus/input, SDL allocator exhaustion and real driver resource-release failures");
    std::fflush(stdout);
    if (saved_stdout >= 0) { _dup2(saved_stdout, _fileno(stdout)); _close(saved_stdout); }
    std::cout << report.Json();
    return exit_code;
}
