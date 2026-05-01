# Improvements Plan — Gates of Skeldal

## Status — Visual track
- ✅ **Improvement 3: Pixel-art scalers** — DONE (Scale2x/Scale3x, F11 toggle) — merged to `main`
- ✅ **Improvement 1: 32-bit color** — DONE (branch: `visual/32bit-color`, NOT merged yet)
- 🔧 **Improvement 4: Post-processing** — IN PROGRESS (quick wins: gamma/brightness/color_temperature LUT, smooth transparency)
- ⬜ **Improvement 2: AI-upscaled textures** — TODO (requires 32-bit color)

## Status — Input / Accessibility track
- ⬜ **Improvement 5: Touchscreen support** — PLANNED (new branch off `main`, see "Touchscreen Support Plan" below)

## Architecture Context (after 32-bit upgrade)

- **Framebuffer**: 640×480, **32-bit ARGB** (`pixel_t*` buffers, `pixel_t = uint32_t`)
- **3D viewport**: 640×360 (bottom 120px is UI)
- **Textures**: 8-bit indexed with per-surface **32-bit palettes** (converted from 24-bit PCX on load)
- **Asset format**: DDL files still contain 16-bit RGB555 palettes — converted on render via `rgb555to32()`
- **Renderer**: Software lookup-table perspective, not raycasting
- **Output**: Software → SDL streaming texture (ARGB8888) → SDL_RenderCopy → window
- **Transparency**: `BGSWITCHBIT (0x80000000)` = transparent, `0x00RRGGBB` = opaque
- **Key macros**: `RGB555()`, `RGB888()`, `PIXEL_IS_TRANSPARENT()`, `BLEND_PIXELS()`, `PIXEL_NODRAW`
- **Mode values in image headers**: 
  - `8` = raw DDL 8-bit indexed (16-bit palette at offset 6)
  - `15/16` = raw DDL hicolor (16-bit pixel data)
  - `32` = PCX-loaded hicolor (pixel_t data)
  - `808` = PCX-loaded 8-bit indexed (pixel_t palette at offset 6)
  - `808+256` = PCX-loaded 8-bit with shade palettes (pixel_t)
- **Key files**: `libs/engine1.c`, `libs/bgraph2.c`, `game/builder.c`, `platform/sdl/sdl_context.cpp`, `platform/sdl/BGraph2.cpp`

## New Possibilities Enabled by 32-bit Color

With the framebuffer now in 32-bit ARGB (8 bits per channel), the following enhancements are now possible:

### Quick Wins (low effort, immediate visual impact)
- ✅ **Gamma / contrast adjustment** — per-channel LUT applied during SDL pixel format conversion. INI: `gamma=1.0` (0.5–2.0)
- ✅ **Brightness adjustment** — folded into the same LUT. INI: `brightness=1.0` (0.5–2.0)
- ✅ **Color temperature / warmth** — shifts R/B balance, folded into same LUT. INI: `color_temperature=0` (-100 to +100)
- ✅ **Smooth transparency** — `trans_bar_alpha(x, y, xs, ys, color, alpha)` with per-channel alpha blending (0–255). `trans_bar25` now uses proper 25% opacity.
- ✅ **Targeted UI/effect blending pass** — shared 32-bit blend helpers plus softer disabled controls, cleaner rune-bar dimming, and improved chargen pearl shading/glow.
- **Better distance fog** — palette shading already uses 256 shade levels instead of 32. Fog transitions are smoother, especially noticeable in long corridors.
- **Spell / projectile glows** — additive highlights for magic shots, impacts, shields, and rune effects.
- **Scene mood overlays** — underwater tint, poison haze, confusion/night-vision, heat/lava tint.
- **UI translucency and highlights** — softer selection rims, translucent panels, improved hover/pressed states.
- **Reworked floor/ceiling autofade** — replace remaining RGB555-era interpolation with proper per-channel 32-bit shading.

### Medium Effort
- **Bloom / glow effect** — extract pixels above a brightness threshold, Gaussian blur them, and add back. Makes torches, magic effects, and bright surfaces glow.
- **Vignette** — darken the screen edges with a smooth radial gradient. Adds cinematic atmosphere.
- **Color grading via LUT** — apply a 3D color lookup table to transform the entire palette. Can simulate film looks (warm vintage, cold horror, etc.).
- **Per-pixel ambient occlusion approximation** — darken pixels near edges/corners based on local contrast.
- **Combat readability pass** — color-coded damage flashes, clearer target highlight, smoother disabled action states.
- **Soft fog / mist overlays** — dither-free atmospheric layers for sectors, spells, and environmental effects.

