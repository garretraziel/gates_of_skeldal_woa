# Visual Improvements Plan — Gates of Skeldal

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
