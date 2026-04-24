#pragma once
#include <cstdint>

enum class PixelScaleType {
    none,
    scale2x,
    scale3x
};

/// Scale2x (EPX/AdvMAME2x) pixel art upscaling on RGB555 pixels
/// srcPitch and dstPitch are in pixels (not bytes)
void pixel_scale2x(const uint16_t *src, uint16_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch);

/// Scale3x (AdvMAME3x) pixel art upscaling on RGB555 pixels
/// srcPitch and dstPitch are in pixels (not bytes)
void pixel_scale3x(const uint16_t *src, uint16_t *dst,
                   int srcW, int srcH, int srcPitch, int dstPitch);
