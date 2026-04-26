# Visual Improvements Plan — Gates of Skeldal

## Status
- ✅ **Improvement 3: Pixel-art scalers** — DONE (Scale2x/Scale3x, F11 toggle) — merged to `main`
- ✅ **Improvement 1: 32-bit color** — DONE (branch: `visual/32bit-color`, NOT merged yet)
- ⬜ **Improvement 4: Post-processing** — TODO (requires 32-bit color)
- ⬜ **Improvement 2: AI-upscaled textures** — TODO (requires 32-bit color)

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
- **Gamma / contrast adjustment** — apply a simple power curve to all pixels before SDL upload. Makes the game look more vibrant on modern displays.
- **Color temperature / warmth** — shift the color balance (e.g., warmer for dungeons, cooler for ice levels). Just multiply R/G/B channels by different factors.
- **Smooth transparency** — `trans_bar` currently does 50% blend only. With 8-bit precision, we can do any opacity (10%, 25%, 75%, etc.) for better UI overlays.
- **Better distance fog** — palette shading already uses 256 shade levels instead of 32. Fog transitions are smoother, especially noticeable in long corridors.

### Medium Effort
- **Bloom / glow effect** — extract pixels above a brightness threshold, Gaussian blur them, and add back. Makes torches, magic effects, and bright surfaces glow.
- **Vignette** — darken the screen edges with a smooth radial gradient. Adds cinematic atmosphere.
- **Color grading via LUT** — apply a 3D color lookup table to transform the entire palette. Can simulate film looks (warm vintage, cold horror, etc.).
- **Per-pixel ambient occlusion approximation** — darken pixels near edges/corners based on local contrast.

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

## Notes
- Each improvement is on its own branch
- Improvements 2–4 benefit from Improvement 1 being done first
- Improvement 3 is independent and can be done anytime
- All improvements should be optional (INI-configurable) with zero-impact defaults

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
