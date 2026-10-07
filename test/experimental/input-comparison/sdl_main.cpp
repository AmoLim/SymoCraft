#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <glad/glad.h>
#include "comparison.h"
#include "input_candidate.h"
#include <iostream>
#include <stdexcept>

namespace C = SymoCraft::Experimental::Comparison;
namespace S = SymoCraft::Experimental::Sdl3;
namespace {
    void SdlRequire(bool condition, const char* operation) {
        if (!condition) throw std::runtime_error(std::string(operation) + ": " + SDL_GetError());
    }
    struct Runtime {
        SDL_Window* window{};
        SDL_GLContext context{};
        bool initialized{};
        ~Runtime() {
            if (context && !SDL_GL_DestroyContext(context)) std::cerr << "cleanup: SDL_GL_DestroyContext: " << SDL_GetError() << '\n';
            if (window) SDL_DestroyWindow(window);
            if (initialized) SDL_QuitSubSystem(SDL_INIT_VIDEO);
            SDL_Quit();
        }
        void Close() {
            if (context) {
                SdlRequire(SDL_GL_DestroyContext(context), "SDL_GL_DestroyContext");
                context = nullptr;
            }
            if (window) { SDL_DestroyWindow(window); window = nullptr; }
            if (initialized) { SDL_QuitSubSystem(SDL_INIT_VIDEO); initialized = false; }
            SDL_Quit();
        }
        void Open(const C::Options& options) {
            SDL_SetMainReady();
            SdlRequire(SDL_SetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE, options.system_scale ? "1" : "0"), "SDL_SetHint(relative system scale)");
            SdlRequire(SDL_InitSubSystem(SDL_INIT_VIDEO), "SDL_InitSubSystem(video)");
            initialized = true;
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,4), "SDL_GL_SetAttribute(major)");
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,6), "SDL_GL_SetAttribute(minor)");
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE), "SDL_GL_SetAttribute(core)");
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,1), "SDL_GL_SetAttribute(sample buffers)");
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,4), "SDL_GL_SetAttribute(samples)");
            SdlRequire(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24), "SDL_GL_SetAttribute(depth)");
            window = SDL_CreateWindow("SDL3 input comparison",960,640,SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
            SdlRequire(window != nullptr, "SDL_CreateWindow");
            SdlRequire(SDL_SetWindowPosition(window,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED), "SDL_SetWindowPosition");
            context = SDL_GL_CreateContext(window);
            SdlRequire(context != nullptr, "SDL_GL_CreateContext");
            SdlRequire(SDL_GL_MakeCurrent(window,context), "SDL_GL_MakeCurrent");
            C::Require(gladLoadGLLoader([](const char* name) { return reinterpret_cast<void*>(SDL_GL_GetProcAddress(name)); }) != 0 && GLAD_GL_VERSION_4_6,
                       "SDL comparison GLAD/GL4.6 unavailable");
            GLint samples{}, profile{};
            glGetIntegerv(GL_SAMPLES,&samples); glGetIntegerv(GL_CONTEXT_PROFILE_MASK,&profile);
            C::Require(samples == 4 && (profile & GL_CONTEXT_CORE_PROFILE_BIT), "SDL comparison GL Core/4x MSAA mismatch");
            SdlRequire(SDL_GL_SetSwapInterval(1), "SDL_GL_SetSwapInterval(1)");
            std::ofstream identity(options.output / "runtime.txt");
            identity << "SDL_version=" << SDL_GetVersion() << "\nSDL_revision=" << SDL_GetRevision()
                     << "\nvideo_driver=" << SDL_GetCurrentVideoDriver() << "\nGL=" << glGetString(GL_VERSION)
                     << "\nGL_samples=" << samples << "\nGL_vendor=" << glGetString(GL_VENDOR)
                     << "\nGL_renderer=" << glGetString(GL_RENDERER) << "\nrelative_system_scale="
                     << SDL_GetHint(SDL_HINT_MOUSE_RELATIVE_SYSTEM_SCALE) << "\n";
            identity.flush(); C::Require(identity.good(), "SDL runtime identity write failed");
        }
        void Cursor(SymoCraft::CursorMode mode) {
            SdlRequire(SDL_SetWindowRelativeMouseMode(window,mode == SymoCraft::CursorMode::Lock), "SDL_SetWindowRelativeMouseMode");
            if (mode == SymoCraft::CursorMode::Normal) SdlRequire(SDL_ShowCursor(), "SDL_ShowCursor");
            else SdlRequire(SDL_HideCursor(), "SDL_HideCursor");
        }
        S::PhysicalInputState Physical() const {
            S::PhysicalInputState state;
            state.window_id = SDL_GetWindowID(window);
            state.focused = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
            state.minimized = (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) != 0;
            int count{};
            const bool* keys = SDL_GetKeyboardState(&count);
            C::Require(keys != nullptr, "SDL_GetKeyboardState returned no state");
            for (std::size_t index = 0; index < state.keys.size(); ++index) {
                const auto scan = static_cast<int>(S::ControlScancodes[index]);
                state.keys[index] = scan >= 0 && scan < count && keys[scan];
            }
            const auto buttons = SDL_GetMouseState(nullptr,nullptr);
            state.left_button = (buttons & SDL_BUTTON_LMASK) != 0;
            state.right_button = (buttons & SDL_BUTTON_RMASK) != 0;
            return state;
        }
    };
    bool Wait(SDL_Event& event) {
        SDL_ClearError();
        const bool received = SDL_WaitEventTimeout(&event,40);
        if (!received && *SDL_GetError()) SdlRequire(false, "SDL_WaitEventTimeout");
        return received;
    }
}
int main(int argc, char** argv) {
    try {
        const auto options = C::Parse(argc,argv);
        C::Session session(options);
        Runtime runtime;
        runtime.Open(options);
        S::InputCandidate input(SDL_GetWindowID(runtime.window),false,runtime.Physical().focused);
        runtime.Cursor(options.cursor);
        input.ResetInput(); input.SynchronizePhysicalState(runtime.Physical());
        unsigned rendered{};
        {
        C::Scene scene;
        C::Controls controls;
        auto cursor = options.cursor;
        bool paused = true;
        double previous = static_cast<double>(SDL_GetTicksNS()) / 1e9;
        while (!input.ShouldClose() && (!options.frames || session.Frame() < static_cast<unsigned>(options.frames))) {
            input.PollAndPublish([](SDL_Event& event) { return SDL_PollEvent(&event); });
            input.SynchronizePhysicalState(runtime.Physical());
            input.CaptureKeys();
            const auto now = static_cast<double>(SDL_GetTicksNS()) / 1e9;
            const bool active = input.Focused() && !input.Minimized() && !input.Input().focus_lost;
            unsigned actions{};
            if (controls.ChangeCursor(active)) {
                actions |= 1;
                cursor = C::NextCursor(cursor);
                runtime.Cursor(cursor);
                input.ResetInput(); input.SynchronizePhysicalState(runtime.Physical()); input.CaptureKeys();
            }
            if (controls.ResetInput(active)) {
                actions |= 4;
                input.ResetInput(); input.SynchronizePhysicalState(runtime.Physical()); input.CaptureKeys();
            }
            if (controls.ResetCamera(active)) { actions |= 2; scene.ResetCamera(); }
            if (active && paused) {
                input.ResetInput(); input.SynchronizePhysicalState(runtime.Physical()); input.CaptureKeys();
            }
            if (active) scene.Update(input.Input(),paused ? 0.0 : now - previous);
            session.Record(now,input.Input(),input.Focused(),input.Minimized(),cursor,scene.CameraPose(),actions);
            if (input.Input().Down(SymoCraft::Key::Escape)) break;
            if (!active) {
                input.ResetInput(); paused = true; previous = now;
                input.WaitAndDrain(Wait,[](SDL_Event& event) { return SDL_PollEvent(&event); });
                continue;
            }
            paused = false; previous = now;
            int width{}, height{};
            SdlRequire(SDL_GetWindowSizeInPixels(runtime.window,&width,&height), "SDL_GetWindowSizeInPixels");
            if (width > 0 && height > 0) {
                scene.Draw(width,height,input.Input(),cursor);
                if (!rendered) scene.SaveFrame(options.output / "first-frame.bmp",width,height);
                SdlRequire(SDL_GL_SwapWindow(runtime.window), "SDL_GL_SwapWindow");
                ++rendered;
            }
            const auto title = scene.Title(input.Input(),input.Focused(),input.Minimized(),cursor);
            SdlRequire(SDL_SetWindowTitle(runtime.window,title.c_str()), "SDL_SetWindowTitle");
        }
        }
        runtime.Close();
        session.Finish(true,rendered);
        std::cout << "SDL input comparison exited normally; " << rendered << " rendered frames.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "SDL input comparison failed: " << error.what() << '\n';
        return 1;
    }
}
