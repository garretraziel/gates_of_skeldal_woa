#pragma once
#include "../platform.h"
#include <cstdint>
#include <cmath>
#include <algorithm>

/// Configuration for post-processing color adjustments
struct PostProcessConfig {
    float gamma = 1.0f;             ///< Gamma curve (0.5 = brighter, 2.0 = darker)
    float brightness = 1.0f;        ///< Brightness multiplier (0.5 = dim, 2.0 = bright)
    int color_temperature = 0;      ///< Color temperature shift (-100 = cool/blue, +100 = warm/amber)

    bool is_default() const {
        return gamma == 1.0f && brightness == 1.0f && color_temperature == 0;
    }
};

/// Per-channel color lookup table for fast pixel transformation
/// Combines gamma, brightness, and color temperature into a single LUT pass
class PostProcessLUT {
public:
    /// Recompute LUT from config. Call once at startup or when config changes.
    void init(const PostProcessConfig &cfg);

    /// Returns true if LUT is identity (no transformation needed)
    bool is_identity() const { return _identity; }

    /// Apply LUT to a single pixel (ARGB format, preserves alpha)
    inline uint32_t apply(uint32_t pixel) const {
        uint32_t r = (pixel >> 16) & 0xFF;
        uint32_t g = (pixel >> 8) & 0xFF;
        uint32_t b = pixel & 0xFF;
        uint32_t a = pixel & 0xFF000000u;
        return a | ((uint32_t)_lut_r[r] << 16)
                 | ((uint32_t)_lut_g[g] << 8)
                 |  (uint32_t)_lut_b[b];
    }

    /// Apply LUT to buffer of pixels in-place
    void apply_buffer(uint32_t *pixels, int count) const;

private:
    uint8_t _lut_r[256];
    uint8_t _lut_g[256];
    uint8_t _lut_b[256];
    bool _identity = true;
};
