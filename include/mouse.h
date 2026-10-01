/**
 * NSK OS v0.3 - PS/2 Mouse Driver (Phase 3)
 * Handles IRQ 12, 3-byte packets, tracking, and alpha-blended cursor rendering
 */
#ifndef NSK_MOUSE_H
#define NSK_MOUSE_H

#include "types.h"

#define MOUSE_BTN_LEFT   (1 << 0)
#define MOUSE_BTN_RIGHT  (1 << 1)
#define MOUSE_BTN_MIDDLE (1 << 2)

typedef struct {
    int  x;
    int  y;
    int  prev_x;
    int  prev_y;
    uint8_t buttons;
    uint8_t prev_buttons;
    bool moved;
    bool clicked;
    bool released;
} mouse_state_t;

void mouse_init(uint32_t screen_width, uint32_t screen_height);
void mouse_get_state(mouse_state_t* out_state);
void mouse_draw_cursor(int x, int y);
void mouse_restore_background(int x, int y);

#endif /* NSK_MOUSE_H */
