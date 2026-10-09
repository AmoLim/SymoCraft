//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_INPUT_H
#define SYMOCRAFT_INPUT_H
#include "core.h"

namespace SymoCraft
{
    namespace Input
    {
        extern bool key_pressed[GLFW_KEY_LAST];             // key pressing
        extern bool key_begin_pressed[GLFW_KEY_LAST];  // key begin pressed

        // Set Window Size
        // Parameters: new window size
        void SetWindowSize(const glm::vec2& _window_size);

        // Inactivate all input interface
        void EndFrame();

        // Process Key Event
        // Parameters: key, action
        void ProcessKeyEvent( int key, int action);

        // Return Key Pressing status
        // Parameters: key
        bool IsKeyPressed( int key);

        // Return Key Begin Pressed status
        // Parameters: key
        bool IsKeyBeginPressed( int key);
    }
}

#endif //SYMOCRAFT_INPUT_H
