/**
 * NSK OS v0.3 - Window Manager & Desktop UI Engine (Phase 3)
 * Exact Reference Match: macOS / Windows 11 Bloom Light Theme
 * Features: Top Menu Bar, Left Desktop Icons, Right System Widget,
 * Centered Floating Frosted Dock, Light-Themed Windows with Traffic Lights.
 */
#include "wm.h"
#include "gfx.h"
#include "font.h"
#include "mouse.h"
#include "keyboard.h"
#include "wallpaper.h"
#include "kheap.h"
#include "pit.h"
#include "printf.h"
#include "string.h"

static window_t  windows[WM_MAX_WINDOWS];
static window_t* z_order[WM_MAX_WINDOWS];
static int       num_windows = 0;

static uint32_t* wallpaper_cache = NULL;
static uint32_t  screen_w = 0;
static uint32_t  screen_h = 0;

static bool      start_menu_open = false;
static window_t* dragging_window = NULL;

void wm_init(void) {
    screen_w = gfx_get_width();
    screen_h = gfx_get_height();

    num_windows = 0;
    dragging_window = NULL;
    start_menu_open = false;

    size_t cache_bytes = screen_w * screen_h * sizeof(uint32_t);
    wallpaper_cache = (uint32_t*)kmalloc_aligned(cache_bytes, 16);

    if (wallpaper_cache) {
        kprintf("[NSK WM] Generating Bloom Light Wallpaper cache (%ux%u)...\n", screen_w, screen_h);
        wallpaper_generate(wallpaper_cache, screen_w, screen_h);
    }

    kprintf("[NSK WM] Window Manager initialized successfully\n");
}

window_t* wm_create_window(const char* title, int x, int y, int w, int h,
                           window_render_fn render_fn, window_click_fn click_fn) {
    if (num_windows >= WM_MAX_WINDOWS) return NULL;

    window_t* win = &windows[num_windows];
    win->id = num_windows + 1;
    strncpy(win->title, title ? title : "Untitled", sizeof(win->title) - 1);
    win->title[sizeof(win->title) - 1] = '\0';

    win->x = x;
    win->y = y;
    win->w = w;
    win->h = h;
    win->orig_x = x;
    win->orig_y = y;
    win->orig_w = w;
    win->orig_h = h;
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
    win->is_maximized = false;
    win->is_minimized = false;
    win->is_closed = false;
    win->is_focused = false;
    win->is_dragging = false;
    win->render_client = render_fn;
    win->on_click = click_fn;
    win->user_data = NULL;

    z_order[num_windows] = win;
    num_windows++;

    wm_focus_window(win);

    kprintf("[NSK WM] Created Window %d: \"%s\" [%d,%d %dx%d]\n", win->id, win->title, x, y, w, h);
    return win;
}

void wm_bring_to_front(window_t* win) {
    if (!win) return;

    int idx = -1;
    for (int i = 0; i < num_windows; i++) {
        if (z_order[i] == win) {
            idx = i;
            break;
        }
    }

    if (idx != -1 && idx < num_windows - 1) {
        for (int i = idx; i < num_windows - 1; i++) {
            z_order[i] = z_order[i + 1];
        }
        z_order[num_windows - 1] = win;
    }
}

void wm_focus_window(window_t* win) {
    for (int i = 0; i < num_windows; i++) {
        windows[i].is_focused = false;
    }
    if (win) {
        win->is_focused = true;
        win->is_minimized = false;
        wm_bring_to_front(win);
    }
}

void wm_close_window(window_t* win) {
    if (win) {
        win->is_closed = true;
        win->is_focused = false;
        if (dragging_window == win) dragging_window = NULL;
    }
}

void wm_minimize_window(window_t* win) {
    if (win) {
        win->is_minimized = true;
        win->is_focused = false;
        if (dragging_window == win) dragging_window = NULL;
    }
}

void wm_restore_window(window_t* win) {
    if (win) {
        win->is_minimized = false;
        wm_focus_window(win);
    }
}

bool wm_is_start_menu_open(void) {
    return start_menu_open;
}

void wm_toggle_start_menu(void) {
    start_menu_open = !start_menu_open;
}

// -----------------------------------------------------------------------------
// Desktop Decoration Rendering (Top Menu Bar, Desktop Icons, Widget, Dock)
// -----------------------------------------------------------------------------

