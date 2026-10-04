//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_WINDOW_H
#define SYMOCRAFT_WINDOW_H

#include "core.h"

namespace SymoCraft
{

    enum class CursorMode : uint8_t
    {
        Hidden = 0,
        Lock,
        Normal
    };

    class Window
    {
    public:
        // public data members
        int width{};
        int height{};
        const char* title{};
        void* window_ptr{};

        // GLFW function interface
        void MakeContextCurrent();
        void PollInt();
        void SwapBuffers();     // swap buffer
        bool ShouldClose();     // return shouldClose

        // Close the window
        void Close();

        // Destroy the window
        void Destroy();

        // Set Cursor Mode of the window
        // Parameters: Cursor Mode
        void SetCursorMode(CursorMode cursorMode);

        // Set Vsync
        // Parameters: On or Off
        void SetVsync(bool on);

        // Set Title
        // Parameters: Title
        void SetTitle(const char* new_title);

        // Set the size of the window
        // Parameters: width, height
        void SetSize(int width, int height);

        // Get Aspect ratio of the window
        float GetAspectRatio() const;

        // Creat a window
        // Static
        // Parameters: Window title
        static Window* Create(const char* window_title, int width = 0, int height = 0);

        // Initialize glfw
        // Static
        static void Init();

        // Free window
        // Static
        static void Free();

    };
}

#endif //SYMOCRAFT_WINDOW_H
