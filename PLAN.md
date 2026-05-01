# Improvements Plan — Gates of Skeldal

## Status

**Visual track:**
- ✅ **Improvement 3: Pixel-art scalers** — DONE (Scale2x/Scale3x, F11 toggle) — merged
- 🔧 **Improvement 1: 32-bit color** — IN PROGRESS on branch `visual/32bit-color` (not yet merged)
- 🔧 **Improvement 4: Post-processing** — IN PROGRESS on `visual/32bit-color` (gamma/brightness/color_temperature LUT, smooth transparency)
- ⬜ **Improvement 2: AI-upscaled textures** — TODO (depends on 32-bit color)

**Input / Accessibility track:**
- ⬜ **Improvement 5: Touchscreen support** — PLANNED (will be implemented on a new `feature/touchscreen` branch off `main`)

## Architecture Context

- **Framebuffer**: 640×480, 16-bit RGB555 (`word*` buffers)
- **3D viewport**: 640×360 (bottom 120px is UI)
- **Textures**: 8-bit indexed with per-surface 16-bit palettes
- **Renderer**: Software lookup-table perspective, not raycasting
- **Output**: Software → SDL streaming texture → SDL_RenderCopy → window
- **Key files**: `libs/engine1.c`, `libs/bgraph2.c`, `game/builder.c`, `platform/sdl/sdl_context.cpp`, `platform/sdl/BGraph2.cpp`

## Improvement 1: 32-bit Color (RGB8888)

**Branch**: `visual/32bit-color`
**Goal**: Eliminate 16-bit color banding by upgrading internal rendering to 32-bit RGBA.
**Impact**: Most visually impactful single change — smoother gradients, richer colors.

### Scope
- Change screen buffers from `word*` (uint16_t) to `uint32_t*`
- Update `RGB555()` macro to produce 32-bit RGBA values
- Update palette conversion: 8-bit indexed → 32-bit instead of 16-bit
- Update all blitting/drawing functions to operate on 32-bit pixels
- Update SDL texture format selection to prefer RGBA8888
- Update `scr_linelen` / `scr_linelen2` semantics (currently in words)

### Key files to modify
- `libs/bgraph.h` — `curcolor`, `charcolors`, screen pointer types
- `libs/bgraph2.c` — screen buffer allocation, drawing primitives
- `libs/bgraph2a.c` — line/bar/font rendering
- `libs/engine1.c` — 3D rendering, texture zoom/scroll routines
- `game/engine1.c` — game-side rendering
- `game/engine2.c` — animation rendering
- `game/builder.c` — scene construction
- `platform/sdl/sdl_context.cpp` — SDL texture format, present_rect
- `platform/sdl/BGraph2.cpp` — buffer allocation, showview

### Risks
- Very wide-reaching change — almost every rendering file is affected
- `word` type used pervasively for pixel data
- Palette format in DDL game data is 16-bit — need conversion layer
- Memory usage doubles for screen buffers (minimal concern on modern hardware)

---

## Improvement 2: AI-Upscaled Textures

**Branch**: `visual/upscaled-textures`
**Goal**: Higher-resolution textures via offline AI upscaling of original 8-bit assets.
**Impact**: Sharper walls, floors, objects without changing the rendering engine.

### Scope
- Extract textures from SKELDAL.DDL
- Run through AI upscaler (e.g., waifu2x, ESRGAN, or Real-ESRGAN)
- Modify texture loading to support higher-res variants
- Adjust zoom/blit tables for higher-res textures
- Package as optional texture pack (keep original as fallback)

### Key files to modify
- `libs/memman.c` — asset loading
- `libs/engine1.c` — texture dimension handling
- DDL tooling (`tools/ddl_ar.cpp`) — for extraction/repacking

### Risks
- Lookup tables are precomputed for specific texture sizes — may need recalculation
- 8-bit indexed format means upscaling must preserve palette or switch to direct color
- Depends on Improvement 1 (32-bit color) for best results

---

## Improvement 3: Better Upscale Filters (xBRZ / HQx)

**Branch**: `visual/pixel-art-scalers`
**Goal**: Add high-quality pixel-art scaling algorithms as alternatives to the existing CRT filter.
**Impact**: Smoother, cleaner image at higher display resolutions.

### Scope
- Integrate xBRZ or HQx scaler library
- Apply as post-processing step before SDL_RenderCopy
- Add INI option: `scale_filter = none | crt | xbrz | hqx`
- Existing CRT filter infrastructure can be extended

### Key files to modify
- `platform/sdl/sdl_context.cpp` — rendering pipeline, filter application
- `platform/sdl/BGraph2.cpp` — filter option parsing
- `skeldal.ini` — new config option

### Risks
- Low risk — purely additive, doesn't change internal rendering
- Performance: xBRZ at 4x on 640×480 → 2560×1920 may need optimization
- Easiest improvement to implement

