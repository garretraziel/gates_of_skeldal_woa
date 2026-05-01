#include "touch_input.h"
#include <algorithm>

void TouchInput::set_config(int long_press_ms, int slop_radius_px, int tap_max_ms) {
    if (long_press_ms > 50) _long_press_ms = long_press_ms;
    if (slop_radius_px >= 0) _slop_radius = slop_radius_px;
    if (tap_max_ms > 50) _tap_max_ms = tap_max_ms;
}

TouchInput::FingerState *TouchInput::find(SDL_FingerID id) {
    for (auto &f : _fingers) if (f.id == id) return &f;
    return nullptr;
}

bool TouchInput::over_slop(const FingerState &f) const {
    int dx = f.current_x - f.start_x;
    int dy = f.current_y - f.start_y;
    return (dx*dx + dy*dy) > (_slop_radius * _slop_radius);
}

// Design notes
// ============
// PRESS is emitted at FINGERDOWN, RELEASE at FINGERUP. This mirrors how real
// mouse events flow (button-down comes ms before button-up, separated in time
// by the user's finger), so the game thread reliably observes both states
// instead of collapsing them in MS_EVENT.event_type bit-OR.
//
// Long-press = "convert in-flight left to right":
//   On timer fire we emit LeftRelease, then RightPress. On FINGERUP we emit
//   RightRelease. The game sees a clean left-down/left-up early followed by a
//   clean right-down/right-up — close enough to "I right-clicked at this spot."
//
// Two-finger tap = same idea: at second FINGERDOWN we close out the current
// left button and emit a Right press+release pair (the right release is paired
// with primary's FINGERUP via current_button=Right).

std::vector<TouchSynthEvent> TouchInput::handle_finger_down(SDL_FingerID id,
                                                            int src_x, int src_y,
                                                            uint32_t now_ms) {
    std::vector<TouchSynthEvent> out;

    if (auto *existing = find(id)) {
        existing->start_x = existing->current_x = src_x;
        existing->start_y = existing->current_y = src_y;
        existing->down_time_ms = now_ms;
        return out;
    }

    FingerState fs{ id, src_x, src_y, src_x, src_y, now_ms, false };
    _fingers.push_back(fs);

    // First finger becomes primary: emit Move + LeftPress immediately.
    if (_primary_id == -1) {
        _primary_id = id;
        _current_button = Button::Left;
        out.push_back({ TouchSynthEvent::Move,      src_x, src_y });
        out.push_back({ TouchSynthEvent::LeftPress, src_x, src_y });
        return out;
    }

    // Second finger arriving while primary holds Left: convert to right-click.
    // (Suppressed if primary is dragging — avoid misfire while scrolling.)
    auto *primary = find(_primary_id);
    if (primary && _current_button == Button::Left && !primary->dragging) {
        out.push_back({ TouchSynthEvent::LeftRelease, primary->current_x, primary->current_y });
        out.push_back({ TouchSynthEvent::RightPress,  primary->current_x, primary->current_y });
        _current_button = Button::Right;
    }
    return out;
}

std::vector<TouchSynthEvent> TouchInput::handle_finger_motion(SDL_FingerID id,
                                                              int src_x, int src_y,
                                                              uint32_t now_ms) {
    (void)now_ms;
    std::vector<TouchSynthEvent> out;
    auto *f = find(id);
    if (!f) return out;
    f->current_x = src_x;
    f->current_y = src_y;

    if (id != _primary_id) return out;

    // Mark drag once over slop (used to disable long-press / two-finger).
    if (!f->dragging && over_slop(*f)) {
        f->dragging = true;
    }

    // Always emit Move; this naturally implements drag-while-pressed (scrollbars,
    // save-list arrow auto-repeat) since the button stays down between events.
    out.push_back({ TouchSynthEvent::Move, src_x, src_y });
    return out;
}

std::vector<TouchSynthEvent> TouchInput::handle_finger_up(SDL_FingerID id,
                                                          int src_x, int src_y,
                                                          uint32_t now_ms) {
    (void)now_ms;
    std::vector<TouchSynthEvent> out;
    auto *f = find(id);
    if (!f) return out;

    bool was_primary = (id == _primary_id);
    f->current_x = src_x;
    f->current_y = src_y;

    if (was_primary) {
        // Final move to release point (in case finger drifted between motions).
        out.push_back({ TouchSynthEvent::Move, src_x, src_y });
        if (_current_button == Button::Left) {
            out.push_back({ TouchSynthEvent::LeftRelease, src_x, src_y });
        } else if (_current_button == Button::Right) {
            out.push_back({ TouchSynthEvent::RightRelease, src_x, src_y });
        }
        _current_button = Button::None;
    }

    // Remove this finger.
    _fingers.erase(std::remove_if(_fingers.begin(), _fingers.end(),
                                  [id](const FingerState &x){ return x.id == id; }),
                   _fingers.end());

    if (was_primary) {
        // If other fingers are still down, promote oldest as new primary,
        // but do NOT auto-press for them — they came in mid-gesture.
        if (!_fingers.empty()) {
            _primary_id = _fingers.front().id;
        } else {
            _primary_id = -1;
        }
    }
    return out;
}

std::vector<TouchSynthEvent> TouchInput::tick(uint32_t now_ms) {
    std::vector<TouchSynthEvent> out;
    if (_primary_id == -1) return out;
    if (_current_button != Button::Left) return out;

    auto *p = find(_primary_id);
    if (!p) return out;
    if (p->dragging) return out;
    if (over_slop(*p)) return out;

    uint32_t held = now_ms - p->down_time_ms;
    if (held >= (uint32_t)_long_press_ms) {
        // Convert in-flight Left to Right (emit Left-up, then Right-down).
        // Right-up will be emitted on FINGERUP via _current_button = Right.
        out.push_back({ TouchSynthEvent::LeftRelease, p->start_x, p->start_y });
        out.push_back({ TouchSynthEvent::RightPress,  p->start_x, p->start_y });
        _current_button = Button::Right;
    }
    return out;
}

int TouchInput::next_deadline_ms(uint32_t now_ms) const {
    if (_primary_id == -1) return -1;
    if (_current_button != Button::Left) return -1;
    for (const auto &f : _fingers) if (f.id == _primary_id) {
        if (f.dragging) return -1;
        uint32_t deadline = f.down_time_ms + (uint32_t)_long_press_ms;
        if (deadline <= now_ms) return 0;
        return (int)(deadline - now_ms);
    }
    return -1;
}