### Larger Scope
- **AI-upscaled texture packs** — extract 8-bit textures from DDL, upscale offline with ESRGAN/Real-ESRGAN, reload as higher-res pixel_t data. With pixel_t palettes, the rendering pipeline already supports direct 32-bit color output.
- **True alpha blending for UI** — use the alpha channel (currently only used for transparency flag) for smooth semi-transparent UI panels over the 3D viewport.
- **HDR-like rendering** — accumulate lighting in higher precision (the 8-bit channels give much more headroom than the old 5-bit ones), then tone-map to display range.
- **Dynamic palette effects** — modify palette entries in real-time for effects like pulsing lights, poison tinting, low-health red flash, etc. Now with 16 million colors instead of 32K.

### Implementation Notes
- Best hook point for post-processing: in `sdl_context.cpp` between `_shadow_buffer` update and `SDL_UpdateTexture` call
- The pixel scaler (Scale2x/Scale3x) should run AFTER post-processing
- Post-processing should be optional (INI-configurable) with zero overhead when disabled
- Color grading LUT can be precomputed at startup and applied per-pixel very efficiently

## Improvement 1: 32-bit Color (RGB8888) — COMPLETED

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

## Recommended Next Batch on `visual/32bit-color`

1. **Spell and projectile glows** — the most visible gameplay-facing improvement now that additive and alpha blending helpers exist
2. **Full-screen mood overlays** — underwater, poison, confusion, lava/heat, and night-vision style grading
3. **UI panel and selection polish** — translucent panels, cleaner highlights, better hover/disabled states
4. **Floor/ceiling autofade rewrite** — replace remaining legacy shading math with proper 32-bit blending

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

Touch controls are orthogonal to color depth. The 32-bit color branch is large (~35 files), still settling, and shouldn't gate this feature. Conflict analysis below shows merging the two later is mechanical, not semantic.

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

**Files only one branch touches (zero conflict):**
- 32-bit only: `libs/bgraph2.c`, `libs/bgraph.h`, `libs/bgraph2a.c`, `libs/engine1.c`, `libs/pcx.c`, `libs/mgifplaya.c`, `libs/mgifmapmem.c`, `libs/gui.c`, `libs/basicobj.h`, `game/builder.c`, `game/engine1.c/.h`, `game/engine2.c`, `game/chargen.c/2.c`, `game/inv.c`, `game/menu.c`, `game/realgame.c`, `game/skeldal.c`, `game/specproc.c`, `game/gamesave.c`, `game/globals.h`, `game/globmap.c`, `game/dump.cpp`, `game/kouzla.c`, `platform/platform.h`, `platform/sdl/pixel_scaler.cpp/.h`, `platform/sdl/post_process.cpp/.h`, `platform/sdl/BGraph2.h`, `platform/CMakeLists.txt`, `malloc.h`
- Touch only: `game/clk_map.c` (snap-to-nearest), 3 new SDL files, plus the touch-mode hooks listed above

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

---

## Detailed Plan: 32-bit Color (RGB8888) Upgrade

### Strategy

Rather than changing every `word` to `uint32_t` across 30+ files, use a **typedef approach**:

1. Define `pixel_t` as the pixel type (currently `uint16_t`, change to `uint32_t`)
2. Update macros (`RGB555` → `RGB8888`) in one header
3. Update palette conversion (8-bit indexed → 32-bit)
4. Update screen buffers and pitch calculations
5. Update SDL output (skip 16→32 conversion)

### Phase 1: Foundation (types and macros)

#### 1a. `platform/platform.h` — Core pixel type and color macros
- Add `typedef uint32_t pixel_t;`
- Change `RGB555(r,g,b)` macro to produce 32-bit: `((r<<19)|(g<<11)|(b<<3)|0xFF000000)`
- Change `RGB555_ALPHA(r,g,b)` for transparent: `((r<<19)|(g<<11)|(b<<3))`
- Change `BGSWITCHBIT` from `0x8000` to `0x80000000` (alpha bit)
- Add backward-compat `RGB555to32(c)` converter for reading legacy 16-bit palette data

#### 1b. `libs/types.h` — Keep `word` as `uint16_t` (it's used for non-pixel things too)

#### 1c. `libs/bgraph.h` — Screen interface types
- Change `curcolor` from `word` to `pixel_t`
- Change `charcolors[7]` from `word` to `pixel_t`
- Change `GetScreenAdr()` return from `word*` to `pixel_t*`
- Change `showview` signature parameters if needed

### Phase 2: Screen buffers

#### 2a. `libs/bgraph2.c` — Buffer declarations
- `word *screen` → `pixel_t *screen`
- `word curcolor` → `pixel_t curcolor`
- `word charcolors[7]` → `pixel_t charcolors[7]`
- `screen_buffer_size` calculations: multiply by 4 instead of 2

#### 2b. `platform/sdl/BGraph2.cpp` — SDL screen buffer
- `screen_buffer` from `uint16_t[]` to `pixel_t[]`
- `screen_pitch` stays in pixels (unchanged)
- `present_rect` — change from `uint16_t*` to `pixel_t*`