static void wm_render_topbar(void) {
    // 28px macOS-style top menu bar
    int bar_h = 26;
    gfx_fill_rect(0, 0, screen_w, bar_h, 0xC0F8FAFC); // Translucent frosted white
    gfx_draw_line(0, bar_h - 1, screen_w, bar_h - 1, 0x30CBD5E1);

    // Left: Apple / OS Icon & Brand
    gfx_fill_rounded_rect_aa(12, 6, 14, 14, 7, 0xFF1E293B);
    font_draw_string(32, 6, "NSK OS", 0xFF0F172A, 1);

    // Center: Date & Time
    uint32_t uptime_sec = pit_get_uptime_seconds();
    uint32_t mins  = (uptime_sec / 60) % 60;
    uint32_t hours = ((uptime_sec / 3600) + 20) % 24; // Default starting at 20:xx as in reference

    char time_buf[48];
    snprintf(time_buf, sizeof(time_buf), "Tue, 30 Sep 2026   %02u:%02u", hours, mins);
    int center_x = ((int)screen_w - 180) / 2;
    font_draw_string(center_x, 6, time_buf, 0xFF1E293B, 1);

    // Right: Status tray icons (Display, WiFi, Audio, Battery 85%, Search)
    int rx = (int)screen_w - 130;
    // WiFi symbol representation
    font_draw_string(rx, 6, "(.)", 0xFF475569, 1);
    // Audio representation
    font_draw_string(rx + 24, 6, "<)", 0xFF475569, 1);
    // Battery pill
    gfx_draw_rounded_rect_aa(rx + 48, 6, 22, 12, 3, 0xFF475569);
    gfx_fill_rounded_rect_aa(rx + 50, 8, 15, 8, 2, 0xFF10B981); // 85% green
    font_draw_string(rx + 74, 6, "85%", 0xFF334155, 1);
}

static void wm_render_desktop_icons(void) {
    const char* names[] = { "Home", "Documents", "Pictures", "Music", "Trash" };
    int start_y = 44;
    int spacing_y = 68;

    for (int i = 0; i < 5; i++) {
        int ix = 24;
        int iy = start_y + (i * spacing_y);

        // Fluent Azure Icon body
        if (i == 4) {
            // Trash icon
            gfx_fill_rounded_rect_aa(ix + 2, iy, 34, 30, 8, 0xFFE2E8F0);
            gfx_draw_rounded_rect_aa(ix + 2, iy, 34, 30, 8, 0xFF94A3B8);
            font_draw_string(ix + 14, iy + 7, "[x]", 0xFF3B82F6, 1);
        } else {
            // Blue Folder / Item Icon
            gfx_fill_rounded_rect_aa(ix, iy, 38, 30, 8, 0xFF38BDF8); // Fluent Sky-Blue
            gfx_fill_rounded_rect_aa(ix + 2, iy + 4, 34, 24, 6, 0xFF0284C7); // Inner azure fold
            gfx_fill_rounded_rect_aa(ix + 6, iy + 2, 14, 6, 3, 0xFF38BDF8); // Folder tab
        }

        // Label with shadow below icon
        font_draw_string_shadow(ix - 2, iy + 34, names[i], 0xFF0F172A, 0x40FFFFFF, 1);
    }
}

