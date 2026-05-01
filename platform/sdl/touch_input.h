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
/// Gestures recognised:
///   Tap          : down + up within slop radius and < tap_max_ms  -> Left PRESS+RELEASE
///   Long-press   : single finger held > long_press_ms within slop -> Right PRESS+RELEASE
///   Two-finger   : second finger down while primary still active  -> Right PRESS+RELEASE
///   Drag         : single finger moves > slop                     -> Move events (no click)
///
/// Press and release are delivered as SEPARATE events. Never combined into one
/// "click" event: MS_EVENT.event_type ORs bits until consumed by getMsEvent(),
/// so a same-frame press+release would lose the press.
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
    /// Fires deferred events such as long-press.
    std::vector<TouchSynthEvent> tick(uint32_t now_ms);

    /// Milliseconds until the next deadline (e.g., long-press timeout).
    /// Returns -1 if no deadline pending.
    int next_deadline_ms(uint32_t now_ms) const;

    /// True if any finger is currently down or recently lifted.
    bool is_active() const { return !_fingers.empty(); }

private:
    struct FingerState {
        SDL_FingerID id;
        int start_x, start_y;
        int current_x, current_y;
        uint32_t down_time_ms;
        bool dragging;          ///< exceeded slop radius
        bool consumed;          ///< this finger already produced a click/long-press
    };

    int _long_press_ms = 500;
    int _slop_radius   = 12;
    int _tap_max_ms    = 350;

    std::vector<FingerState> _fingers;
    SDL_FingerID _primary_id = -1;
    bool _long_press_fired = false;     ///< primary already fired long-press
    bool _two_finger_fired = false;     ///< two-finger right-click already fired this gesture

    FingerState *find(SDL_FingerID id);
    bool over_slop(const FingerState &f) const;
};
