#include "input_candidate.h"

#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace Candidate = SymoCraft::Experimental::Sdl3;
using SymoCraft::Key;

namespace
{
    constexpr SDL_WindowID WindowId = 7;
    unsigned checks{};

    void Require(bool condition, const char* message)
    {
        ++checks;
        if (!condition)
            throw std::runtime_error(message);
    }

    SDL_Event Keyboard(Uint32 type, SDL_Scancode scancode, SDL_WindowID window = WindowId, bool repeat = false)
    {
        SDL_Event event{};
        event.type = type;
        event.key.windowID = window;
        event.key.scancode = scancode;
        event.key.key = SDLK_F1;
        event.key.down = type == SDL_EVENT_KEY_DOWN;
        event.key.repeat = repeat;
        return event;
    }

    SDL_Event WindowEvent(Uint32 type, SDL_WindowID window = WindowId)
    {
        SDL_Event event{};
        event.type = type;
        event.window.windowID = window;
        return event;
    }

    SDL_Event Motion(float dx, float dy, SDL_WindowID window = WindowId)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_MOTION;
        event.motion.windowID = window;
        event.motion.xrel = dx;
        event.motion.yrel = dy;
        return event;
    }

    SDL_Event Wheel(float y, SDL_MouseWheelDirection direction = SDL_MOUSEWHEEL_NORMAL,
                    SDL_WindowID window = WindowId)
    {
        SDL_Event event{};
        event.type = SDL_EVENT_MOUSE_WHEEL;
        event.wheel.windowID = window;
        event.wheel.y = y;
        event.wheel.direction = direction;
        return event;
    }

    SDL_Event Button(Uint32 type, Uint8 button, SDL_WindowID window = WindowId)
    {
        SDL_Event event{};
        event.type = type;
        event.button.windowID = window;
        event.button.button = button;
        return event;
    }

    Candidate::PhysicalInputState Physical(bool focused = true, bool minimized = false,
                                            SDL_WindowID window = WindowId)
    {
        Candidate::PhysicalInputState state{};
        state.window_id = window;
        state.focused = focused;
        state.minimized = minimized;
        return state;
    }

    void MappingAndStickyKeys()
    {
        constexpr std::array<std::pair<SDL_Scancode, Key>, 11> expected{{
            {SDL_SCANCODE_ESCAPE, Key::Escape}, {SDL_SCANCODE_LSHIFT, Key::LeftShift},
            {SDL_SCANCODE_CAPSLOCK, Key::CapsLock}, {SDL_SCANCODE_LCTRL, Key::LeftControl},
            {SDL_SCANCODE_W, Key::W}, {SDL_SCANCODE_S, Key::S}, {SDL_SCANCODE_D, Key::D},
            {SDL_SCANCODE_A, Key::A}, {SDL_SCANCODE_SPACE, Key::Space},
            {SDL_SCANCODE_E, Key::E}, {SDL_SCANCODE_Q, Key::Q}}};
        Candidate::InputCandidate input(WindowId);
        for (const auto& [code, key] : expected)
        {
            Require(Candidate::ProjectKey(code) == key, "Project scancode mapping changed");
            input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, code));
            input.Handle(Keyboard(SDL_EVENT_KEY_UP, code));
        }
        Require(!Candidate::ProjectKey(SDL_SCANCODE_F1), "Unknown scancode became a project control");
        input.CaptureKeys();
        for (std::size_t index = 0; index < Candidate::ControlScancodes.size(); ++index)
            Require(input.Input().Down(static_cast<Key>(index)), "Short press was not latched");
        Require(input.Input().Down(Key::Space) && input.Input().Down(Key::Space), "Snapshot reads consumed a key");
        input.CaptureKeys();
        for (const bool down : input.Input().keys)
            Require(!down, "Released short press leaked to another capture");

        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        input.CaptureKeys();
        Require(input.Input().Down(Key::W), "Held key was lost on first capture");
        input.CaptureKeys();
        Require(input.Input().Down(Key::W), "Held key was lost on second capture");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, WindowId, true));
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_W));
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W), "Repeat manufactured a new sticky press");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, WindowId, true));
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W), "Orphan repeat reactivated a cleared key");
    }

    void WindowFilteringAndFocus()
    {
        Candidate::InputCandidate input(WindowId);
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, 99));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST, 99));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED, 99));
        input.Handle(Motion(9, 9, 99));
        input.Handle(Wheel(9, SDL_MOUSEWHEEL_NORMAL, 99));
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, 99));
        input.PublishEvents();
        input.CaptureKeys();
        Require(input.Focused() && !input.ShouldClose(), "Foreign-window control event was accepted");
        Require(!input.Input().Down(Key::W) && !input.Input().left_button && input.Input().pointer_events.empty(),
                "Foreign-window input was accepted");

        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        input.Handle(Wheel(1));
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE));
        input.Handle(Motion(20, 20));
        input.Handle(Wheel(2));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        input.PublishEvents();
        input.CaptureKeys();
        Require(input.Focused() && input.Input().focus_lost, "Same-pump regain hid a focus loss");
        Require(!input.Input().Down(Key::W) && !input.Input().Down(Key::Space) && !input.Input().left_button,
                "Focus loss kept held or background input");
        Require(input.Input().pointer_events.empty(), "Focus loss kept stale pointer events");
        input.PublishEvents();
        Require(!input.Input().focus_lost, "Focus-loss pulse repeated without another event");

        input.Handle(WindowEvent(SDL_EVENT_WINDOW_MINIMIZED));
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        input.CaptureKeys();
        Require(input.Minimized() && !input.Input().Down(Key::W), "Minimized input was accepted");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_RESTORED));
        Require(!input.Minimized(), "Restore did not remove minimized state");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED));
        Require(input.ShouldClose(), "Matching window close was ignored");
        Candidate::InputCandidate hidden(WindowId, false, false);
        hidden.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        hidden.CaptureKeys();
        Require(!hidden.Focused() && !hidden.Input().Down(Key::W), "Initial unfocused state accepted gameplay input");
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        hidden.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        hidden.CaptureKeys();
        Require(hidden.Focused() && hidden.Input().Down(Key::W), "Initial unfocused window did not recover input");
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_MINIMIZED));
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_RESTORED));
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        hidden.PublishEvents();
        Require(hidden.Input().focus_lost, "Minimize/restore discarded an earlier focus loss in the same pump");
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_MINIMIZED));
        hidden.Handle(WindowEvent(SDL_EVENT_WINDOW_RESTORED));
        hidden.PublishEvents();
        Require(!hidden.Input().focus_lost, "Minimize replayed a focus-loss pulse published by an earlier pump");
        Candidate::InputCandidate quit(WindowId);
        SDL_Event event{};
        event.type = SDL_EVENT_QUIT;
        quit.Handle(event);
        Require(quit.ShouldClose(), "Process quit event was ignored");
    }

    void PointerOrderAndReset()
    {
        Candidate::InputCandidate input(WindowId);
        input.Handle(Motion(1000, 1000));
        input.Handle(Motion(2, -3));
        input.Handle(Wheel(0.5F));
        input.Handle(Motion(-4, 5));
        input.Handle(Wheel(2, SDL_MOUSEWHEEL_FLIPPED));
        input.PublishEvents();
        const auto& events = input.Input().pointer_events;
        Require(events.size() == 4, "First motion suppression or event count changed");
        Require(events[0].mouse_dx == 2 && events[0].mouse_dy == 3 && events[0].scroll_y == 0,
                "Relative motion sign mapping changed");
        Require(events[1].scroll_y == 0.5 && events[2].mouse_dx == -4 && events[2].mouse_dy == -5,
                "Interleaved motion/wheel order was collapsed");
        Require(events[3].scroll_y == -2, "Flipped wheel direction was not normalized");
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT));
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT));
        input.CaptureKeys();
        Require(input.Input().left_button && input.Input().right_button, "Held mouse buttons were lost");
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT));
        input.CaptureKeys();
        Require(!input.Input().left_button && input.Input().right_button, "Mouse release affected the wrong button");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE));
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE));
        input.Handle(Wheel(4));
        input.ResetInput();
        Require(!input.Input().Down(Key::Space) && !input.Input().right_button,
                "Reset kept the previously published key/button snapshot");
        input.PublishEvents();
        input.CaptureKeys();
        Require(!input.Input().Down(Key::Space) && input.Input().right_button && input.Input().pointer_events.empty(),
                "Reset did not clear the short press or preserve a held mouse button");
        input.Handle(Motion(500, 500));
        input.Handle(Motion(1, 2));
        input.PublishEvents();
        Require(input.Input().pointer_events.size() == 1 && input.Input().pointer_events[0].mouse_dy == -2,
                "Reset did not restart the first-motion guard");
        input.PublishEvents();
        Require(input.Input().pointer_events.empty(), "Pointer sequence replayed on another frame");
    }

    void PhysicalStateAndReset()
    {
        Candidate::InputCandidate input(WindowId);
        auto held = Physical();
        held.keys.fill(true);
        held.left_button = held.right_button = true;
        Require(input.SynchronizePhysicalState(held), "Focused physical snapshot was rejected");
        input.CaptureKeys();
        for (const bool down : input.Input().keys)
            Require(down, "Physical project key was not captured");
        Require(input.Input().left_button && input.Input().right_button, "Physical mouse buttons were not captured");
        input.ResetInput();
        Require(!input.Input().left_button && !input.Input().right_button && !input.Input().Down(Key::W),
                "Reset did not clear the public snapshot immediately");
        input.CaptureKeys();
        for (const bool down : input.Input().keys)
            Require(down, "Active reset destroyed a known held key");
        Require(input.Input().left_button && input.Input().right_button, "Active reset destroyed known held buttons");

        Require(!input.SynchronizePhysicalState(Physical(true, false, 99)), "Foreign physical snapshot was accepted");
        input.CaptureKeys();
        Require(input.Input().Down(Key::W) && input.Input().left_button && input.Input().right_button,
                "Foreign physical snapshot changed this window's held state");
        Require(input.SynchronizePhysicalState(Physical()), "Released physical snapshot was rejected");
        input.CaptureKeys();
        for (const bool down : input.Input().keys)
            Require(!down, "Physical release failed to remove a held key");
        Require(!input.Input().left_button && !input.Input().right_button, "Physical release failed to remove held buttons");

        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE));
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE));
        input.Handle(Wheel(0.25F));
        Require(input.SynchronizePhysicalState(Physical()), "Post-drain short-press physical snapshot was rejected");
        input.PublishEvents();
        input.CaptureKeys();
        Require(input.Input().Down(Key::Space), "Final released physical state erased a short-press latch");
        Require(input.Input().pointer_events.size() == 1 && input.Input().pointer_events[0].scroll_y == 0.25,
                "Physical resynchronization changed pending pointer events");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::Space), "Physical resynchronization duplicated a short-press latch");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE));
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE));
        input.ResetInput();
        Require(input.SynchronizePhysicalState(Physical()), "Post-reset released snapshot was rejected");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::Space), "Active reset kept a released sticky press");

        held.keys.fill(false);
        held.keys[static_cast<std::size_t>(Key::W)] = true;
        Require(input.SynchronizePhysicalState(held), "Held snapshot before reset was rejected");
        input.ResetInput();
        Require(input.SynchronizePhysicalState(Physical()), "Fresh release after reset was rejected");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W) && !input.Input().left_button && !input.Input().right_button,
                "Fresh physical release after reset left stale held state");
        Require(input.SynchronizePhysicalState(held), "Held snapshot for duplicate test was rejected");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W));
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, WindowId, true));
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_W));
        Require(input.SynchronizePhysicalState(Physical()), "Released duplicate-key snapshot was rejected");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W), "Duplicate or repeated down event manufactured a sticky press");
        input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, WindowId, true));
        Require(input.SynchronizePhysicalState(Physical()), "Orphan-repeat physical snapshot was rejected");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W), "Orphan repeat manufactured a press after physical release");
    }

    void PhysicalFocusAndRestore()
    {
        Candidate::InputCandidate input(WindowId, false, false);
        auto held = Physical();
        held.keys[static_cast<std::size_t>(Key::W)] = true;
        held.left_button = held.right_button = true;
        Require(!input.SynchronizePhysicalState(held), "Initially unfocused window adopted global active input");
        Require(!input.SynchronizePhysicalState(Physical(false)), "Inactive physical snapshot was accepted");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W) && !input.Input().left_button && !input.Input().right_button,
                "Initially unfocused physical state leaked gameplay input");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        Require(input.SynchronizePhysicalState(held), "Focus regain could not reacquire physically held input");
        input.CaptureKeys();
        Require(input.Input().Down(Key::W) && input.Input().left_button && input.Input().right_button,
                "Held input was not reacquired after focus regain");

        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        Require(!input.SynchronizePhysicalState(held), "Focus-lost window adopted a stale active sample");
        input.PublishEvents();
        input.CaptureKeys();
        Require(input.Input().focus_lost && !input.Input().Down(Key::W) && !input.Input().left_button,
                "Focus loss retained physical held input or lost its pulse");
        input.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_W, 99));
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT, 99));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        Require(input.SynchronizePhysicalState(Physical()), "Outside release could not be sampled after regain");
        input.ResetInput();
        Require(input.SynchronizePhysicalState(Physical()), "Resume reset rejected fresh released physical state");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W) && !input.Input().left_button && !input.Input().right_button,
                "Release outside the window left a stuck key or mouse button");

        Require(input.SynchronizePhysicalState(held), "Pre-minimize held snapshot was rejected");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_MINIMIZED));
        input.CaptureKeys();
        Require(input.Focused() && input.Minimized() && !input.Input().Down(Key::W) && !input.Input().left_button,
                "Minimize failed to invalidate held state independently of focus");
        auto minimized = held;
        minimized.minimized = true;
        Require(!input.SynchronizePhysicalState(minimized), "Minimized window adopted global held input");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_RESTORED));
        Require(input.SynchronizePhysicalState(held), "Focused restore failed to reacquire physically held input");
        input.ResetInput();
        input.CaptureKeys();
        Require(input.Input().Down(Key::W) && input.Input().left_button && input.Input().right_button,
                "Restore reset destroyed freshly reacquired held input");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_MINIMIZED));
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_RESTORED));
        Require(!input.SynchronizePhysicalState(held), "Restore without focus adopted physical input");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::W), "Restore without focus revived an old held key");
        input.Handle(WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED));
        Require(input.SynchronizePhysicalState(held), "Focused restore rejected current held input");
        input.PublishEvents();
        Require(input.Input().focus_lost, "Physical reacquisition consumed the pending focus-loss pulse");
        input.ResetInput();
        Require(input.SynchronizePhysicalState(held), "Same-pump resume reset rejected current held input");
        input.CaptureKeys();
        Require(input.Input().Down(Key::W) && input.Input().left_button && !input.Input().focus_lost,
                "Same-pump app reset kept the old pulse or destroyed current physical input");
        input.PublishEvents();
        Require(!input.Input().focus_lost, "Physical focus recovery replayed an old focus pulse");

        Require(!input.SynchronizePhysicalState(Physical(false)), "Contradictory physical focus flags were accepted");
        input.CaptureKeys();
        Require(input.Focused() && !input.Input().Down(Key::W) && !input.Input().left_button,
                "Contradictory focus sample changed event facts or kept stale held state");
        auto contradictory = held;
        contradictory.minimized = true;
        Require(!input.SynchronizePhysicalState(contradictory), "Contradictory physical minimized flags were accepted");
        input.CaptureKeys();
        Require(!input.Minimized() && !input.Input().Down(Key::W),
                "Contradictory minimized sample changed event facts or adopted keys");
    }

    void BenchmarkControls()
    {
        Candidate::InputCandidate input(WindowId, true);
        for (const auto code : Candidate::ControlScancodes)
            input.Handle(Keyboard(SDL_EVENT_KEY_DOWN, code));
        input.Handle(Motion(1, 2));
        input.Handle(Wheel(3));
        input.Handle(Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT));
        input.PublishEvents();
        input.CaptureKeys();
        Require(input.Input().Down(Key::Escape), "Benchmark exit key was lost");
        for (std::size_t index = 1; index < Candidate::ControlScancodes.size(); ++index)
            Require(!input.Input().Down(static_cast<Key>(index)), "Benchmark accepted a gameplay key");
        Require(input.Input().pointer_events.empty() && !input.Input().left_button,
                "Benchmark accepted gameplay pointer input");
        auto physical = Physical();
        physical.keys.fill(true);
        physical.left_button = physical.right_button = true;
        input.ResetInput();
        Require(input.SynchronizePhysicalState(physical), "Benchmark focused physical snapshot was rejected");
        input.CaptureKeys();
        Require(input.Input().Down(Key::Escape), "Benchmark physical exit key was lost");
        for (std::size_t index = 1; index < Candidate::ControlScancodes.size(); ++index)
            Require(!input.Input().Down(static_cast<Key>(index)), "Benchmark adopted a physical gameplay key");
        Require(!input.Input().left_button && !input.Input().right_button,
                "Benchmark adopted physical gameplay mouse buttons");
        Candidate::InputCandidate unfocused(WindowId, true, false);
        unfocused.Handle(Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE));
        unfocused.Handle(Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE));
        Require(!unfocused.SynchronizePhysicalState(Physical(false)), "Unfocused benchmark adopted global physical state");
        unfocused.CaptureKeys();
        Require(unfocused.Input().Down(Key::Escape), "Physical rejection erased a matching benchmark exit event");
        unfocused.CaptureKeys();
        Require(!unfocused.Input().Down(Key::Escape), "Unfocused benchmark exit latch was consumed more than once");
    }

    void WaitConsumesFirstEvent()
    {
        Candidate::InputCandidate input(WindowId);
        std::vector<SDL_Event> rest{
            Keyboard(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE), Wheel(1)};
        std::size_t index{};
        const auto wait = [](SDL_Event& event)
        {
            event = Keyboard(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE);
            return true;
        };
        const auto poll = [&](SDL_Event& event)
        {
            if (index == rest.size())
                return false;
            event = rest[index++];
            return true;
        };
        Require(input.WaitAndDrain(wait, poll), "Wait wake was not reported");
        const auto empty = [](SDL_Event&) { return false; };
        input.PollAndPublish(empty);
        Require(input.SynchronizePhysicalState(Physical()), "Wait short-press physical state was rejected");
        input.CaptureKeys();
        Require(input.Input().Down(Key::Space) && input.Input().pointer_events.size() == 1,
                "Wait return event or drained events were lost");
        input.CaptureKeys();
        Require(!input.Input().Down(Key::Space), "Wait key latch was consumed more than once");

        input.WaitAndDrain([](SDL_Event& event)
        {
            event = WindowEvent(SDL_EVENT_WINDOW_FOCUS_LOST);
            return true;
        }, empty);
        input.PollAndPublish(empty);
        Require(input.Input().focus_lost, "Focus loss returned by Wait vanished before next poll");
        Require(!input.WaitAndDrain(empty, empty), "Wait timeout manufactured an event");

        Candidate::InputCandidate closing(WindowId);
        closing.WaitAndDrain([](SDL_Event& event)
        {
            event = WindowEvent(SDL_EVENT_WINDOW_CLOSE_REQUESTED);
            return true;
        }, empty);
        Require(closing.ShouldClose(), "Window close returned by Wait was lost");

        Candidate::InputCandidate ordered(WindowId);
        ordered.Handle(Motion(0, 0));
        const std::vector<SDL_Event> pointers{Wheel(0.5F), Motion(-2, 3)};
        std::size_t pointer_index{};
        ordered.WaitAndDrain([](SDL_Event& event)
        {
            event = Motion(1, -1);
            return true;
        }, [&](SDL_Event& event)
        {
            if (pointer_index == pointers.size())
                return false;
            event = pointers[pointer_index++];
            return true;
        });
        ordered.PollAndPublish(empty);
        const auto& pointer_events = ordered.Input().pointer_events;
        Require(pointer_events.size() == 3 && pointer_events[0].mouse_dx == 1 &&
                pointer_events[1].scroll_y == 0.5 && pointer_events[2].mouse_dy == -3,
                "Wait event and polled pointer events lost their shared order");

        Candidate::InputCandidate waking(WindowId, false, false);
        waking.WaitAndDrain([](SDL_Event& event)
        {
            event = WindowEvent(SDL_EVENT_WINDOW_FOCUS_GAINED);
            return true;
        }, empty);
        auto held = Physical();
        held.keys[static_cast<std::size_t>(Key::Space)] = true;
        waking.PollAndPublish(empty);
        Require(waking.Focused() && waking.SynchronizePhysicalState(held),
                "Wait focus-gain event did not enable fresh physical resynchronization");
        waking.ResetInput();
        Require(waking.SynchronizePhysicalState(held), "Resume reset rejected a physically held wait key");
        waking.CaptureKeys();
        Require(waking.Input().Down(Key::Space), "Resume reset lost a physically held key after wait");
        Require(waking.SynchronizePhysicalState(Physical()), "Wait-restored physical release was rejected");
        waking.CaptureKeys();
        Require(!waking.Input().Down(Key::Space), "Wait-restored physical release left a stuck key");
    }
}

int main()
{
    try
    {
        MappingAndStickyKeys();
        WindowFilteringAndFocus();
        PointerOrderAndReset();
        PhysicalStateAndReset();
        PhysicalFocusAndRestore();
        BenchmarkControls();
        WaitConsumesFirstEvent();
        std::cout << "SDL3 synthetic input candidate: " << checks << " checks passed.\n"
                  << "No SDL runtime, production adapter, physical layout, or pointer feel was validated.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "SDL3 input candidate failed: " << error.what() << '\n';
        return 1;
    }
}
