//
// Created by Amo on 2022/6/15.
//

#include "input/input.h"
#include "core.h"

namespace SymoCraft
{

    namespace Input
    {
        bool key_pressed[GLFW_KEY_LAST] = {};
        bool key_begin_pressed[GLFW_KEY_LAST] = {};

        void EndFrame()
        {
            AmoBase::AmoMemory_ZeroMem(key_begin_pressed, sizeof(key_begin_pressed));
        }

        void ProcessKeyEvent( int key, int action)
        {
            // invalid key value
            if (key < 0 || key > GLFW_KEY_LAST)
                return;

            if (action == GLFW_PRESS)
            {
                key_pressed[key] = true;
                key_begin_pressed[key] = true;
            }
            else if (action == GLFW_RELEASE)
            {
                key_pressed[key] = false;
                key_begin_pressed[key] = false;
            }
        }

        bool IsKeyPressed( int key)
        {
            AmoLogger_Assert( key >= 0 && key < GLFW_KEY_LAST, "Invalid key. ");
            return key_pressed[key];
        }

        bool IsKeyBeginPressed( int key)
        {
            AmoLogger_Assert( key >= 0 && key < GLFW_KEY_LAST, "Invalid key. ");
            return key_begin_pressed[key];
        }
    }
}