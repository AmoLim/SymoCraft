//
// Created by Amo on 2022/6/15.
//

#include "input/key_bindings.h"
#include "input/input.h"

namespace SymoCraft
{
    namespace KeyBindings
    {
        // A hash unordered map for keybindings
        static robin_hood::unordered_map<KeyBind, uint32_t> bindings;

        void Init()
        {
            bindings = robin_hood::unordered_map<KeyBind, uint32_t>();

            bindings[KeyBind::Exit] = GLFW_KEY_F10;
        }


        void SetKeyBinding( KeyBind key, uint32_t value)
        {
            bindings[key] = value;
        }

        uint32_t GetKeyBinding( KeyBind key)
        {
            // Keybind is in bindings
            if (bindings.find(key) != bindings.end())
                return bindings[key];

            AmoLogger_Warning("Unable to find KeyBindings key: %d", &key);
            return GLFW_KEY_LAST;
        }


        bool IsKeyBeginPressed( KeyBind key)
        {
            return Input::IsKeyBeginPressed(GetKeyBinding(key));
        }
    }

}
