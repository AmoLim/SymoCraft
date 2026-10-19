#include "input_state.h"
#include <symocraft/platform/window.h>
#include <iostream>
#include <stdexcept>

namespace {
    using namespace SymoCraft;
    using Platform::Detail::InputState;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    SDL_Event KeyEvent(SDL_Scancode key, bool down, bool repeat = false, SDL_WindowID id = 7) {
        SDL_Event event{}; event.type = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.windowID = id; event.key.scancode = key; event.key.repeat = repeat;
        return event;
    }
    SDL_Event WindowEvent(Uint32 type) { SDL_Event event{}; event.type = type; event.window.windowID = 7; return event; }
    SDL_Event Motion(float x, float y) {
        SDL_Event event{}; event.type = SDL_EVENT_MOUSE_MOTION; event.motion.windowID = 7;
        event.motion.xrel = x; event.motion.yrel = y; return event;
    }
    void KeysAndHandoff() {
        InputState state(7, true);
        state.Handle(KeyEvent(SDL_SCANCODE_W, true)); state.Handle(KeyEvent(SDL_SCANCODE_W, false));
        state.Handle(KeyEvent(SDL_SCANCODE_Q, true, false, 99));
        state.PublishEvents();
        InputFrame first; state.Exchange(first);
        const auto index = static_cast<std::size_t>(Key::W);
        Require(!first.held[index] && first.pressed[index] && first.Down(Key::W) && first.Down(Key::W),
            "Short press fact or non-consuming query changed");
        Require(!first.Down(Key::Q), "Foreign window input entered the frame");
        state.PublishEvents(); InputFrame next; state.Exchange(next);
        Require(!next.Down(Key::W) && first.Down(Key::W), "Handoff consumed twice or changed the owned old frame");
        state.Handle(KeyEvent(SDL_SCANCODE_W, true)); state.Handle(KeyEvent(SDL_SCANCODE_W, true, true));
        state.PublishEvents(); next.Clear(); state.Exchange(next);
        Require(next.held[index] && next.pressed[index], "Held or initial press missing");
        state.PublishEvents(); next.Clear(); state.Exchange(next);
        Require(next.held[index] && !next.pressed[index], "Repeat fabricated a press");
        bool rejected = false;
        try { state.Exchange(next); } catch (const std::logic_error&) { rejected = true; }
        Require(rejected, "Non-cleared destination was accepted");
    }
    void PendingAndFocus() {
        InputState state(7, true);
        state.Handle(Motion(1, 1)); // First movement establishes the baseline.
        state.Handle(Motion(2, -3));
        state.PublishEvents();
        InputFrame first; state.Exchange(first);
        Require(first.pointer_events.size() == 1 && first.pointer_events[0].mouse_dx == 2 &&
            first.pointer_events[0].mouse_dy == 3, "Ordered pointer projection changed");
        state.Handle(Motion(4, 5)); // Wait-style pending facts must survive exchange/reset of the caller frame.
        InputFrame empty; state.Exchange(empty);
        Require(empty.pointer_events.empty(), "Unpublished pending event escaped early");
        first.Clear(); state.PublishEvents(); state.Exchange(first);
        Require(first.pointer_events.size() == 1 && first.pointer_events[0].mouse_dx == 4,
            "Pending event was lost or applied twice");
        const auto old = first;
        state.Handle(KeyEvent(SDL_SCANCODE_ESCAPE, true));
        state.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        state.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        Platform::Detail::PhysicalInputState physical;
        physical.window_id = 7; physical.focused = true; physical.keys[static_cast<std::size_t>(Key::W)] = true;
        state.SynchronizePhysicalState(physical);
        state.PublishEvents(); first.Clear(); state.Exchange(first);
        Require(first.focus_lost && first.Down(Key::Escape) && first.held[static_cast<std::size_t>(Key::W)],
            "Same-poll focus loss/queued Escape/fresh held facts were lost");
        state.ResetInput();
        Require(first.focus_lost && old.pointer_events.size() == 1, "Reset mutated already owned frames");
        state.PublishEvents(); InputFrame restored; state.Exchange(restored);
        Require(!restored.Down(Key::Escape) && restored.Down(Key::W) && !restored.focus_lost,
            "Reset replayed a press or discarded physical held state");
        state.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        physical.focused = false; physical.keys.fill(true); state.SynchronizePhysicalState(physical);
        state.PublishEvents(); restored.Clear(); state.Exchange(restored);
        Require(!restored.Down(Key::Escape) && !restored.Down(Key::W), "Unfocused global held keys leaked");
    }
}
int main() {
    try { KeysAndHandoff(); PendingAndFocus(); std::cout << "Owned input facts, pending events and focus handoff passed\n"; return 0; }
    catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