static void wm_render_system_widget(void) {
    // Top-right floating translucent frosted widget card
    int ww = 144;
    int wh = 176;
    int wx = (int)screen_w - ww - 18;
    int wy = 42;
    int r = 16;

    // Card frosted backing
    gfx_draw_drop_shadow(wx, wy, ww, wh, r, 12, 0x1A000000);
    gfx_box_blur_rect(wx, wy, ww, wh, 10);
    gfx_fill_rounded_rect_aa(wx, wy, ww, wh, r, 0xC4FFFFFF);
    gfx_draw_rounded_rect_aa(wx, wy, ww, wh, r, 0x60FFFFFF);

    // Big digital clock
    uint32_t uptime_sec = pit_get_uptime_seconds();
    uint32_t mins  = (uptime_sec / 60) % 60;
    uint32_t hours = ((uptime_sec / 3600) + 20) % 24;

    char clock_str[16];
    snprintf(clock_str, sizeof(clock_str), "%02u:%02u", hours, mins);
    font_draw_string(wx + 14, wy + 12, clock_str, 0xFF0F172A, 2);
    font_draw_string(wx + 14, wy + 38, "Tue, 30 Sep 2026", 0xFF64748B, 1);

    gfx_draw_line(wx + 12, wy + 54, wx + ww - 12, wy + 54, 0x25CBD5E1);

    // Meters: CPU, RAM, Disk
    int meter_y = wy + 64;

    // CPU Meter
    font_draw_string(wx + 14, meter_y, "CPU", 0xFF334155, 1);
    font_draw_string(wx + ww - 32, meter_y, "6%", 0xFF64748B, 1);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, ww - 28, 6, 3, 0xFFE2E8F0);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, (ww - 28) * 6 / 100, 6, 3, 0xFF3B82F6);
    meter_y += 32;

    // RAM Meter
    font_draw_string(wx + 14, meter_y, "RAM", 0xFF334155, 1);
    font_draw_string(wx + ww - 38, meter_y, "85%", 0xFF64748B, 1);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, ww - 28, 6, 3, 0xFFE2E8F0);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, (ww - 28) * 85 / 100, 6, 3, 0xFF2563EB);
    meter_y += 32;

    // Disk Meter
    font_draw_string(wx + 14, meter_y, "Disk", 0xFF334155, 1);
    font_draw_string(wx + ww - 38, meter_y, "12%", 0xFF64748B, 1);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, ww - 28, 6, 3, 0xFFE2E8F0);
    gfx_fill_rounded_rect_aa(wx + 14, meter_y + 14, (ww - 28) * 12 / 100, 6, 3, 0xFF0284C7);
}

static void wm_render_dock(void) {
    // Centered floating frosted dock (macOS / Windows 11 centered style)
    int dock_w = 400;
    int dock_h = 58;
    int dock_x = ((int)screen_w - dock_w) / 2;
    int dock_y = (int)screen_h - dock_h - 16;
    int r = 22;

    // Frosted acrylic glass body with heavy blur & shadow
    gfx_draw_drop_shadow(dock_x, dock_y, dock_w, dock_h, r, 16, 0x22000000);
    gfx_box_blur_rect(dock_x, dock_y, dock_w, dock_h, 14);
    gfx_fill_rounded_rect_aa(dock_x, dock_y, dock_w, dock_h, r, 0xC0FFFFFF);
    gfx_draw_rounded_rect_aa(dock_x, dock_y, dock_w, dock_h, r, 0x80FFFFFF);

    // 8 Modern Fluent App Icons
    // 1: Finder/Files, 2: Browser, 3: Folder, 4: Photos, 5: Music, 6: Terminal, 7: Settings, 8: Trash
    int icon_size = 38;
    int gap = 11;
    int cur_x = dock_x + 14;
    int icon_y = dock_y + 8;

    // 1. Finder (Smiling dual face)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFF38BDF8);
    gfx_fill_rounded_rect_aa(cur_x + 19, icon_y, 19, icon_size, 10, 0xFF0284C7);
    font_draw_string(cur_x + 11, icon_y + 10, "^v^", 0xFFFFFFFF, 1);
    // Active dot under Finder
    gfx_fill_rounded_rect_aa(cur_x + 16, dock_y + dock_h - 6, 5, 5, 2, 0xFF0F172A);
    cur_x += icon_size + gap;

    // 2. Web Browser (Edge / Safari circle)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFF0284C7);
    gfx_fill_rounded_rect_aa(cur_x + 6, icon_y + 6, 26, 26, 13, 0xFF38BDF8);
    gfx_fill_rounded_rect_aa(cur_x + 12, icon_y + 12, 14, 14, 7, 0xFF10B981);
    cur_x += icon_size + gap;

    // 3. Azure Folder
    gfx_fill_rounded_rect_aa(cur_x, icon_y + 3, icon_size, 32, 8, 0xFF38BDF8);
    gfx_fill_rounded_rect_aa(cur_x + 3, icon_y + 7, icon_size - 6, 25, 6, 0xFF0284C7);
    // Active dot
    gfx_fill_rounded_rect_aa(cur_x + 16, dock_y + dock_h - 6, 5, 5, 2, 0xFF0F172A);
    cur_x += icon_size + gap;

    // 4. Photos (Flower petals)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFFFFFFFF);
    gfx_draw_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFFE2E8F0);
    gfx_fill_rounded_rect_aa(cur_x + 10, icon_y + 10, 8, 8, 4, 0xFFEF4444);
    gfx_fill_rounded_rect_aa(cur_x + 20, icon_y + 10, 8, 8, 4, 0xFFF59E0B);
    gfx_fill_rounded_rect_aa(cur_x + 10, icon_y + 20, 8, 8, 4, 0xFF3B82F6);
    gfx_fill_rounded_rect_aa(cur_x + 20, icon_y + 20, 8, 8, 4, 0xFF10B981);
    cur_x += icon_size + gap;

    // 5. Music (Red rounded tile)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFFEF4444);
    font_draw_string(cur_x + 14, icon_y + 10, "~#", 0xFFFFFFFF, 1);
    cur_x += icon_size + gap;

    // 6. Terminal (Dark tile with >_)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFF1E293B);
    font_draw_string(cur_x + 9, icon_y + 10, ">_", 0xFFFFFFFF, 1);
    // Active dot under Terminal
    gfx_fill_rounded_rect_aa(cur_x + 16, dock_y + dock_h - 6, 5, 5, 2, 0xFF0F172A);
    cur_x += icon_size + gap;

    // 7. Settings (Slate gear)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFF64748B);
    font_draw_string(cur_x + 13, icon_y + 10, "@*", 0xFFFFFFFF, 1);
    cur_x += icon_size + gap;

    // 8. Trash (Translucent bin)
    gfx_fill_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFFF1F5F9);
    gfx_draw_rounded_rect_aa(cur_x, icon_y, icon_size, icon_size, 10, 0xFFCBD5E1);
    font_draw_string(cur_x + 11, icon_y + 10, "[x]", 0xFF64748B, 1);
}

