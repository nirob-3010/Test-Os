/**
 * NSK OS v0.3 - Authentic Windows 11 Bloom Wallpaper Engine
 * Scales the embedded real Bloom Silk Flower image (kernel/wallpaper_bloom.h)
 * across any screen resolution using high-quality bilinear interpolation.
 */
#include "wallpaper.h"
#include "wallpaper_bloom.h"
#include "gfx.h"

// Unpack RGB565 to 32-bit ARGB (0xFFRRGGBB)
static inline uint32_t rgb565_to_argb(uint16_t c) {
    uint32_t r = (c >> 11) & 0x1F;
    uint32_t g = (c >> 5) & 0x3F;
    uint32_t b = c & 0x1F;

    r = (r * 527 + 23) >> 6; // Fast 5-bit to 8-bit expansion
    g = (g * 259 + 33) >> 6; // Fast 6-bit to 8-bit expansion
    b = (b * 527 + 23) >> 6;

    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

void wallpaper_generate(uint32_t* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;

    // Fixed-point 16.16 step ratios
    uint32_t x_ratio = ((BLOOM_W - 1) << 16) / width;
    uint32_t y_ratio = ((BLOOM_H - 1) << 16) / height;

    for (int y = 0; y < height; y++) {
        uint32_t src_y_fp = y * y_ratio;
        int y0 = src_y_fp >> 16;
        int y1 = (y0 + 1 < BLOOM_H) ? y0 + 1 : y0;
        int y_diff = (src_y_fp >> 8) & 0xFF; // 8-bit fraction
        int y_diff_inv = 256 - y_diff;

        const uint16_t* row0 = &bloom_wallpaper_rgb565[y0 * BLOOM_W];
        const uint16_t* row1 = &bloom_wallpaper_rgb565[y1 * BLOOM_W];
        uint32_t* dst_row = &buffer[y * width];

        for (int x = 0; x < width; x++) {
            uint32_t src_x_fp = x * x_ratio;
            int x0 = src_x_fp >> 16;
            int x1 = (x0 + 1 < BLOOM_W) ? x0 + 1 : x0;
            int x_diff = (src_x_fp >> 8) & 0xFF; // 8-bit fraction
            int x_diff_inv = 256 - x_diff;

            // Sample 4 neighboring pixels
            uint32_t c00 = rgb565_to_argb(row0[x0]);
            uint32_t c10 = rgb565_to_argb(row0[x1]);
            uint32_t c01 = rgb565_to_argb(row1[x0]);
            uint32_t c11 = rgb565_to_argb(row1[x1]);

            // Bilinear blend weights
            int w00 = (x_diff_inv * y_diff_inv) >> 8;
            int w10 = (x_diff * y_diff_inv) >> 8;
            int w01 = (x_diff_inv * y_diff) >> 8;
            int w11 = (x_diff * y_diff) >> 8;

            // Interpolate Red
            int r = (((c00 >> 16) & 0xFF) * w00 +
                     ((c10 >> 16) & 0xFF) * w10 +
                     ((c01 >> 16) & 0xFF) * w01 +
                     ((c11 >> 16) & 0xFF) * w11) >> 8;

            // Interpolate Green
            int g = (((c00 >> 8) & 0xFF) * w00 +
                     ((c10 >> 8) & 0xFF) * w10 +
                     ((c01 >> 8) & 0xFF) * w01 +
                     ((c11 >> 8) & 0xFF) * w11) >> 8;

            // Interpolate Blue
            int b = ((c00 & 0xFF) * w00 +
                     (c10 & 0xFF) * w10 +
                     (c01 & 0xFF) * w01 +
                     (c11 & 0xFF) * w11) >> 8;

            dst_row[x] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
