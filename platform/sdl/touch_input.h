#pragma once

#include <SDL.h>
#include <vector>
#include <cstdint>

/// Synthesized touch-derived event that maps to a mouse action.
/// SDLContext applies these to its MS_EVENT struct exactly like real mouse events.
struct TouchSynthEvent {
    enum Kind {
        Move,           ///< mouse-motion (no button change)
        LeftPress,
        LeftRelease,
        RightPress,
        RightRelease
    };
    Kind kind;
    int x;              ///< source-coordinate x (0..639), already clamped
    int y;              ///< source-coordinate y (0..479), already clamped
};

/// Per-touch gesture state machine.
///
/// Consumes SDL_TouchFingerEvent + a timer "tick" and emits synthesized mouse
/// events. The caller must:
///   1. Convert SDL_TouchFingerEvent normalized coords to SOURCE coords (640x480),
///      clamped to [0,639]/[0,479], BEFORE calling handle_finger_*.
///   2. Call tick(now_ms) periodically (or arrange a wakeup at next_deadline_ms).
///   3. Apply each returned TouchSynthEvent to MS_EVENT.
///
/// Gesture model (simplified for "press at down, release at up"):
///   FINGERDOWN of primary -> Move + LeftPress emitted immediately
///   FINGERMOTION of primary -> Move (drag-while-pressed works naturally)
///   FINGERUP of primary -> Right/Left Release matching current button
///   Long-press timer fires (still in slop, no drag):
///       LeftRelease + RightPress emitted; release will pair on FINGERUP
///   Two-finger arrives while primary holds Left:
///       LeftRelease + RightPress for primary; primary stays "Right" until UP
///
/// Press and release are NEVER emitted in the same call. They are separated by
/// real time (finger movement, timer fire, or second finger), so the game
/// thread reliably observes both states instead of collapsing them in
/// MS_EVENT.event_type bit-OR.
class TouchInput {
public:
    void set_config(int long_press_ms, int slop_radius_px, int tap_max_ms = 350);

    /// Returns synthesized mouse events to apply to MS_EVENT.
    /// `src_x`/`src_y` must already be in 0..639 / 0..479 (clamped).
    std::vector<TouchSynthEvent> handle_finger_down(SDL_FingerID id,
                                                    int src_x, int src_y,
                                                    uint32_t now_ms);
    std::vector<TouchSynthEvent> handle_finger_up(SDL_FingerID id,
                                                  int src_x, int src_y,
                                                  uint32_t now_ms);
    std::vector<TouchSynthEvent> handle_finger_motion(SDL_FingerID id,
                                                      int src_x, int src_y,
                                                      uint32_t now_ms);

    /// Called periodically (typically before SDL_WaitEventTimeout).
    /// Fires deferred events such as long-press conversion.
    std::vector<TouchSynthEvent> tick(uint32_t now_ms);

    /// Milliseconds until the next deadline (e.g., long-press timeout).
    /// Returns -1 if no deadline pending.
    int next_deadline_ms(uint32_t now_ms) const;

    /// True if any finger is currently down.
    bool is_active() const { return !_fingers.empty(); }

private:
    enum class Button { None, Left, Right };

    struct FingerState {
        SDL_FingerID id;
        int start_x, start_y;
        int current_x, current_y;
        uint32_t down_time_ms;
        bool dragging;          ///< exceeded slop radius
    };

    int _long_press_ms = 500;
    int _slop_radius   = 12;
    int _tap_max_ms    = 350;

    std::vector<FingerState> _fingers;
    SDL_FingerID _primary_id = -1;
    Button _current_button = Button::None;

    FingerState *find(SDL_FingerID id);
    bool over_slop(const FingerState &f) const;
};
