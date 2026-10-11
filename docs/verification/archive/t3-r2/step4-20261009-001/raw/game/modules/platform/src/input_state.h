#pragma once
#include <symocraft/platform/window.h>
#include <SDL3/SDL_events.h>
#include <array>
#include <optional>

namespace SymoCraft::Platform::Detail {
    inline constexpr std::array<SDL_Scancode, static_cast<std::size_t>(Key::Count)> ControlScancodes{
        SDL_SCANCODE_ESCAPE, SDL_SCANCODE_LSHIFT, SDL_SCANCODE_CAPSLOCK, SDL_SCANCODE_LCTRL,
        SDL_SCANCODE_W, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_A, SDL_SCANCODE_SPACE,
        SDL_SCANCODE_E, SDL_SCANCODE_Q};

    inline std::optional<Key> ProjectKey(SDL_Scancode scancode) {
        for (std::size_t index = 0; index < ControlScancodes.size(); ++index)
            if (ControlScancodes[index] == scancode) return static_cast<Key>(index);
        return std::nullopt;
    }

    struct PhysicalInputState {
        SDL_WindowID window_id{};
        bool focused{}, minimized{};
        std::array<bool, static_cast<std::size_t>(Key::Count)> keys{};
        bool left_button{}, right_button{};
    };

    class InputState {
    public:
        explicit InputState(SDL_WindowID window_id, bool benchmark, bool initially_focused)
            : window_id_(window_id), benchmark_(benchmark), focused_(initially_focused) {
            input_.pointer_events.reserve(64);
            pending_pointer_events_.reserve(64);
        }

        void Handle(const SDL_Event& event) {
            if (event.type == SDL_EVENT_QUIT) { close_ = true; return; }
            if (!BelongsToWindow(event)) return;
            switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED: close_ = true; break;
            case SDL_EVENT_WINDOW_FOCUS_LOST: {
                const bool escape_press = benchmark_ && !minimized_ &&
                                          pressed_[static_cast<std::size_t>(Key::Escape)];
                focused_ = false;
                ResetInput();
                pending_focus_loss_ = true;
                if (escape_press) pressed_[static_cast<std::size_t>(Key::Escape)] = true;
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED: focused_ = true; first_motion_ = true; break;
            case SDL_EVENT_WINDOW_MINIMIZED: {
                const bool focus_loss = pending_focus_loss_;
                minimized_ = true;
                ResetInput();
                pending_focus_loss_ = focus_loss;
                break;
            }
            case SDL_EVENT_WINDOW_RESTORED: minimized_ = false; first_motion_ = true; break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: HandleKey(event.key); break;
            case SDL_EVENT_MOUSE_MOTION:
                if (!benchmark_ && focused_ && !minimized_) {
                    if (!first_motion_)
                        pending_pointer_events_.push_back({event.motion.xrel, -event.motion.yrel, 0});
                    first_motion_ = false;
                }
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                if (!benchmark_ && focused_ && !minimized_) {
                    const double direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0 : 1.0;
                    pending_pointer_events_.push_back({0, 0, direction * event.wheel.y});
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (!benchmark_ && focused_ && !minimized_) {
                    const bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
                    if (event.button.button == SDL_BUTTON_LEFT) left_button_ = down;
                    else if (event.button.button == SDL_BUTTON_RIGHT) right_button_ = down;
                }
                break;
            default: break;
            }
        }

        void PublishEvents() {
            input_.pointer_events.clear();
            input_.pointer_events.swap(pending_pointer_events_);
            input_.focus_lost = pending_focus_loss_;
            pending_focus_loss_ = false;
        }

        void CaptureKeys() {
            for (std::size_t index = 0; index < down_.size(); ++index) {
                input_.keys[index] = down_[index] || pressed_[index];
                pressed_[index] = false;
            }
            input_.left_button = left_button_;
            input_.right_button = right_button_;
        }

        void SynchronizeWindowState(SDL_WindowID window_id, bool focused, bool minimized) {
            if (window_id != window_id_ || (focused == focused_ && minimized == minimized_)) return;
            const bool focus_loss = pending_focus_loss_ || (focused_ && !focused);
            const bool escape_press = benchmark_ && !minimized && pressed_[static_cast<std::size_t>(Key::Escape)];
            focused_ = focused;
            minimized_ = minimized;
            ClearHeldInput();
            ResetInput();
            pending_focus_loss_ = focus_loss;
            // Focus changes must not discard an already queued benchmark exit event.
            if (escape_press) pressed_[static_cast<std::size_t>(Key::Escape)] = true;
        }

        void SynchronizePhysicalState(const PhysicalInputState& physical) {
            if (physical.window_id != window_id_) return;
            if (physical.focused != focused_ || physical.minimized != minimized_ || !focused_ || minimized_) {
                ClearHeldInput();
                // Benchmark Escape events remain valid without sampling global unfocused keys.
                if (!benchmark_) pressed_.fill(false);
                return;
            }
            down_ = physical.keys;
            if (benchmark_)
                for (std::size_t index = 1; index < down_.size(); ++index) down_[index] = false;
            left_button_ = !benchmark_ && physical.left_button;
            right_button_ = !benchmark_ && physical.right_button;
        }

        void ResetInput() {
            // Match sticky-key reset: discard presses, but retain physically held controls.
            if (!focused_ || minimized_) ClearHeldInput();
            pressed_.fill(false);
            input_.keys.fill(false);
            input_.left_button = input_.right_button = input_.focus_lost = false;
            input_.pointer_events.clear();
            pending_pointer_events_.clear();
            pending_focus_loss_ = false;
            first_motion_ = true;
        }

        const InputSnapshot& Input() const { return input_; }
        bool ShouldClose() const { return close_; }
        void Close() { close_ = true; }

    private:
        void ClearHeldInput() { down_.fill(false); left_button_ = right_button_ = false; }

        bool BelongsToWindow(const SDL_Event& event) const {
            switch (event.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: return event.key.windowID == window_id_;
            case SDL_EVENT_MOUSE_MOTION: return event.motion.windowID == window_id_;
            case SDL_EVENT_MOUSE_WHEEL: return event.wheel.windowID == window_id_;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP: return event.button.windowID == window_id_;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_MINIMIZED:
            case SDL_EVENT_WINDOW_RESTORED: return event.window.windowID == window_id_;
            default: return false;
            }
        }

        void HandleKey(const SDL_KeyboardEvent& event) {
            const auto key = ProjectKey(event.scancode);
            if (!key || (benchmark_ && *key != Key::Escape)) return;
            const auto index = static_cast<std::size_t>(*key);
            if (event.type == SDL_EVENT_KEY_UP) down_[index] = false;
            else if (!event.repeat && !minimized_ && (focused_ || benchmark_)) {
                if (!down_[index]) pressed_[index] = true;
                down_[index] = true;
            }
        }

        SDL_WindowID window_id_{};
        bool benchmark_{}, focused_{}, minimized_{}, close_{};
        bool first_motion_{true}, pending_focus_loss_{}, left_button_{}, right_button_{};
        std::array<bool, static_cast<std::size_t>(Key::Count)> down_{};
        std::array<bool, static_cast<std::size_t>(Key::Count)> pressed_{};
        InputSnapshot input_;
        std::vector<PointerEvent> pending_pointer_events_;
    };
}
