#include "post_process.h"

void PostProcessLUT::init(const PostProcessConfig &cfg) {
    if (cfg.is_default()) {
        _identity = true;
        for (int i = 0; i < 256; ++i) {
            _lut_r[i] = _lut_g[i] = _lut_b[i] = (uint8_t)i;
        }
        return;
    }

    // Color temperature: shift R/B channels
    // Warm (positive): boost red, reduce blue
    // Cool (negative): reduce red, boost blue
    // Green channel stays neutral
    float temp = std::clamp(cfg.color_temperature, -100, 100) / 100.0f;
    float r_factor = 1.0f + temp * 0.15f;   // ±15% at max
    float g_factor = 1.0f;
    float b_factor = 1.0f - temp * 0.15f;

    float inv_gamma = 1.0f / std::clamp(cfg.gamma, 0.1f, 5.0f);
    float bright = std::clamp(cfg.brightness, 0.1f, 5.0f);

    _identity = true;
    for (int i = 0; i < 256; ++i) {
        float normalized = i / 255.0f;
        float corrected = std::pow(normalized, inv_gamma) * bright;

        auto to_byte = [](float v) -> uint8_t {
            int val = (int)(v * 255.0f + 0.5f);
            return (uint8_t)std::clamp(val, 0, 255);
        };

        _lut_r[i] = to_byte(corrected * r_factor);
        _lut_g[i] = to_byte(corrected * g_factor);
        _lut_b[i] = to_byte(corrected * b_factor);

        if (_lut_r[i] != i || _lut_g[i] != i || _lut_b[i] != i) {
            _identity = false;
        }
    }
}

void PostProcessLUT::apply_buffer(uint32_t *pixels, int count) const {
    if (_identity) return;
    for (int i = 0; i < count; ++i) {
        pixels[i] = apply(pixels[i]);
    }
}