// -----------------------------------------------------------------------------
// Window Rendering (Crisp Light Theme matching reference)
// -----------------------------------------------------------------------------

static void wm_render_window(window_t* win) {
    if (!win || win->is_closed || win->is_minimized) return;

    int wx = win->x;
    int wy = win->y;
    int ww = win->w;
    int wh = win->h;
    int radius = 16;

    // 1. Soft Light-Theme Diffused Drop Shadow
    int shadow_size = win->is_focused ? 20 : 12;
    uint32_t shadow_col = win->is_focused ? 0x2C000000 : 0x18000000;
    gfx_draw_drop_shadow(wx, wy, ww, wh, radius, shadow_size, shadow_col);

    // 2. Pure Crisp White Body with Subtle Frosted Acrylic Sheen
    gfx_box_blur_rect(wx, wy, ww, wh, 8);
    gfx_fill_rounded_rect_aa(wx, wy, ww, wh, radius, 0xF6FFFFFF); // 96% White

    // Subtle 1px boundary
    uint32_t border_col = win->is_focused ? 0x6094A3B8 : 0x30CBD5E1;
    gfx_draw_rounded_rect_aa(wx, wy, ww, wh, radius, border_col);

    // 3. Titlebar Header Separator Line
    gfx_draw_line(wx + 8, wy + WM_TITLEBAR_HEIGHT, wx + ww - 8, wy + WM_TITLEBAR_HEIGHT, 0x20CBD5E1);

    // 4. macOS Traffic Light Dots on Left (Red, Amber, Green)
    int btn_y = wy + 11;
    int btn_r = 6;

    // Red: Close
    gfx_fill_rounded_rect_aa(wx + 14, btn_y, 12, 12, btn_r, 0xFFEF4444);
    gfx_draw_rounded_rect_aa(wx + 14, btn_y, 12, 12, btn_r, 0x40000000);

    // Amber: Minimize
    gfx_fill_rounded_rect_aa(wx + 32, btn_y, 12, 12, btn_r, 0xFFF59E0B);
    gfx_draw_rounded_rect_aa(wx + 32, btn_y, 12, 12, btn_r, 0x40000000);

    // Green: Maximize
    gfx_fill_rounded_rect_aa(wx + 50, btn_y, 12, 12, btn_r, 0xFF10B981);
    gfx_draw_rounded_rect_aa(wx + 50, btn_y, 12, 12, btn_r, 0x40000000);

    // Window Title
    font_draw_string(wx + 72, wy + 9, win->title, 0xFF1E293B, 1);

    // Windows controls on right: _ [] x
    font_draw_string(wx + ww - 58, wy + 9, "_  []  x", 0xFF94A3B8, 1);

    // 5. Client Content Rendering
    int client_x = wx + 8;
    int client_y = wy + WM_TITLEBAR_HEIGHT + 2;
    int client_w = ww - 16;
    int client_h = wh - WM_TITLEBAR_HEIGHT - 10;

    if (win->render_client) {
        win->render_client(win, client_x, client_y, client_w, client_h);
    }
}

