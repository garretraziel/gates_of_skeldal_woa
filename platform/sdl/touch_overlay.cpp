#include "touch_overlay.h"

namespace {

constexpr int BTN_SIZE   = 44;   ///< window-pixel size of each button
constexpr int BTN_GAP    = 6;
constexpr int EDGE_PAD   = 6;    ///< distance from window edge

const TouchOverlay::Icon BUTTON_ORDER[] = {
    TouchOverlay::Icon::Esc,
    TouchOverlay::Icon::Enter,
    TouchOverlay::Icon::Tab,
    TouchOverlay::Icon::Backspace
};

SDL_Scancode scancode_for(TouchOverlay::Icon icon) {
    switch (icon) {
        case TouchOverlay::Icon::Esc:       return SDL_SCANCODE_ESCAPE;
        case TouchOverlay::Icon::Enter:     return SDL_SCANCODE_RETURN;
        case TouchOverlay::Icon::Tab:       return SDL_SCANCODE_TAB;
        case TouchOverlay::Icon::Backspace: return SDL_SCANCODE_BACKSPACE;
    }
    return SDL_SCANCODE_UNKNOWN;
}

} // namespace

void TouchOverlay::layout(int window_w, int window_h) {
    _buttons.clear();
    int n = sizeof(BUTTON_ORDER) / sizeof(BUTTON_ORDER[0]);
    int total_h = n * BTN_SIZE + (n - 1) * BTN_GAP;
    int x = window_w - BTN_SIZE - EDGE_PAD;
    int y0 = (window_h - total_h) / 2;
    if (y0 < EDGE_PAD) y0 = EDGE_PAD;
    for (int i = 0; i < n; ++i) {
        Button b;
        b.rect = { x, y0 + i * (BTN_SIZE + BTN_GAP), BTN_SIZE, BTN_SIZE };
        b.icon = BUTTON_ORDER[i];
        b.scancode = scancode_for(b.icon);
        _buttons.push_back(b);
    }
}

SDL_Scancode TouchOverlay::hit_test(int x, int y) const {
    if (!_visible) return SDL_SCANCODE_UNKNOWN;
    for (const auto &b : _buttons) {
        if (x >= b.rect.x && x < b.rect.x + b.rect.w &&
            y >= b.rect.y && y < b.rect.y + b.rect.h) {
            return b.scancode;
        }
    }
    return SDL_SCANCODE_UNKNOWN;
}

void TouchOverlay::render(SDL_Renderer *r) const {
    if (!_visible || _buttons.empty()) return;
    SDL_BlendMode prev;
    SDL_GetRenderDrawBlendMode(r, &prev);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (const auto &b : _buttons) {
        // Translucent black background
        SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
        SDL_RenderFillRect(r, &b.rect);
        // White border
        SDL_SetRenderDrawColor(r, 220, 220, 220, 220);
        SDL_RenderDrawRect(r, &b.rect);
        // Inset border for click feedback area
        SDL_Rect inset = { b.rect.x + 1, b.rect.y + 1, b.rect.w - 2, b.rect.h - 2 };
        SDL_RenderDrawRect(r, &inset);
        // Icon
        draw_icon(r, b.rect, b.icon);
    }
    SDL_SetRenderDrawBlendMode(r, prev);
}

void TouchOverlay::draw_icon(SDL_Renderer *r, const SDL_Rect &rc, Icon icon) const {
    int cx = rc.x + rc.w / 2;
    int cy = rc.y + rc.h / 2;
    int half = (rc.w - 14) / 2;
    if (half < 6) half = 6;

    switch (icon) {
        case Icon::Esc: {
            // Red X
            SDL_SetRenderDrawColor(r, 230, 80, 80, 255);
            for (int t = -1; t <= 1; ++t) {  // 3-px-thick
                SDL_RenderDrawLine(r, cx - half + t, cy - half, cx + half + t, cy + half);
                SDL_RenderDrawLine(r, cx + half + t, cy - half, cx - half + t, cy + half);
            }
            break;
        }
        case Icon::Enter: {
            // Green return arrow: ↵
            //   ___|
            // <─┘
            SDL_SetRenderDrawColor(r, 100, 220, 100, 255);
            // Vertical stem on the right (3 px wide), from top to middle
            SDL_Rect stem = { cx + half - 1, cy - half, 3, half + 2 };
            SDL_RenderFillRect(r, &stem);
            // Horizontal body (3 px tall), from arrow-head end to right stem
            SDL_Rect body = { cx - half + 7, cy - 1, half * 2 - 7, 3 };
            SDL_RenderFillRect(r, &body);
            // Left-pointing solid arrow head (filled triangle)
            for (int i = 0; i <= 7; ++i) {
                SDL_RenderDrawLine(r, cx - half + i, cy - i, cx - half + i, cy + i);
            }
            break;
        }
        case Icon::Tab: {
            // Blue 3x3 grid (map icon)
            SDL_SetRenderDrawColor(r, 100, 160, 240, 255);
            int step = (half * 2) / 3;
            for (int row = 0; row < 3; ++row) {
                for (int col = 0; col < 3; ++col) {
                    SDL_Rect cell = {
                        cx - half + col * step,
                        cy - half + row * step,
                        step - 2, step - 2
                    };
                    SDL_RenderFillRect(r, &cell);
                }
            }
            break;
        }
        case Icon::Backspace: {
            // Gray left arrow: ◄──
            SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
            // Body (3 px tall horizontal bar from arrow head to right edge)
            SDL_Rect body = { cx - half + 8, cy - 1, half * 2 - 8, 3 };
            SDL_RenderFillRect(r, &body);
            // Filled left-pointing triangle arrow head
            for (int i = 0; i <= 8; ++i) {
                SDL_RenderDrawLine(r, cx - half + i, cy - i, cx - half + i, cy + i);
            }
            break;
        }
    }
}