#### 2c. `libs/engine1.c` / `game/engine1.c` — Rendering buffers
- `word *buffer_2nd` → `pixel_t *buffer_2nd`
- All `memcpy` sizes that use `*2` → `*sizeof(pixel_t)`
- `screen_buffer_size = 640*480*sizeof(pixel_t)`

### Phase 3: Drawing primitives

#### 3a. `libs/bgraph2a.c` — Lines, bars, font rendering
- `bar32`, `hor_line32`, `ver_line32` — change `word*` → `pixel_t*`
- `curcolor2` — 32-bit pair packing needs rethinking (was `curcolor | (curcolor<<16)`)
- `char_32`, `char2_32` — font blitter pixel writes
- `charsize` — unaffected (returns dimensions not pixels)

#### 3b. `libs/bgraph2a.cpp` — Assembly font rendering (NOT COMPILED, skip)

### Phase 4: Palette conversion

#### 4a. `libs/pcx.c` — PCX loading
- PCX files store 24-bit RGB palettes (768 bytes at end of file)
- Currently converts to 16-bit RGB555 palette
- Change to produce 32-bit RGBA palette entries
- Lines 109-150 are the key conversion code

#### 4b. `game/engine2.c` — Animation/sprite rendering
- `word *paleta` → `pixel_t *paleta` in all functions
- `small_anm_buff`, `small_anm_delta` — palette lookup produces pixel_t

#### 4c. `libs/mgifplaya.c` — Video playback
- `show_full_lfb12e`, `show_delta_lfb12e` — palette lookup → pixel_t output

#### 4d. `libs/mgifmapmem.c` — MGIF animation
- `paleta` pointer type change
- `StretchImageHQ` — pixel type change

### Phase 5: Rendering engine

#### 5a. `libs/engine1.c` — 3D perspective engine
- `word *palette` in zoom struct → `pixel_t *palette`
- All texture rendering functions that write pixels
- `zoom.palette` usage throughout

#### 5b. `game/engine1.c` — Game-side 3D rendering
- Same palette/pixel changes

#### 5c. `game/builder.c` — Scene construction
- Color blending code (lines ~800-810) — RGB channel extraction needs updating
- `0x8000` transparency bit → `0x80000000`

### Phase 6: UI and game code

#### 6a. Files with `curcolor = RGB555(...)` assignments (many files):
- `game/automap.c`, `game/chargen.c`, `game/dialogy.c`, `game/gamesave.c`
- `game/interfac.c`, `game/inv.c`, `game/menu.c`, `game/skeldal.c`
- `game/souboje.c`, `game/setup.c`, `game/console.c`
- `libs/basicobj.c`, `libs/gui.c`
- These should "just work" if `RGB555` macro is updated — verify each

#### 6b. Files with RGB555 bit manipulation (CRITICAL):
- `game/builder.c` — channel shifts `>>10`, `>>5`, `& 0x1F` → `>>16`, `>>8`, `& 0xFF`
- `game/engine2.c` — `0x8000` transparency, `0x7BDE` blend mask
- `game/skeldal.c` — `0x80008000`, `~0x1F`, `~BGSWITCHBIT`
- `libs/basicobj.c` — `0x8000` / transparency checks
- `libs/bgraph2.c` — pixel format bit extraction
- `platform/sdl/sdl_context.cpp` — 16-bit unpacking (can be simplified for 32-bit)

### Phase 7: SDL output

#### 7a. `platform/sdl/sdl_context.cpp`
- `present_rect` — accepts `pixel_t*` instead of `uint16_t*`
- `update_texture_with_conversion` — when pixels are already 32-bit RGBA, skip conversion
- Choose `SDL_PIXELFORMAT_ARGB8888` as default format
- `convert_bitmap` templates can be simplified

#### 7b. `platform/sdl/pixel_scaler.cpp`
- Update Scale2x/Scale3x to work on `pixel_t` (uint32_t) instead of uint16_t

### Phase 8: Asset compatibility layer

#### 8a. Legacy palette conversion
- Game data files store 16-bit RGB555 palettes
- Add `RGB555to32()` function to convert on load
- Apply in `pcx.c`, `mgifplaya.c`, `mgifmapmem.c`, `engine2.c`

### Risk Summary

| Risk | Impact | Mitigation |
|------|--------|------------|
| Screen buffer size doubles (640×480×4 = 1.2MB) | Low | Negligible on modern hardware |
| Palette data in DDL is 16-bit | Medium | Convert on load with RGB555to32() |
| Bit manipulation hardcoded in many files | High | Systematic search-and-replace |
| `word` used for both pixels AND non-pixel data | High | Only change pixel-specific uses |
| Pixel scaler needs updating | Low | Change uint16_t → pixel_t |

### Estimated scope: ~35 files, ~500 lines of changes