// -----------------------------------------------------------------------------
// Main Render Pass & Event Loop
// -----------------------------------------------------------------------------

void wm_render(void) {
    uint32_t* backbuffer = gfx_get_backbuffer();
    if (!backbuffer) return;

    // 1. Fast Background Restore from Light Bloom Wallpaper Cache
    if (wallpaper_cache) {
        memcpy(backbuffer, wallpaper_cache, screen_w * screen_h * sizeof(uint32_t));
    } else {
        gfx_clear(0xFFCADEEF);
    }

    // 2. Render Top Menu Bar
    wm_render_topbar();

    // 3. Render Desktop Icons (Left column)
    wm_render_desktop_icons();

    // 4. Render System Resource Widget (Top Right)
    wm_render_system_widget();

    // 5. Render Windows sorted by Z-Index (Lowest to Highest)
    for (int i = 0; i < num_windows; i++) {
        window_t* win = z_order[i];
        if (win && !win->is_closed && !win->is_minimized) {
            wm_render_window(win);
        }
    }

    // 6. Render Bottom Centered Floating Dock
    wm_render_dock();

    // 7. Render High-Contrast Retina Mouse Pointer on Top
    mouse_state_t ms;
    mouse_get_state(&ms);
    mouse_draw_cursor(ms.x, ms.y);

    // 8. Presentation Swap
    gfx_swap();
}

void wm_process_events(void) {
    mouse_state_t ms;
    mouse_get_state(&ms);

    // 1. Handle Active Window Dragging
    if (dragging_window) {
        if (ms.buttons & MOUSE_BTN_LEFT) {
            dragging_window->x = ms.x - dragging_window->drag_offset_x;
            dragging_window->y = ms.y - dragging_window->drag_offset_y;

            // Clamping
            if (dragging_window->x < 0) dragging_window->x = 0;
            if (dragging_window->y < 26) dragging_window->y = 26; // Below top menu bar
            if (dragging_window->x + dragging_window->w > (int)screen_w)
                dragging_window->x = (int)screen_w - dragging_window->w;
            if (dragging_window->y + dragging_window->h > (int)screen_h - 70)
                dragging_window->y = (int)screen_h - 70 - dragging_window->h;
        } else {
            dragging_window->is_dragging = false;
            dragging_window = NULL;
        }
    }

    // 2. Handle Mouse Left Click
    if (ms.clicked) {
        int mx = ms.x;
        int my = ms.y;

        // Check Windows (Topmost Z to Lowest Z)
        for (int i = num_windows - 1; i >= 0; i--) {
            window_t* win = z_order[i];
            if (!win || win->is_closed || win->is_minimized) continue;

            if (mx >= win->x && mx <= win->x + win->w &&
                my >= win->y && my <= win->y + win->h) {

                wm_focus_window(win);

                // Check Traffic Light Buttons
                if (my >= win->y + 8 && my <= win->y + 24) {
                    // Close button (Red)
                    if (mx >= win->x + 12 && mx <= win->x + 28) {
                        wm_close_window(win);
                        return;
                    }
                    // Minimize button (Amber)
                    if (mx >= win->x + 30 && mx <= win->x + 46) {
                        wm_minimize_window(win);
                        return;
                    }
                    // Maximize button (Green)
                    if (mx >= win->x + 48 && mx <= win->x + 64) {
                        if (win->is_maximized) {
                            win->x = win->orig_x;
                            win->y = win->orig_y;
                            win->w = win->orig_w;
                            win->h = win->orig_h;
                            win->is_maximized = false;
                        } else {
                            win->orig_x = win->x;
                            win->orig_y = win->y;
                            win->orig_w = win->w;
                            win->orig_h = win->h;
                            win->x = 20;
                            win->y = 36;
                            win->w = (int)screen_w - 40;
                            win->h = (int)screen_h - 110;
                            win->is_maximized = true;
                        }
                        return;
                    }
                }

                // Check Titlebar Drag
                if (my < win->y + WM_TITLEBAR_HEIGHT) {
                    win->is_dragging = true;
                    win->drag_offset_x = mx - win->x;
                    win->drag_offset_y = my - win->y;
                    dragging_window = win;
                    return;
                }

                // Check Client Area Click
                if (win->on_click) {
                    win->on_click(win, mx - win->x, my - win->y);
                }
                return;
            }
        }
    }
}
