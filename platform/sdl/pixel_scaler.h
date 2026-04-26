#pragma once
#include <cstdint>
#include "../platform.h"

enum class PixelScaleType {
    none,
    scale2x,
    scale3x
};

/// Scale2x (EPX/AdvMAME2x) pixel art upscaling on pixel_t pixels
/// srcPitch and dstPitch are in pixels (not bytes)
void pixel_scale2x(const pixel_t *src, pixel_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch);

/// Scale3x (AdvMAME3x) pixel art upscaling on pixel_t pixels
/// srcPitch and dstPitch are in pixels (not bytes)
void pixel_scale3x(const pixel_t *src, pixel_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch);
