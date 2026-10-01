/**
 * NSK OS v0.3 - Procedural Blooming Wave Wallpaper Engine (Phase 2)
 * Generates the signature vibrant Blooming Wave / Aurora Borealis wallpaper
 */
#include "wallpaper.h"
#include "gfx.h"

// Integer fixed-point trigonometric approximations for fast rendering
// Sin table: 256 entries for 0 to 2*PI, values scaled by 256 (-256 to +256)
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

    for (int y = 0; y < height; y++) {
        // Vertical gradient base (from deep navy #080C16 to midnight sapphire #111E38)
        int base_r = 8  + (y * 10 / height);
        int base_g = 12 + (y * 22 / height);
        int base_b = 24 + (y * 42 / height);

        for (int x = 0; x < width; x++) {
            // Wave 1: Cyan/Aquamarine glowing ribbon
            int w1_angle = (x * 256 / width) + (y * 64 / height);
            int wave1_y = (height * 6 / 10) + (fast_sin(w1_angle) * (height / 6) / 256);
            int dist1 = y - wave1_y;
            if (dist1 < 0) dist1 = -dist1;

            int glow1 = 0;
            if (dist1 < 160) {
                glow1 = (160 - dist1) * (160 - dist1) / 160; // Quadratic glow falloff
            }

            // Wave 2: Violet/Magenta blooming crest
            int w2_angle = (x * 320 / width) + 90;
            int wave2_y = (height * 48 / 100) + (fast_cos(w2_angle) * (height / 5) / 256);
            int dist2 = y - wave2_y;
            if (dist2 < 0) dist2 = -dist2;

            int glow2 = 0;
            if (dist2 < 140) {
                glow2 = (140 - dist2) * (140 - dist2) / 140;
            }

            // Wave 3: Deep Royal Blue background crest
            int w3_angle = (x * 180 / width) + 180;
            int wave3_y = (height * 70 / 100) + (fast_sin(w3_angle) * (height / 7) / 256);
            int dist3 = y - wave3_y;
            if (dist3 < 0) dist3 = -dist3;

            int glow3 = 0;
            if (dist3 < 180) {
                glow3 = (180 - dist3) * 100 / 180;
            }

            // Combine color channels
            int r = base_r + (glow2 * 140 / 140) + (glow1 * 10 / 160);
            int g = base_g + (glow1 * 190 / 160) + (glow2 * 45 / 140) + (glow3 * 20 / 100);
            int b = base_b + (glow1 * 230 / 160) + (glow2 * 220 / 140) + (glow3 * 160 / 100);

            if (r > 255) r = 255;
            if (g > 255) g = 255;
            if (b > 255) b = 255;

            buffer[y * width + x] = 0xFF000000 | (r << 16) | (g << 8) | b;
        }
    }
}
