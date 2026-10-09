#pragma once
#include <symocraft/platform/window.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace SymoCraft::Platform {
    struct NativeBridge {
        // Borrowed until Window::Destroy; never pass this handle to DestroyWindow.
        static HWND GetHandle(Window& window);
    };
}
