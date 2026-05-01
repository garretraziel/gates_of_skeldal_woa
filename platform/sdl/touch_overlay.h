#pragma once

#include <SDL.h>
#include <vector>

/// On-screen virtual key panel for touchscreen users.
///
/// Renders a small column of buttons over the game (after CRT pass), in
/// window-pixel space so it stays crisp regardless of the game's source
/// resolution. Tapping a button injects an SDL scancode into SDLContext's
/// keyboard queue (same path that real keyboard events go through).
///
/// Buttons are drawn as semi-translucent rounded-ish rectangles with simple
/// geometric icons. No bitmap font dependency — keeps the code minimal.
class TouchOverlay {
public:
    enum class Icon {
        Esc,            ///< red X — cancel/back/menu
        Enter,          ///< green ↵ — confirm/attack-start
        Tab,            ///< blue grid — automap
        Backspace       ///< gray ← — cancel battle action
    };

    struct Button {
        SDL_Rect rect;          ///< window-pixel coordinates
        SDL_Scancode scancode;
        Icon icon;
    };

    /// Recompute button positions for current window size.
    void layout(int window_w, int window_h);

    bool is_visible() const { return _visible; }
    void set_visible(bool v) { _visible = v; }

    /// Returns scancode if (window_x, window_y) hit a button, otherwise SDL_SCANCODE_UNKNOWN.
    SDL_Scancode hit_test(int window_x, int window_y) const;

    /// Draws the overlay using the given renderer.
    void render(SDL_Renderer *r) const;

private:
    std::vector<Button> _buttons;
    bool _visible = true;

    void draw_icon(SDL_Renderer *r, const SDL_Rect &rect, Icon icon) const;
};
