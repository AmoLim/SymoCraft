//
// Created by Amo on 2022/6/15.
//

#include "input/key_handler.h"
#include "input/input.h"
#include "input/key_bindings.h"
#include "core/application.h"
#include "core/window.h"

namespace SymoCraft
{
    namespace KeyHandler
    {
        void Update()
        {
            if (KeyBindings::IsKeyBeginPressed(KeyBind::Exit))
            {
                Application::GetWindow().Close();
            }
        }
    }
}