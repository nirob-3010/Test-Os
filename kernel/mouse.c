/**
 * NSK OS v0.3 - PS/2 Mouse Driver (Phase 3)
 * Full 8042 controller auxiliary device driver, IRQ 12 packet handler,
 * and alpha-blended anti-aliased cursor rendering.
 */
#include "mouse.h"
#include "idt.h"
#include "pic.h"
#include "io.h"
#include "gfx.h"
#include "printf.h"

static mouse_state_t mouse_state;
static uint32_t bound_width = 1024;
static uint32_t bound_height = 768;

static uint8_t mouse_cycle = 0;
static int8_t  mouse_bytes[3];

// 18x26 High-Contrast Retina-style pointer with black outline and drop shadow
// ' ' = transparent, '#' = black border, '.' = white fill, 'S' = soft shadow
static const char* cursor_bitmap[26] = {
    "##                ",
    "###               ",
    "#.##              ",
    "#..##             ",
    "#...##            ",
    "#....##           ",
    "#.....##          ",
    "#......##         ",
    "#.......##        ",
    "#........##       ",
    "#.........##      ",
    "#..........##     ",
    "#...........##    ",
    "#............##   ",
    "#........######## ",
    "#....##...##SSSS  ",
    "#...####...##SSS  ",
    "##.##  ##...##SS  ",
    "###     ##...##S  ",
    "##       ##...##  ",
    "#         ##...## ",
    "           ##...##",
    "            ##..##",
    "             #### ",
    "              ##S ",
    "               S  "
};

static inline void mouse_wait_write(void) {
    uint32_t timeout = 100000;
    while ((inb(0x64) & 0x02) && --timeout);
}

static inline void mouse_wait_read(void) {
    uint32_t timeout = 100000;
    while (!(inb(0x64) & 0x01) && --timeout);
}

static void mouse_write(uint8_t write) {
    mouse_wait_write();
    outb(0x64, 0xD4); // Tell 8042 to send next byte to mouse
    mouse_wait_write();
    outb(0x60, write);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(0x60);
}

static void mouse_interrupt_handler(registers_t* regs) {
    (void)regs;
    uint8_t status = inb(0x64);
    if (!(status & 0x01)) return; // No data

    int8_t mouse_in = (int8_t)inb(0x60);

    // Synchronize packet stream: byte 0 must have bit 3 set
    if (mouse_cycle == 0) {
        if ((mouse_in & 0x08) == 0) {
            return; // Out of sync, wait for header
        }
        mouse_bytes[0] = mouse_in;
        mouse_cycle++;
    } else if (mouse_cycle == 1) {
        mouse_bytes[1] = mouse_in;
        mouse_cycle++;
    } else if (mouse_cycle == 2) {
        mouse_bytes[2] = mouse_in;
        mouse_cycle = 0;

        uint8_t flags = (uint8_t)mouse_bytes[0];
        int dx = (int)mouse_bytes[1];
        int dy = (int)mouse_bytes[2];

        // Sign extension
        if (flags & 0x10) dx |= 0xFFFFFF00;
        if (flags & 0x20) dy |= 0xFFFFFF00;

        // Discard overflow packets
        if (flags & 0xC0) return;

        mouse_state.prev_x = mouse_state.x;
        mouse_state.prev_y = mouse_state.y;
        mouse_state.prev_buttons = mouse_state.buttons;

        mouse_state.x += dx;
        mouse_state.y -= dy; // Invert Y delta (PS/2 points upwards)

        // Clamp to screen bounds
        if (mouse_state.x < 0) mouse_state.x = 0;
        if (mouse_state.y < 0) mouse_state.y = 0;
        if (mouse_state.x >= (int)bound_width) mouse_state.x = (int)bound_width - 1;
        if (mouse_state.y >= (int)bound_height) mouse_state.y = (int)bound_height - 1;

        mouse_state.buttons = flags & 0x07;
        mouse_state.moved = (mouse_state.x != mouse_state.prev_x || mouse_state.y != mouse_state.prev_y);
        mouse_state.clicked = ((mouse_state.buttons & MOUSE_BTN_LEFT) && !(mouse_state.prev_buttons & MOUSE_BTN_LEFT));
        mouse_state.released = (!(mouse_state.buttons & MOUSE_BTN_LEFT) && (mouse_state.prev_buttons & MOUSE_BTN_LEFT));
    }
}

void mouse_init(uint32_t screen_width, uint32_t screen_height) {
    bound_width = screen_width ? screen_width : 1024;
    bound_height = screen_height ? screen_height : 768;

    mouse_state.x = (int)bound_width * 46 / 100;
    mouse_state.y = (int)bound_height * 42 / 100;
    mouse_state.prev_x = mouse_state.x;
    mouse_state.prev_y = mouse_state.y;
    mouse_state.buttons = 0;
    mouse_state.prev_buttons = 0;
    mouse_state.moved = false;
    mouse_state.clicked = false;
    mouse_state.released = false;
    mouse_cycle = 0;

    // Enable auxiliary mouse device on 8042 controller
    mouse_wait_write();
    outb(0x64, 0xA8);

    // Read controller command byte
    mouse_wait_write();
    outb(0x64, 0x20);
    uint8_t status = mouse_read();

    // Enable IRQ 12 and enable mouse clock
    status |= 0x02;   // Enable mouse interrupt (bit 1)
    status &= ~0x20;  // Disable mouse clock inhibit (bit 5)

    mouse_wait_write();
    outb(0x64, 0x60);
    mouse_wait_write();
    outb(0x60, status);

    // Tell mouse to use default settings
    mouse_write(0xF6);
    mouse_read(); // ACK

    // Enable data packet streaming
    mouse_write(0xF4);
    mouse_read(); // ACK

    // Register IRQ 12 handler (Interrupt 44 = 32 + 12)
    register_interrupt_handler(44, mouse_interrupt_handler);

    // Unmask IRQ 2 (cascade) and IRQ 12 (PS/2 mouse) in PIC
    pic_unmask_irq(2);
    pic_unmask_irq(12);

    kprintf("[NSK MOUSE] PS/2 Mouse driver initialized (IRQ 12 active, Pos: [%d, %d])\n",
            mouse_state.x, mouse_state.y);
}

void mouse_get_state(mouse_state_t* out_state) {
    if (out_state) {
        *out_state = mouse_state;
    }
}

void mouse_draw_cursor(int x, int y) {
    for (int r = 0; r < 26; r++) {
        const char* row = cursor_bitmap[r];
        for (int c = 0; c < 18; c++) {
            char p = row[c];
            if (p == ' ' || p == '\0') continue;

            int px = x + c;
            int py = y + r;

            if (p == '.') {
                gfx_put_pixel(px, py, 0xFFFFFFFF); // High-contrast solid white body
            } else if (p == '#') {
                gfx_put_pixel(px, py, 0xFF000000); // 2px crisp solid black boundary
            } else if (p == 'S') {
                gfx_blend_pixel(px, py, 0x70000000); // Prominent drop shadow
            }
        }
    }
}
