#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <SDL3/SDL.h>
#define SDL_MAIN_NOIMPL
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include "failure_fixture.h"
#include <cstring>
#include <string>

// Reuse the unchanged ABI declaration; this test DLL never owns another SDL runtime.
#include <core/SDL_core_unsupported.h>
#include <video/SDL_video_unsupported.h>
struct DynamicApiTable {
#define SDL_DYNAPI_PROC(result, name, parameters, arguments, returns) result (SDLCALL* name) parameters;
#include <dynapi/SDL_dynapi_procs.h>
#undef SDL_DYNAPI_PROC
};

namespace {
    DynamicApiTable original{};
    PlatformFailureTrace trace{};
    std::string armed;

    bool Fail(const char* name) {
        if (armed != name) return false;
        armed.clear();
        ++trace.failures_injected;
        original.SDL_SetError("restricted production-platform test fixture: %s", name);
        return true;
    }

    SDL_Window* SDLCALL CreateTestWindow(const char* title, int width, int height, SDL_WindowFlags flags) {
        if (Fail("create-window")) return nullptr;
        auto* window = original.SDL_CreateWindow(title, width, height, flags);
        if (window) ++trace.windows_created;
        return window;
    }
    void SDLCALL DestroyWindow(SDL_Window* window) {
        if (window) ++trace.windows_destroyed;
        original.SDL_DestroyWindow(window);
    }
    SDL_GLContext SDLCALL CreateContext(SDL_Window* window) {
        if (Fail("create-context")) return nullptr;
        const auto context = original.SDL_GL_CreateContext(window);
        if (context) ++trace.contexts_created;
        return context;
    }
    bool SDLCALL DestroyContext(SDL_GLContext context) {
        if (context) ++trace.contexts_destroyed;
        const auto result = original.SDL_GL_DestroyContext(context);
        // The injected secondary failure is after actual release, not a driver leak claim.
        return Fail("destroy-context-after-release") ? false : result;
    }
    bool SDLCALL MakeCurrent(SDL_Window* window, SDL_GLContext context) {
        return Fail("make-current") ? false : original.SDL_GL_MakeCurrent(window, context);
    }
    bool SDLCALL SetSwapInterval(int interval) {
        return Fail("set-vsync") ? false : original.SDL_GL_SetSwapInterval(interval);
    }
    bool SDLCALL SwapWindow(SDL_Window* window) {
        return Fail("present") ? false : original.SDL_GL_SwapWindow(window);
    }
    bool SDLCALL ExtensionSupported(const char* name) {
        return Fail("query-extension") ? false : original.SDL_GL_ExtensionSupported(name);
    }
    SDL_FunctionPointer SDLCALL GetProcedure(const char* name) {
        if (name && std::strcmp(name, "glGetStringi") == 0 && Fail("query-extension")) return nullptr;
        return Fail("get-procedure") ? nullptr : original.SDL_GL_GetProcAddress(name);
    }
    bool SDLCALL CreateSurface(SDL_Window* window, VkInstance instance,
        const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface) {
        ++trace.surface_create_calls;
        return Fail("surface") ? false : original.SDL_Vulkan_CreateSurface(window, instance, allocator, surface);
    }
    void SDLCALL DestroySurface(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* allocator) {
        if (surface) ++trace.surfaces_destroyed;
        original.SDL_Vulkan_DestroySurface(instance, surface, allocator);
    }
    void SDLCALL QuitSubSystem(SDL_InitFlags flags) {
        if (flags & SDL_INIT_VIDEO) ++trace.video_quit_calls;
        original.SDL_QuitSubSystem(flags);
    }
}

extern "C" __declspec(dllexport) void ProbeArmFailure(const char* name) { armed = name ? name : ""; }
extern "C" __declspec(dllexport) PlatformFailureTrace ProbeReadFailureTrace() { return trace; }

extern "C" __declspec(dllexport) Sint32 SDLCALL SDL_DYNAPI_entry(Uint32 version, void* table, Uint32 size) {
    using Entry = Sint32 (SDLCALL*)(Uint32, void*, Uint32);
    const auto entry = reinterpret_cast<Entry>(GetProcAddress(GetModuleHandleW(nullptr), "SDL_DYNAPI_entry"));
    if (!entry || size != sizeof(DynamicApiTable) || entry(version, table, size) != 0) return -1;
    std::memcpy(&original, table, size);
    auto& replacement = *static_cast<DynamicApiTable*>(table);
    replacement.SDL_CreateWindow = CreateTestWindow;
    replacement.SDL_DestroyWindow = DestroyWindow;
    replacement.SDL_GL_CreateContext = CreateContext;
    replacement.SDL_GL_DestroyContext = DestroyContext;
    replacement.SDL_GL_MakeCurrent = MakeCurrent;
    replacement.SDL_GL_SetSwapInterval = SetSwapInterval;
    replacement.SDL_GL_SwapWindow = SwapWindow;
    replacement.SDL_GL_ExtensionSupported = ExtensionSupported;
    replacement.SDL_GL_GetProcAddress = GetProcedure;
    replacement.SDL_Vulkan_CreateSurface = CreateSurface;
    replacement.SDL_Vulkan_DestroySurface = DestroySurface;
    replacement.SDL_QuitSubSystem = QuitSubSystem;
    trace.fixture_loaded = 1;
    return 0;
}
