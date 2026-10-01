/**
 * NSK OS v0.3 - Windows 11 / macOS Bloom Light Theme Wallpaper Engine
 * Generates soft ethereal sky-blue backdrop with lavender, violet and cyan silk bloom petals.
 */
#include "wallpaper.h"
#include "gfx.h"

// Fixed-point trigonometric approximation (0-255 angles, -256 to 256 amplitude)
static const int16_t sin_table[256] = {
      0,   6,  12,  18,  25,  31,  37,  43,  49,  56,  62,  68,  74,  80,  86,  92,
     97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
    180, 185, 189, 193, 197, 201, 205, 208, 212, 215, 219, 222, 225, 228, 231, 233,
    236, 238, 240, 242, 244, 246, 247, 249, 250, 251, 252, 253, 254, 254, 255, 255,
    256, 255, 255, 254, 254, 253, 252, 251, 250, 249, 247, 246, 244, 242, 240, 238,
    236, 233, 231, 228, 225, 222, 219, 215, 212, 208, 205, 201, 197, 193, 189, 185,
    180, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103,
     97,  92,  86,  80,  74,  68,  62,  56,  49,  43,  37,  31,  25,  18,  12,   6,
      0,  -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
    -97,-103,-109,-115,-120,-126,-131,-136,-142,-147,-152,-157,-162,-167,-171,-176,
   -180,-185,-189,-193,-197,-201,-205,-208,-212,-215,-219,-222,-225,-228,-231,-233,
   -236,-238,-240,-242,-244,-246,-247,-249,-250,-251,-252,-253,-254,-254,-255,-255,
   -256,-255,-255,-254,-254,-253,-252,-251,-250,-249,-247,-246,-244,-242,-240,-238,
   -236,-233,-231,-228,-225,-222,-219,-215,-212,-208,-205,-201,-197,-193,-189,-185,
   -180,-176,-171,-167,-162,-157,-152,-147,-142,-136,-131,-126,-120,-115,-109,-103,
    -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12,  -6
};

static inline int fast_sin(int angle_256) {
    return sin_table[angle_256 & 0xFF];
}

static inline int fast_cos(int angle_256) {
    return sin_table[(angle_256 + 64) & 0xFF];
}

void wallpaper_generate(uint32_t* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;

    // Center of the flower / bloom petal spiral (around bottom-center to center-right)
    int cx = width * 58 / 100;
    int cy = height * 72 / 100;

    for (int y = 0; y < height; y++) {
        // Ethereal Sky-Blue Ambient Base (#BFDBFE -> #E2E8F0)
        int base_r = 194 + (y * 28 / height);
        int base_g = 214 + (y * 18 / height);
        int base_b = 238 + (y * 10 / height);

        for (int x = 0; x < width; x++) {
            int dx = x - cx;
            int dy = y - cy;

            // Distance from bloom center
            int dist_sq = (dx * dx + dy * dy) >> 6;
            int dist = 0;
            // Integer sqrt
            while (dist * dist < dist_sq) dist++;

            // Angle for spiral petals
            int angle = (dx * 128 / (dist + 1)) + (fast_sin(y * 256 / height) / 8);

            // Petal 1: Lavender / Soft Periwinkle Silk Fold
            int p1 = fast_sin(angle * 3 + dist * 3);
            int glow_lavender = 0;
            if (p1 > 40 && dist < 240) {
                glow_lavender = (p1 - 40) * (240 - dist) / 200;
            }

            // Petal 2: Deep Cerulean / Royal Blue Fold
            int p2 = fast_cos(angle * 4 - dist * 2);
            int glow_blue = 0;
            if (p2 > 60 && dist < 220) {
                glow_blue = (p2 - 60) * (220 - dist) / 200;
            }

            // Petal 3: Soft White Highlight Ridge
            int p3 = fast_sin(angle * 5 + dist * 4 + 80);
            int glow_white = 0;
            if (p3 > 140 && dist < 210) {
                glow_white = (p3 - 140) * (210 - dist) / 120;
            }

            // Blend ambient base with 3D petals
            int r = base_r;
            int g = base_g;
            int b = base_b;

            // Add Lavender / Violet (#93C5FD / #C4B5FD)
            r = r - (glow_lavender * 35 / 256) + (glow_white * 45 / 256);
            g = g - (glow_lavender * 25 / 256) + (glow_white * 45 / 256);
            b = b + (glow_lavender * 20 / 256) + (glow_white * 45 / 256);

            // Add Deep Blue Creases (#3B82F6)
            r -= (glow_blue * 70 / 256);
            g -= (glow_blue * 45 / 256);
            b -= (glow_blue * 10 / 256);

            if (r < 0) r = 0; else if (r > 255) r = 255;
            if (g < 0) g = 0; else if (g > 255) g = 255;
            if (b < 0) b = 0; else if (b > 255) b = 255;

            buffer[y * width + x] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
