/* SPDX-License-Identifier: GPL-3.0-only */
/* Render the actual menu, exercise font switching and save-on-exit. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define __attribute__(x)
static uint16_t screen[240 * 240], sheet[480 * 480];
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{
    assert(x + w <= 240 && y + h <= 240);
    for (unsigned j = 0; j < h; j++)
        for (unsigned i = 0; i < w; i++) {
            uint16_t v = p[j * w + i];
            screen[(y + j) * 240 + x + i] = (v >> 8) | (v << 8);
        }
}
static void lcd_fill(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint16_t c)
{
    assert(x + w <= 240 && y + h <= 240);
    for (unsigned j = 0; j < h; j++)
        for (unsigned i = 0; i < w; i++) screen[(y+j)*240+x+i] = c;
}
#include "../firmware/src/gfx.c"
#define H_HEAD 20
#define Y_HEAD 0
#define FELUCCA_VERSION "FONT TEST"
enum { B_OCTUP, B_OCTDN, EN_PRESET = 0, EN_K1 = 1, NE = 2 };
static struct { uint32_t menu, menu_sel, menu_sig, force; } ui;
static struct { uint32_t palette, lowcut, zoom, bold, monitor; } settings;
#define NTRK 4u
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
#include "../firmware/src/monitor.c"
static struct { uint8_t btn[2]; } panel = {{0, 1}};
static uint8_t fx_lowcut;
static unsigned saves;
static int32_t knob[NE];
static int32_t panel_enc(unsigned n) { int32_t v = knob[n]; knob[n] = 0; return v; }
static void settings_save(void) { saves++; }
static void go_home(void) {}
static void panel_setup(void) {}
#include "../firmware/src/ui_menu.c"
int main(int argc, char **argv)
{
    settings.lowcut = settings.zoom = 1;
    for (unsigned tile = 0; tile < 4; tile++) {
        settings.palette = tile < 2 ? 11 : NPALETTES - 1;
        palette_set(settings.palette);
        ui.menu = 1; ui.menu_sel = MI_FONT; ui.force = 1;
        knob[EN_K1] = tile % 2 ? 1 : -1;
        menu_input(0);
        assert(settings.bold == tile % 2 && font_bold == tile % 2);
        assert(settings.lowcut == 1 && settings.zoom == 1 && saves == 0);
        draw_menu();
        for (unsigned y = 0; y < 240; y++)
            for (unsigned x = 0; x < 240; x++)
                sheet[(tile / 2 * 240 + y) * 480 + tile % 2 * 240 + x] = screen[y * 240 + x];
    }
    menu_input(1u << B_OCTUP);
    assert(!font_bold && !settings.bold && ui.force);
    menu_input(1u << B_OCTDN);
    assert(ui.menu == 0 && saves == 1);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        fprintf(f, "P6\n480 480\n255\n");
        for (unsigned i = 0; i < 480 * 480; i++) {
            unsigned v = sheet[i];
            fputc((v >> 11) * 255 / 31, f);
            fputc(((v >> 5) & 63) * 255 / 63, f);
            fputc((v & 31) * 255 / 31, f);
        }
        fclose(f);
    }
    puts("Menu: font preview, redraw, other settings preserved, save-on-exit passed.");
}
