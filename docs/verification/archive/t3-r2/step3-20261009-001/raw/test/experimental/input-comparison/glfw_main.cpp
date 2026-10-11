#include "comparison.h"
#include <graphics_bridge.h>
#include <glad/glad.h>
#include <iostream>

namespace C = SymoCraft::Experimental::Comparison;
namespace {
    struct Runtime {
        Runtime() { SymoCraft::Window::Init(); }
        ~Runtime() { SymoCraft::Window::Free(); }
    };
}
int main(int argc, char** argv) {
    try {
        const auto options = C::Parse(argc,argv);
        C::Session session(options);
        Runtime runtime;
        std::unique_ptr<SymoCraft::Window> window(SymoCraft::Window::Create("GLFW input comparison",960,640));
        C::Require(gladLoadGLLoader([](const char* name) { return reinterpret_cast<void*>(SymoCraft::Platform::GraphicsBridge::GetProcedure(name)); }) != 0 && GLAD_GL_VERSION_4_6,
                   "GLFW comparison GLAD/GL4.6 unavailable");
        GLint samples{}, profile{};
        glGetIntegerv(GL_SAMPLES,&samples); glGetIntegerv(GL_CONTEXT_PROFILE_MASK,&profile);
        C::Require(samples == 4 && (profile & GL_CONTEXT_CORE_PROFILE_BIT), "GLFW comparison GL Core/4x MSAA mismatch");
        std::ofstream identity(options.output / "runtime.txt");
        identity << "platform=frozen GLFW Release library\nGL=" << glGetString(GL_VERSION) << "\nGL_samples=" << samples
                 << "\nGL_vendor=" << glGetString(GL_VENDOR) << "\nGL_renderer=" << glGetString(GL_RENDERER) << "\n";
        identity.flush(); C::Require(identity.good(), "GLFW runtime identity write failed");
        window->SetCursorMode(options.cursor); window->ResetInput();
        C::Scene scene;
        C::Controls controls;
        auto cursor = options.cursor;
        bool paused = true;
        unsigned rendered{};
        double previous = SymoCraft::Window::Time();
        while (!window->ShouldClose() && (!options.frames || session.Frame() < static_cast<unsigned>(options.frames))) {
            window->PollInt(); window->CaptureInput();
            const auto now = SymoCraft::Window::Time();
            const bool active = window->Focused() && !window->Minimized() && !window->Input().focus_lost;
            unsigned actions{};
            if (controls.ChangeCursor(active)) {
                actions |= 1;
                cursor = C::NextCursor(cursor); window->SetCursorMode(cursor); window->ResetInput(); window->CaptureInput();
            }
            if (controls.ResetInput(active)) { actions |= 4; window->ResetInput(); window->CaptureInput(); }
            if (controls.ResetCamera(active)) { actions |= 2; scene.ResetCamera(); }
            if (active && paused) { window->ResetInput(); window->CaptureInput(); }
            if (active) scene.Update(window->Input(),paused ? 0.0 : now - previous);
            session.Record(now,window->Input(),window->Focused(),window->Minimized(),cursor,scene.CameraPose(),actions);
            if (window->Input().Down(SymoCraft::Key::Escape)) break;
            if (!active) {
                window->ResetInput(); paused = true; previous = now;
                window->WaitEvents(0.04);
                continue;
            }
            paused = false; previous = now;
            if (window->width > 0 && window->height > 0) {
                scene.Draw(window->width,window->height,window->Input(),cursor);
                if (!rendered) scene.SaveFrame(options.output / "first-frame.bmp",window->width,window->height);
                SymoCraft::Platform::GraphicsBridge::Present(*window);
                ++rendered;
            }
            const auto title = scene.Title(window->Input(),window->Focused(),window->Minimized(),cursor);
            window->SetTitle(title.c_str());
        }
        session.Finish(true,rendered);
        std::cout << "GLFW input comparison exited normally; " << rendered << " rendered frames.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "GLFW input comparison failed: " << error.what() << '\n';
        return 1;
    }
}
