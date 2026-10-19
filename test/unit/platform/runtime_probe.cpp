#include <symocraft/platform/window.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <cstdlib>
#include <new>
#include <string>
#include <typeindex>

namespace {
    thread_local bool fail_new = false;
    thread_local int fail_new_after = -1;
    using namespace SymoCraft;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    template<class Call> void Reject(Call&& call) {
        try { call(); } catch (const std::logic_error&) { return; }
        throw std::runtime_error("Expected lifecycle rejection not reported");
    }
    bool AllocationCause(const std::exception& error) {
        if (dynamic_cast<const std::bad_alloc*>(&error)) return true;
        try { std::rethrow_if_nested(error); }
        catch (const std::exception& nested) { return AllocationCause(nested); }
        catch (...) { return false; }
        return false;
    }
}
void* operator new(std::size_t bytes) {
    if (fail_new || fail_new_after == 0) throw std::bad_alloc();
    if (fail_new_after > 0) --fail_new_after;
    auto* result = std::malloc(bytes ? bytes : 1);
    if (!result) throw std::bad_alloc();
    return result;
}
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }
int main() {
    using namespace SymoCraft;
    std::set_terminate([] { std::_Exit(86); });
    bool external_video = false;
    try {
        SDL_SetMainReady();
        Require(SDL_InitSubSystem(SDL_INIT_VIDEO), "External SDL video setup failed");
        external_video = true;
        Platform::Runtime runtime;
        Reject([&] { Platform::Runtime second; });
        bool wrong_thread_rejected = false;
        std::thread wrong([&] {
            try { runtime.Shutdown(); } catch (const std::logic_error&) { wrong_thread_rejected = true; }
        });
        wrong.join(); Require(wrong_thread_rejected && runtime.Active(), "Wrong thread changed Runtime ownership");
        bool bad_title = false;
        try { Window::Create(runtime, nullptr, 320, 240, false, true, WindowMode::Native); }
        catch (const std::invalid_argument&) { bad_title = true; }
        Require(bad_title, "Invalid Window factory definition accepted");
        unsigned factory_failures = 0;
        bool factory_succeeded = false;
        for (int budget = 0; budget < 32 && !factory_succeeded; ++budget) {
            std::unique_ptr<Window> candidate;
            fail_new_after = budget;
            try {
                candidate = Window::Create(runtime, "T4 window factory allocation sweep", 320, 240,
                    false, true, WindowMode::Native);
                factory_succeeded = true;
            } catch (const std::exception& error) {
                if (!AllocationCause(error)) { fail_new_after = -1; throw; }
                ++factory_failures;
            }
            fail_new_after = -1;
            if (candidate) candidate->Destroy();
            int remaining = 0;
            auto** owned_windows = SDL_GetWindows(&remaining); SDL_free(owned_windows);
            Require(remaining == 0 && runtime.Active(), "Partial factory failure leaked a native window/video reference");
        }
        Require(factory_succeeded && factory_failures >= 3, "Window allocation sweep did not reach owned partial creation");
        auto window = Window::Create(runtime, "T4 native lifecycle verification", 640, 360, false, true, WindowMode::Native);
        Reject([&] { runtime.Shutdown(); });
        Reject([&] { Window::Create(runtime, "second window"); });
        Require(window->Width() > 0 && window->Height() > 0, "Actual drawable dimensions missing");
        int count = 0;
        SDL_Window** windows = SDL_GetWindows(&count);
        Require(windows && count == 1, "Expected exactly one real SDL window");
        auto* native = windows[0];
        const auto id = SDL_GetWindowID(native); SDL_free(windows);
        window->SetSize(720, 480); window->PollInt();
        int width = 0, height = 0;
        Require(SDL_GetWindowSizeInPixels(native, &width, &height) && window->Width() == width && window->Height() == height,
            "Window exposed requested rather than actual drawable dimensions");
        Require(SDL_MinimizeWindow(native), "Cannot minimize the owned native window");
        for (int attempt = 0; attempt < 20 && !window->Minimized(); ++attempt) { window->WaitEvents(0.01); window->PollInt(); }
        Require(window->Minimized(), "Real native minimize fact missing");
        Require(SDL_RestoreWindow(native), "Cannot restore the owned native window");
        for (int attempt = 0; attempt < 20 && window->Minimized(); ++attempt) { window->WaitEvents(0.01); window->PollInt(); }
        Require(!window->Minimized(), "Real native restore fact missing");
        window->PollInt(); InputFrame discard; window->CaptureInput(discard); discard.Clear();
        SDL_Event down{}; down.type = SDL_EVENT_KEY_DOWN; down.key.windowID = id; down.key.scancode = SDL_SCANCODE_ESCAPE;
        SDL_Event up = down; up.type = SDL_EVENT_KEY_UP;
        Require(SDL_PushEvent(&down) && SDL_PushEvent(&up), "Cannot queue window-local test input");
        window->WaitEvents(0.01); window->PollInt();
        InputFrame frame; window->CaptureInput(frame);
        Require(frame.Down(Key::Escape), "Wait lost its first queued key event");
        const auto old = frame;
        frame.Clear(); window->PollInt(); window->CaptureInput(frame);
        Require(!frame.Down(Key::Escape), "Wait/Poll published an event twice");
        window->ResetInput(); window->Destroy(); window->Destroy();
        Require(old.Down(Key::Escape), "Destroy mutated caller-owned old input");
        Reject([&] { frame.Clear(); window->CaptureInput(frame); });
        Reject([&] { window->GetAspectRatio(); });
        window.reset();
        auto fault_window = Window::Create(runtime, "T4 input allocation latch", 320, 240, false, true, WindowMode::Native);
        windows = SDL_GetWindows(&count); Require(windows && count == 1, "Fault window missing");
        const auto fault_id = SDL_GetWindowID(windows[0]); SDL_free(windows);
        SDL_Event gain{}; gain.type = SDL_EVENT_WINDOW_FOCUS_GAINED; gain.window.windowID = fault_id;
        Require(SDL_PushEvent(&gain), "Cannot queue controlled focus fact");
        for (int index = 0; index < 80; ++index) {
            SDL_Event motion{}; motion.type = SDL_EVENT_MOUSE_MOTION; motion.motion.windowID = fault_id; motion.motion.xrel = 1;
            Require(SDL_PushEvent(&motion), "Cannot queue controlled pointer event");
        }
        std::exception_ptr failure;
        fail_new = true;
        try { fault_window->PollInt(); } catch (...) { failure = std::current_exception(); }
        fail_new = false;
        Require(failure != nullptr, "Actual input-vector allocation failure was not reached");
        std::string failure_text;
        std::type_index failure_type(typeid(void));
        try { std::rethrow_exception(failure); }
        catch (const std::exception& error) { failure_text = error.what(); failure_type = typeid(error); }
        for (int operation = 0; operation < 3; ++operation) {
            bool repeated = false;
            try {
                if (operation == 0) fault_window->PollInt();
                if (operation == 1) { InputFrame rejected; fault_window->CaptureInput(rejected); }
                if (operation == 2) fault_window->ResetInput();
            } catch (const std::exception& error) {
                repeated = failure_type == typeid(error) && failure_text == error.what();
            }
            // MSVC may copy exception objects in current_exception; compare preserved facts, not pointer identity.
            Require(repeated, "Input failure latch was cleared or replaced by fake success");
        }
        fault_window->Destroy(); fault_window.reset();
        runtime.Shutdown(); runtime.Shutdown();
        Require(SDL_WasInit(SDL_INIT_VIDEO) != 0, "Runtime released another SDL owner's video reference");
        Reject([&] { Window::Create(runtime, "after shutdown"); });
        SDL_QuitSubSystem(SDL_INIT_VIDEO); external_video = false;
        std::cout << "Real SDL Native lifecycle, external owner and synthetic Wait/input handoff passed; no physical keyboard/GPU claim\n";
        return 0;
    } catch (const std::exception& error) {
        fail_new = false;
        fail_new_after = -1;
        if (external_video) SDL_QuitSubSystem(SDL_INIT_VIDEO);
        std::cerr << error.what() << '\n'; return 1;
    }
}
