#include <input_state.h>
#include "input_test_events.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
    namespace Detail = SymoCraft::Platform::Detail;
    using SymoCraft::Key;
    constexpr SDL_WindowID Id = 7;
    unsigned checks{};

    void Check(bool condition, const char* diagnostic) {
        ++checks;
        if (!condition) throw std::runtime_error(diagnostic);
    }
    Detail::PhysicalInputState Physical(bool focused = true, bool minimized = false, SDL_WindowID id = Id) {
        Detail::PhysicalInputState result{};
        result.window_id = id;
        result.focused = focused;
        result.minimized = minimized;
        return result;
    }
    void Released(const SymoCraft::InputSnapshot& input) {
        for (bool key : input.keys) Check(!key, "An inactive key remained held");
        Check(!input.left_button && !input.right_button, "An inactive mouse button remained held");
    }
    void MappingsAndLatches() {
        constexpr std::array<std::pair<SDL_Scancode, Key>, 11> expected{{
            {SDL_SCANCODE_ESCAPE, Key::Escape}, {SDL_SCANCODE_LSHIFT, Key::LeftShift},
            {SDL_SCANCODE_CAPSLOCK, Key::CapsLock}, {SDL_SCANCODE_LCTRL, Key::LeftControl},
            {SDL_SCANCODE_W, Key::W}, {SDL_SCANCODE_S, Key::S}, {SDL_SCANCODE_D, Key::D},
            {SDL_SCANCODE_A, Key::A}, {SDL_SCANCODE_SPACE, Key::Space},
            {SDL_SCANCODE_E, Key::E}, {SDL_SCANCODE_Q, Key::Q}}};
        Detail::InputState state(Id, false, true);
        for (const auto& [code, key] : expected) {
            Check(Detail::ProjectKey(code) == key, "Physical control mapping changed");
            state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, code, Id));
            state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, code, Id));
        }
        Check(!Detail::ProjectKey(SDL_SCANCODE_F1), "Unknown scancode became a project control");
        state.SynchronizePhysicalState(Physical());
        state.CaptureKeys();
        for (bool key : state.Input().keys) Check(key, "Same-drain short press was lost during released resynchronization");
        Check(state.Input().Down(Key::W) && state.Input().Down(Key::W), "Snapshot reads consumed a latch");
        state.CaptureKeys();
        Released(state.Input());
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, Id));
        state.CaptureKeys();
        Check(state.Input().Down(Key::W), "Initial W press was lost");
        state.CaptureKeys();
        Check(state.Input().Down(Key::W), "Held W was lost");
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, Id, true));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_W, Id));
        state.CaptureKeys();
        Check(!state.Input().Down(Key::W), "Repeat manufactured a new press");
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, Id, true));
        state.CaptureKeys();
        Check(!state.Input().Down(Key::W), "Orphan repeat reactivated a key");
    }
    void FilteringAndPointers() {
        Detail::InputState state(Id, false, true);
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, 99));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, 99));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_MINIMIZED, 99));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_CLOSE_REQUESTED, 99));
        state.Handle(InputTest::Motion(9, 9, 99));
        state.Handle(InputTest::Wheel(9, 99));
        state.Handle(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, 99));
        state.PublishEvents();
        state.CaptureKeys();
        Released(state.Input());
        Check(!state.ShouldClose() && !state.Input().focus_lost && state.Input().pointer_events.empty(),
              "Foreign window events were accepted");
        state.Handle(InputTest::Motion(1000, 1000, Id));
        state.Handle(InputTest::Motion(2.5F, -3.25F, Id));
        state.Handle(InputTest::Wheel(0.5F, Id));
        state.Handle(InputTest::Motion(-4, 5, Id));
        state.Handle(InputTest::Wheel(2, Id, SDL_MOUSEWHEEL_FLIPPED));
        state.PublishEvents();
        const auto& pointer = state.Input().pointer_events;
        Check(pointer.size() == 4, "First motion suppression or pointer count changed");
        Check(pointer[0].mouse_dx == 2.5 && pointer[0].mouse_dy == 3.25, "Relative motion sign/precision changed");
        Check(pointer[1].scroll_y == 0.5 && pointer[2].mouse_dx == -4 && pointer[2].mouse_dy == -5,
              "Interleaved motion/wheel order changed");
        Check(pointer[3].scroll_y == -2, "Flipped wheel direction changed");
        state.PublishEvents();
        Check(state.Input().pointer_events.empty(), "Pointer sequence replayed next frame");
        state.Handle(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, Id));
        state.Handle(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_RIGHT, Id));
        state.CaptureKeys();
        Check(state.Input().left_button && state.Input().right_button, "Held button event state was lost");
        state.Handle(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_UP, SDL_BUTTON_LEFT, Id));
        state.CaptureKeys();
        Check(!state.Input().left_button && state.Input().right_button, "Left release affected the right button");
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, Id));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, Id));
        state.Handle(InputTest::Wheel(4, Id));
        state.ResetInput();
        Released(state.Input());
        state.PublishEvents();
        state.CaptureKeys();
        Check(!state.Input().Down(Key::Space) && state.Input().right_button && state.Input().pointer_events.empty(),
              "Reset kept a stale press/pointer or destroyed known held input");
        state.Handle(InputTest::Motion(500, 500, Id));
        state.Handle(InputTest::Motion(1, 2, Id));
        state.PublishEvents();
        Check(state.Input().pointer_events.size() == 1 && state.Input().pointer_events[0].mouse_dy == -2,
              "Reset did not restart the first-motion guard");
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_CLOSE_REQUESTED, Id));
        Check(state.ShouldClose(), "Matching close event was lost");
        Detail::InputState quitting(Id, false, true);
        quitting.Handle(InputTest::Window(SDL_EVENT_QUIT, 99));
        Check(quitting.ShouldClose(), "Global quit was incorrectly window-filtered");
        quitting.ResetInput();
        Check(quitting.ShouldClose(), "Reset cleared the close request");
    }
    void PhysicalAndFocus() {
        Detail::InputState state(Id, false, true);
        auto held = Physical();
        held.keys.fill(true);
        held.left_button = held.right_button = true;
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        for (bool key : state.Input().keys) Check(key, "A physical project key was not captured");
        Check(state.Input().left_button && state.Input().right_button, "Physical buttons were not captured");
        state.ResetInput();
        Released(state.Input());
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        for (bool key : state.Input().keys) Check(key, "Active reset lost a still-held physical key");
        Check(state.Input().left_button && state.Input().right_button, "Active reset lost physically held buttons");
        state.SynchronizePhysicalState(Physical(true, false, 99));
        state.CaptureKeys();
        Check(state.Input().Down(Key::W) && state.Input().left_button, "Foreign physical state changed held controls");
        state.SynchronizePhysicalState(Physical());
        state.CaptureKeys();
        Released(state.Input());
        state.SynchronizePhysicalState(held);
        state.Handle(InputTest::Wheel(4, Id));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, Id));
        state.Handle(InputTest::Wheel(4, Id));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_MINIMIZED, Id));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_RESTORED, Id));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_GAINED, Id));
        state.SynchronizePhysicalState(Physical());
        state.PublishEvents();
        state.CaptureKeys();
        Released(state.Input());
        Check(state.Input().focus_lost && state.Input().pointer_events.empty(), "Same-drain restore hid focus loss or retained pointers");
        state.PublishEvents();
        Check(!state.Input().focus_lost, "Focus-loss pulse was published twice");
        state.Handle(InputTest::Motion(50, 50, Id));
        state.Handle(InputTest::Motion(1, 2, Id));
        state.PublishEvents();
        Check(state.Input().pointer_events.size() == 1, "Restore did not restart the first-motion guard");
        state.SynchronizePhysicalState(Physical(false));
        state.CaptureKeys();
        Released(state.Input());
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_MINIMIZED, Id));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, Id));
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        Released(state.Input());
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_RESTORED, Id));
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        Check(state.Input().Down(Key::Space), "Restored focused state could not resynchronize fresh held input");
        Detail::InputState hidden(Id, false, false);
        hidden.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_W, Id));
        hidden.SynchronizePhysicalState(held);
        hidden.CaptureKeys();
        Released(hidden.Input());
        hidden.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_GAINED, Id));
        hidden.SynchronizePhysicalState(held);
        hidden.CaptureKeys();
        Check(hidden.Input().Down(Key::W), "Initially unfocused input did not recover");
    }
    void BenchmarkIsolation() {
        for (bool focused : {false, true}) {
            Detail::InputState state(Id, true, focused);
            for (SDL_Scancode code : Detail::ControlScancodes) {
                state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, code, Id));
                state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, code, Id));
            }
            state.Handle(InputTest::Motion(1, 2, Id));
            state.Handle(InputTest::Wheel(0.25F, Id));
            state.Handle(InputTest::Button(SDL_EVENT_MOUSE_BUTTON_DOWN, SDL_BUTTON_LEFT, Id));
            auto physical = Physical(focused);
            physical.keys.fill(true);
            physical.left_button = physical.right_button = true;
            state.SynchronizePhysicalState(physical);
            state.PublishEvents();
            state.CaptureKeys();
            Check(state.Input().Down(Key::Escape), "Benchmark Escape latch was erased by physical resynchronization");
            for (std::size_t index = 1; index < state.Input().keys.size(); ++index)
                Check(!state.Input().keys[index], "Benchmark adopted a gameplay key");
            Check(!state.Input().left_button && !state.Input().right_button && state.Input().pointer_events.empty(),
                  "Benchmark adopted buttons or pointer events");
            state.SynchronizePhysicalState(Physical(focused));
            state.CaptureKeys();
            Check(!state.Input().Down(Key::Escape), "Benchmark short press repeated across captures");
            state.Handle(InputTest::Window(SDL_EVENT_WINDOW_MINIMIZED, Id));
            state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
            state.CaptureKeys();
            Check(!state.Input().Down(Key::Escape), "Minimized benchmark accepted Escape");
        }
    }
    void AuthoritativeWindowFacts() {
        Detail::InputState state(Id, false, true);
        auto held = Physical();
        held.keys[static_cast<std::size_t>(Key::W)] = true;
        held.left_button = held.right_button = true;
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        state.Handle(InputTest::Motion(0, 0, Id));
        state.Handle(InputTest::Motion(2, 3, Id));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_SPACE, Id));
        state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_SPACE, Id));
        state.SynchronizeWindowState(99, false, true);
        state.PublishEvents();
        state.CaptureKeys();
        Check(state.Input().Down(Key::W) && state.Input().Down(Key::Space) && state.Input().left_button &&
              state.Input().pointer_events.size() == 1 && !state.Input().focus_lost,
              "Foreign authoritative facts changed input");
        state.SynchronizeWindowState(Id, false, true);
        state.PublishEvents();
        state.CaptureKeys();
        Released(state.Input());
        Check(state.Input().focus_lost && state.Input().pointer_events.empty(),
              "Missing focus/minimize events were not repaired from authoritative facts");
        state.PublishEvents();
        Check(!state.Input().focus_lost, "Facts-derived focus loss was published twice");
        state.SynchronizeWindowState(Id, true, false);
        state.SynchronizePhysicalState(held);
        state.CaptureKeys();
        Check(state.Input().Down(Key::W) && state.Input().left_button && state.Input().right_button,
              "Missing regain/restore left the reducer permanently inactive");
        state.Handle(InputTest::Motion(500, 500, Id));
        state.Handle(InputTest::Motion(1, 2, Id));
        state.PublishEvents();
        Check(state.Input().pointer_events.size() == 1 && state.Input().pointer_events[0].mouse_dy == -2,
              "Facts-based recovery did not reset the first-motion baseline");
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
        state.Handle(InputTest::Window(SDL_EVENT_WINDOW_MINIMIZED, Id));
        state.SynchronizeWindowState(Id, true, false);
        state.PublishEvents();
        state.SynchronizePhysicalState(Physical());
        state.CaptureKeys();
        Released(state.Input());
        Check(state.Input().focus_lost, "Correcting delayed focus/minimize events erased an unpublished focus-loss pulse");
        state.PublishEvents();
        Check(!state.Input().focus_lost, "Corrected delayed focus loss was published twice");
        state.Handle(InputTest::Wheel(0.5F, Id));
        state.SynchronizeWindowState(Id, true, false);
        state.PublishEvents();
        Check(state.Input().pointer_events.size() == 1, "Unchanged authoritative facts destroyed valid pending input");
        for (bool initially_focused : {false, true}) {
            Detail::InputState benchmark(Id, true, initially_focused);
            benchmark.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
            benchmark.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
            benchmark.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE, Id));
            benchmark.SynchronizeWindowState(Id, true, false);
            benchmark.SynchronizePhysicalState(Physical());
            benchmark.PublishEvents();
            benchmark.CaptureKeys();
            Check(benchmark.Input().Down(Key::Escape), "Authoritative benchmark regain erased the Escape exit latch");
            benchmark.CaptureKeys();
            Check(!benchmark.Input().Down(Key::Escape), "Regain-preserved benchmark Escape latch repeated");
            benchmark.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
            benchmark.SynchronizeWindowState(Id, false, true);
            benchmark.CaptureKeys();
            Check(!benchmark.Input().Down(Key::Escape), "Authoritative minimize retained a stale benchmark Escape press");
        }
    }
    void BenchmarkCancelAcrossFocusLoss() {
        for (bool loss_first : {false, true}) {
            Detail::InputState state(Id, true, true);
            if (loss_first) state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
            state.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
            state.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE, Id));
            if (!loss_first) state.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
            state.SynchronizeWindowState(Id, false, false);
            state.SynchronizePhysicalState(Physical(false));
            state.PublishEvents();
            state.CaptureKeys();
            Check(state.Input().Down(Key::Escape) && state.Input().focus_lost,
                  "Focus loss erased the matching benchmark cancellation latch");
            state.CaptureKeys();
            Check(!state.Input().Down(Key::Escape), "Focus-loss-preserved cancellation latch was consumed twice");
        }
        Detail::InputState facts(Id, true, true);
        facts.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
        facts.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE, Id));
        facts.SynchronizeWindowState(Id, false, false);
        facts.SynchronizePhysicalState(Physical(false));
        facts.PublishEvents();
        facts.CaptureKeys();
        Check(facts.Input().Down(Key::Escape) && facts.Input().focus_lost,
              "Authoritative focus loss without an event erased benchmark cancellation");
        facts.CaptureKeys();
        facts.Handle(InputTest::Window(SDL_EVENT_WINDOW_FOCUS_LOST, Id));
        facts.CaptureKeys();
        Check(!facts.Input().Down(Key::Escape), "A delayed focus loss replayed an already consumed benchmark cancellation");
        Detail::InputState ordinary(Id, false, true);
        ordinary.Handle(InputTest::Key(SDL_EVENT_KEY_DOWN, SDL_SCANCODE_ESCAPE, Id));
        ordinary.Handle(InputTest::Key(SDL_EVENT_KEY_UP, SDL_SCANCODE_ESCAPE, Id));
        ordinary.SynchronizeWindowState(Id, false, false);
        ordinary.SynchronizePhysicalState(Physical(false));
        ordinary.CaptureKeys();
        Released(ordinary.Input());
    }
}

int main() {
    try {
        MappingsAndLatches();
        FilteringAndPointers();
        PhysicalAndFocus();
        BenchmarkIsolation();
        AuthoritativeWindowFacts();
        BenchmarkCancelAcrossFocusLoss();
        std::cout << "{\"status\":\"pass\",\"checks\":" << checks
                  << ",\"scope\":\"actual production private InputState; synthetic data only; no SDL runtime or hardware\"}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Production InputState pure regression failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
