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

std::vector<TouchSynthEvent> TouchInput::handle_finger_down(SDL_FingerID id,
                                                            int src_x, int src_y,
                                                            uint32_t now_ms) {
    std::vector<TouchSynthEvent> out;

    // Already tracking this id? Defensive: replace state.
    if (auto *existing = find(id)) {
        existing->start_x = existing->current_x = src_x;
        existing->start_y = existing->current_y = src_y;
        existing->down_time_ms = now_ms;
        existing->dragging = false;
        existing->consumed = false;
        return out;
    }

    FingerState fs{ id, src_x, src_y, src_x, src_y, now_ms, false, false };
    _fingers.push_back(fs);

    // First finger becomes primary; emit move-to so subsequent UI sees focus there.
    if (_primary_id == -1) {
        _primary_id = id;
        _long_press_fired = false;
        _two_finger_fired = false;
        out.push_back({ TouchSynthEvent::Move, src_x, src_y });
        return out;
    }

    // Second finger arriving while primary is active: two-finger right-click.
    // Suppressed if primary is already dragging (prevents misfire while scrolling).
    auto *primary = find(_primary_id);
    if (primary && !primary->dragging && !_two_finger_fired && !_long_press_fired) {
        _two_finger_fired = true;
        if (primary) primary->consumed = true;
        out.push_back({ TouchSynthEvent::Move,         primary->current_x, primary->current_y });
        out.push_back({ TouchSynthEvent::RightPress,   primary->current_x, primary->current_y });
        out.push_back({ TouchSynthEvent::RightRelease, primary->current_x, primary->current_y });
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

    // Promote to drag once over slop; emit motion events while dragging.
    if (!f->dragging && over_slop(*f)) {
        f->dragging = true;
    }
    if (f->dragging) {
        out.push_back({ TouchSynthEvent::Move, src_x, src_y });
    }
    return out;
}

std::vector<TouchSynthEvent> TouchInput::handle_finger_up(SDL_FingerID id,
                                                          int src_x, int src_y,
                                                          uint32_t now_ms) {
    std::vector<TouchSynthEvent> out;
    auto *f = find(id);
    if (!f) return out;

    bool was_primary = (id == _primary_id);
    bool fired_already = f->consumed || (was_primary && (_long_press_fired || _two_finger_fired));
    bool is_drag = was_primary && f->dragging;
    uint32_t held_ms = now_ms - f->down_time_ms;

    // Update position one last time.
    f->current_x = src_x;
    f->current_y = src_y;

    if (was_primary && !fired_already && !is_drag && held_ms <= (uint32_t)_tap_max_ms) {
        // It's a tap: emit a left press+release pair at the down position.
        // (Use start position so tiny finger movements don't shift the click.)
        out.push_back({ TouchSynthEvent::Move,        f->start_x, f->start_y });
        out.push_back({ TouchSynthEvent::LeftPress,   f->start_x, f->start_y });
        out.push_back({ TouchSynthEvent::LeftRelease, f->start_x, f->start_y });
    }
    // If drag: no click events; the Move events were already emitted during motion.
    // If long-press already fired: no click on release.

    // Remove this finger.
    _fingers.erase(std::remove_if(_fingers.begin(), _fingers.end(),
                                  [id](const FingerState &x){ return x.id == id; }),
                   _fingers.end());

    if (was_primary) {
        // If other fingers are still down, promote oldest as new primary,
        // but do NOT re-arm long-press (avoid surprise).
        if (!_fingers.empty()) {
            _primary_id = _fingers.front().id;
        } else {
            _primary_id = -1;
        }
        _long_press_fired = false;
        _two_finger_fired = false;
    }
    return out;
}

std::vector<TouchSynthEvent> TouchInput::tick(uint32_t now_ms) {
    std::vector<TouchSynthEvent> out;
    if (_primary_id == -1 || _long_press_fired || _two_finger_fired) return out;

    auto *p = find(_primary_id);
    if (!p) return out;
    if (p->dragging || p->consumed) return out;
    if (over_slop(*p)) return out;

    uint32_t held = now_ms - p->down_time_ms;
    if (held >= (uint32_t)_long_press_ms) {
        _long_press_fired = true;
        p->consumed = true;
        out.push_back({ TouchSynthEvent::Move,         p->start_x, p->start_y });
        out.push_back({ TouchSynthEvent::RightPress,   p->start_x, p->start_y });
        out.push_back({ TouchSynthEvent::RightRelease, p->start_x, p->start_y });
    }
    return out;
}

int TouchInput::next_deadline_ms(uint32_t now_ms) const {
    if (_primary_id == -1 || _long_press_fired || _two_finger_fired) return -1;
    // Cannot use find() (const). Scan inline.
    for (const auto &f : _fingers) if (f.id == _primary_id) {
        if (f.dragging || f.consumed) return -1;
        uint32_t deadline = f.down_time_ms + (uint32_t)_long_press_ms;
        if (deadline <= now_ms) return 0;
        return (int)(deadline - now_ms);
    }
    return -1;
}