---

## Improvement 4: Post-Processing Effects

**Branch**: `visual/post-processing`
**Goal**: Add optional visual effects: bloom, vignette, color grading, ambient occlusion approximation.
**Impact**: More atmospheric look, modernized visual feel.

### Scope
- Implement as shader passes on the final SDL texture (if using hardware renderer)
- OR as CPU-based post-processing on the framebuffer (software fallback)
- Add INI options to enable/disable individual effects
- Bloom: extract bright pixels, blur, add back
- Vignette: darken edges
- Color grading: LUT-based color transformation

### Key files to modify
- `platform/sdl/sdl_context.cpp` — post-processing pipeline
- `platform/sdl/BGraph2.cpp` — option parsing
- New file: `platform/sdl/post_process.cpp`

### Risks
- Shader-based approach requires OpenGL/Vulkan SDL renderer (not software)
- CPU fallback would need to be fast enough at 60fps
- Should be entirely optional — no impact when disabled

---

## Recommended Order

1. **Pixel-art scalers** (Improvement 3) — lowest risk, immediate visual payoff
2. **32-bit color** (Improvement 1) — foundational for other improvements
3. **Post-processing** (Improvement 4) — builds on 32-bit color
4. **AI-upscaled textures** (Improvement 2) — builds on 32-bit color, most complex

## Notes
- Each improvement is on its own branch
- Improvements 2–4 benefit from Improvement 1 being done first
- Improvement 3 is independent and can be done anytime
- All improvements should be optional (INI-configurable) with zero-impact defaults

---

## Improvement 5: Touchscreen Support

**Branch**: `feature/touchscreen` (NEW — fork from `main`, NOT from `visual/32bit-color`)
**Goal**: Make the game fully playable on a Surface Pro / Windows tablet using only the touchscreen — no keyboard or mouse.
**Impact**: Opens the game to a new platform (tablet) without changing core gameplay.

### Why a separate branch off `main` (not `visual/32bit-color`)

Touch controls are orthogonal to color depth. The 32-bit color branch is large and still settling, and shouldn't gate this feature. Conflict analysis below shows merging the two later is mechanical, not semantic.

### Conflict analysis: 32-bit color × touch controls

**Files modified by both planned tracks:**

| File | 32-bit hunks | Touch additions | Real conflict? |
|---|---|---|---|
| `platform/sdl/sdl_context.cpp` | `convert_bitmap`, `update_texture_with_conversion`, `update_screen` (palette/scale paths), `init_window` (post-process LUT init) | new `SDL_FINGERDOWN/UP/MOTION` cases in `event_loop`, gated cursor-draw branch in `refresh_screen`, `SDL_TEXTINPUT` case | **No.** Disjoint regions; both are additive. |
| `platform/sdl/sdl_context.h` | added `PostProcessLUT _post_lut`, `PostProcessConfig` in `VideoConfig` | adds `TouchInput _touch`, `_input_mode` enum, `TouchConfig` in `VideoConfig` | **No.** Adjacent additions in same struct — trivial 3-way merge. |
| `platform/sdl/BGraph2.cpp` | new INI block parsing `gamma`/`brightness`/`color_temperature` under `[video]` | new INI block parsing `[touch]` section | **No.** Different sections, additive. |
| `platform/sdl/CMakeLists.txt` | added `post_process.cpp` | adds `touch_input.cpp`, `touch_overlay.cpp`, `soft_keyboard.cpp` | **No.** Trivial line additions. |
| `game/souboje.c` | hunks at lines 1477, 1509, 1678, 2014, 2169 (pixel_t conversions) | edits to `clk_souboje[]` table around line 151 + new ~30×30 turn buttons in `clk_presun[]` (174-219) | **No.** Different lines. The mask-bit change at line 151 is a single literal in a static data table that 32-bit branch leaves untouched. |
| `game/dialogy.c`, `game/interfac.c`, `game/automap.c`, `libs/basicobj.c` | pixel_t type updates throughout | adds `game_display_set_touch_mode(...)` / `game_display_begin_text_input(...)` calls at flow entry/exit points | **No.** Function calls into platform layer; pixel format agnostic. |
| `skeldal.ini` | new keys in `[video]` | new `[touch]` section | **No.** Different sections. |

**Semantic risk areas:**

1. **Touch overlay & soft keyboard rendering** — drawn via SDL primitives directly (`SDL_RenderCopy` on independent SDL_Textures, after CRT pass). They never touch the game's framebuffer pixel type. ✅ pixel_t-independent.
2. **Battle turn arrow buttons** — drawn using existing helpers (`bar32`, `put_picture`, `set_font`, `trans_bar`). The API names are identical on both branches; only the underlying pixel storage differs. The new code calling those helpers compiles unchanged. ✅
3. **Click-snap in `clk_map.c`** — operates on integer rectangle coordinates from `T_CLK_MAP` arrays. Coordinates are unrelated to color. ✅
4. **INI parsing** — additive sections, no overlap. ✅

