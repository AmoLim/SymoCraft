#pragma once
#include <symocraft/platform/window.h>

namespace SymoCraft::Platform {
    // Only the renderer receives this private bridge; no native window handle escapes.
    struct GraphicsBridge {
        using Procedure = void (*)();
        static void MakeCurrent(Window& window);
        static bool HasCurrentContext();
        static Procedure GetProcedure(const char* name);
        static bool ExtensionSupported(const char* name);
        static void SetVsync(bool enabled);
        static void Present(Window& window);
    };
}
