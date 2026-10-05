//
// Created by Amo on 2022/6/15.
//

#ifndef SYMOCRAFT_KEYBINDINGS_H
#define SYMOCRAFT_KEYBINDINGS_H
#include "core.h"

namespace SymoCraft
{

    enum class KeyBind
    {
        Exit,
        Escape
    };

    namespace KeyBindings
    {
        // KeyBindings Initializing
        void Init();

        // Set key bindings
        // Parameters: Key, Value
        void SetKeyBinding( KeyBind key, uint32_t value);

        // Get key bindings
        // Parameters: Key
        uint32_t GetKeyBinding( KeyBind key);

        // Is key begin pressed
        // Parameters: Key
        bool IsKeyBeginPressed( KeyBind key);
    }

}
#endif //SYMOCRAFT_KEYBINDINGS_H