**Estimated merge effort:** When 32-bit color eventually merges to `main`, then touch merges:
- ~5 mechanical conflicts in `sdl_context.cpp` / `.h` / `BGraph2.cpp` / `CMakeLists.txt` / `skeldal.ini` (additive sections, both kept)
- 0 semantic conflicts expected
- **Estimated resolution time: 30 min – 1 hour**

### Plan summary

Build progressively in 9 phases on a fresh `feature/touchscreen` branch off `main`:

| Phase | Scope | Goal |
|---|---|---|
| 0 | `TouchInput` class with proper PRESS/RELEASE model, timer-driven long-press, coord clamp + slop, defensive event filter | Foundation |
| 1 | Cursor hiding on touch; basic FINGER → MS_EVENT wiring | Less visual clutter |
| 5b | Click-snap in `clk_map.c` (+12 px tolerance, snap-to-nearest in touch mode) | Most cramped UI usable immediately |
| 2 | Battle turn arrows + fix `clk_fly_cursor` mask | Resolve #1 blocker |
| 3 | Always-visible helper bar for ESC / ENTER / TAB / F-keys | Reach all keyboard-only menus |
| 4 | `SDL_TEXTINPUT` bridge + in-game soft QWERTY (for save names, character names, automap notes) | Text entry without keyboard |
| 5a | Mode-aware gesture policy (per-mode long-press / right-click suppression) + drag-scroll | Prevent destructive accidents |
| 6 | Hold-repeat verification, palm/jitter rejection refinement | Polish |
| 7 | Configuration (`[touch]` INI section), tap visual feedback, optional debug overlay | Final polish |
| 8 | Tests: unit tests for `TouchInput` on synthetic events, dev-only mouse-as-touch shim | Sustainable |

### UI size audit summary

Audited every clickable surface; thresholds: ✅ ≥30 px, ⚠️ 20–29 px, ❌ <20 px (native, before window scaling).

**Worst offenders (require click-snap):**
- NPC dialog top buttons & choice rows — 14 px tall (`game/dialogy.c:117-133`)
- Battle top bar (map/setup/save/load/book) — 14 px tall (`game/souboje.c:135-153`)
- Automap top controls — 20×14 (`game/automap.c:73-99`)
- Generic GUI buttons — typically 20 px tall (`libs/basicobj.c:112-195`)

**Borderline (acceptable with click-snap):**
- Spell rune bar — 24×22 (`game/souboje.c`)
- Battle power buttons — 102×20

**Healthy (no fix needed):**
- Save/load slot rows ~33 px, inventory cells 55×60, chargen portraits 306×37, battle movement arrows ~30×30

Note: small gaps between adjacent **large** buttons are not a problem — taps in a 0–2 px gap are unambiguously attributable to one neighbor. Only button **size** drives the rating.

### Approach: how to fix small targets without redrawing the game

The visual artwork is original and shouldn't change. Instead:
- **Expand click regions invisibly** in touch mode — extend `T_CLK_MAP` rectangles by N px transparently
- **Snap-to-nearest** when expanded regions overlap — pick the target whose center is closest to the tap
- Implemented as a ~20-line second-pass in `clk_map.c`'s region matcher; a single feature gate fixes every cramped screen at once
- Mouse users keep pixel-precise behavior (snap is touch-mode-only)

### Scope (what's IN)

- Pure SDL2 — no external touch library
- Phases 0–8 above
- Windows tablet (Surface Pro) primary target
- Code is portable; iOS/Android possible but untested

### Out of scope

- Multi-touch zoom (game framebuffer is fixed 640×480; window scaling exists separately)
- Reworking the click-map system architecture
- New tutorial/onboarding screens (a one-time hint may be added in Phase 7 if needed)
- Localized soft keyboards (Czech accented chars TBD)

### Key risks / open issues

1. Battle UI is full — exact placement for turn arrow buttons TBD (likely overlaid translucent on viewport edges)
2. Helper bar contents per-context (battle vs main menu vs dialog) — to be tuned in Phase 3
3. Soft keyboard layout — minimal alphabetic vs full QWERTY — decide in Phase 4
4. Stale long-press validation — when a 500 ms timer fires, must re-verify finger still down, still in slop, still same mode (handled in Phase 0)

### New `[touch]` INI section (planned)

```ini
[touch]
enabled = auto              # auto | on | off
long_press_ms = 500
right_click_via = long_press,two_finger
helper_bar = top_right      # top_right | top_left | bottom | off
hide_cursor_on_touch = on
target_snap_px = 12
slop_radius_px = 12
battle_hints = on
soft_keyboard = auto
```
