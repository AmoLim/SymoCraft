#pragma once
#include <SDL3/SDL_events.h>

namespace InputTest {
    inline SDL_Event Key(Uint32 type, SDL_Scancode code, SDL_WindowID id, bool repeat = false) {
        SDL_Event event{};
        event.type = type;
        event.key.windowID = id;
        event.key.scancode = code;
        // Deliberately unrelated logical symbol: gameplay must follow the physical scancode.
        event.key.key = SDLK_F1;
        event.key.down = type == SDL_EVENT_KEY_DOWN;
        event.key.repeat = repeat;
        return event;
    }
    inline SDL_Event Window(Uint32 type, SDL_WindowID id) {
        SDL_Event event{};
        event.type = type;
        event.window.windowID = id;
        return event;
    }
    inline SDL_Event Motion(float dx, float dy, SDL_WindowID id) {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.windowID = id;
        event.motion.xrel = dx;
        event.motion.yrel = dy;
        return event;
    }
    inline SDL_Event Wheel(float y, SDL_WindowID id, SDL_MouseWheelDirection direction = SDL_MOUSEWHEEL_NORMAL) {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.windowID = id;
        event.wheel.y = y;
        event.wheel.direction = direction;
        return event;
    }
    inline SDL_Event Button(Uint32 type, Uint8 button, SDL_WindowID id) {
        SDL_Event event{};
        event.type = type;
        event.button.windowID = id;
        event.button.button = button;
        return event;
    }
}
