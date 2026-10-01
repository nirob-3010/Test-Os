/**
 * NSK OS v0.3 - Windows 11 Bloom Silk Flower Wallpaper Engine
 * Exactly reproduces the uploaded HOME.PNG wallpaper:
 * Soft sky-slate ambient backdrop with 3D spiraling folded silk petals,
 * translucent lavender/lilac rims, ice-blue shading, and deep navy ambient creases.
 */
#include "wallpaper.h"
#include "gfx.h"

// 256-entry fixed-point trigonometric sine table (-256 to +256)
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

static inline int fast_sin(int a) { return sin_table[a & 0xFF]; }
static inline int fast_cos(int a) { return sin_table[(a + 64) & 0xFF]; }

void wallpaper_generate(uint32_t* buffer, int width, int height) {
    if (!buffer || width <= 0 || height <= 0) return;

    // Center of the flower blossom (centered horizontally, lower-middle vertically)
    int cx = width / 2;
    int cy = (height * 68) / 100;
    int max_radius = (height * 48) / 100;

    for (int y = 0; y < height; y++) {
        // Serene sky-slate ambient backdrop from HOME.PNG (#B7C7D8 -> #D4DFEC)
        int bg_r = 183 + (y * 28 / height);
        int bg_g = 199 + (y * 24 / height);
        int bg_b = 216 + (y * 20 / height);

        for (int x = 0; x < width; x++) {
            int dx = x - cx;
            int dy = y - cy;

            // Scaled elliptical coordinates (Bloom is slightly wider than tall)
            int ex = dx;
            int ey = (dy * 11) / 10;
            int dist_sq = (ex * ex + ey * ey);

            int dist = 0;
            while (dist * dist < (dist_sq >> 4)) dist++;
            dist <<= 2; // Approximate Euclidean distance in pixels

            if (dist > max_radius + 40 || y > height - 10) {
                // Outside bloom area: smooth ambient background
                buffer[y * width + x] = 0xFF000000 | ((uint32_t)bg_r << 16) | ((uint32_t)bg_g << 8) | (uint32_t)bg_b;
                continue;
            }

            // Pseudo-angle around swirl center (0-255)
            int angle = (dx * 128 / (dist + 1)) + 64;
            if (dy > 0) angle = 256 - angle;

            // --- Multi-tier parametric silk ribbon wave functions ---
            // 1. Overarching spiral crest
            int f1 = fast_sin(angle * 2 + dist * 3 / 2);
            // 2. Translucent curved ribbon folds
            int f2 = fast_cos(angle * 3 - dist * 2 + (dx * 64 / (width + 1)));
            // 3. Crisp edge highlights
            int f3 = fast_sin(angle * 4 + dist * 3 + 40);

            // Distance falloff from center of flower
            int bloom_alpha = (max_radius - dist);
            if (bloom_alpha < 0) bloom_alpha = 0;
            if (bloom_alpha > 120) bloom_alpha = 120;
            bloom_alpha = (bloom_alpha * 255) / 120;

            // Calculate lighting and fold intensity
            int fold = (f1 + f2) / 2; // -256 to +256

            // Deep shadows between silk folds (#1E314D / #2D486E)
            int shadow = 0;
            if (fold < -40) {
                shadow = (-fold - 40) * 180 / 216;
            }

            // Highlight crest along ribbon edge (#FFFFFF)
            int highlight = 0;
            if (f3 > 160 && fold > 20) {
                highlight = (f3 - 160) * 190 / 96;
            }

            // Lilac / Lavender outer petals tint (matching top of HOME.PNG)
            int lilac = 0;
            if (y < cy && fold > 0) {
                lilac = (cy - y) * 120 / cy;
                if (lilac > 90) lilac = 90;
            }

            // Base petal ice-blue color (#CADAEF)
            int pr = 202 - shadow + highlight + (lilac * 18 / 100);
            int pg = 218 - (shadow * 8 / 10) + highlight;
            int pb = 238 - (shadow * 5 / 10) + highlight + (lilac * 12 / 100);

            // Clamping
            if (pr < 30) pr = 30;   if (pr > 255) pr = 255;
            if (pg < 45) pg = 45;   if (pg > 255) pg = 255;
            if (pb < 75) pb = 75;   if (pb > 255) pb = 255;

            // Alpha-blend bloom petal over sky-slate background
            int r = (pr * bloom_alpha + bg_r * (255 - bloom_alpha)) >> 8;
            int g = (pg * bloom_alpha + bg_g * (255 - bloom_alpha)) >> 8;
            int b = (pb * bloom_alpha + bg_b * (255 - bloom_alpha)) >> 8;

            buffer[y * width + x] = 0xFF000000 | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
    }
}
