/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* Small-canvas renderer (no full framebuffer). Draw text/lines into
 * an off-screen strip, then blit it in one DMA transfer. Pixels are stored
 * byte-swapped (the panel takes RGB565 big-endian). */
typedef struct {               /* proportional, see tools/gen_font.py */
    uint8_t h;
    uint8_t pad;               /* bitmap starts this many pixels left of the pen */
    uint8_t first, last;
    const uint8_t *adv;        /* advance per glyph */
    const uint8_t *bw;         /* bitmap width per glyph (starts FONT_PAD left of the pen) */
    const uint16_t *off;       /* byte offset of each glyph */
    const uint8_t *data;
} felucca_font_t;
#include "felucca_font.h"

static uint8_t font_bold;
/* Matching metrics keep all existing labels, columns and large readouts aligned. */
static const felucca_font_t *font_selected(const felucca_font_t *f)
{
    if (font_bold && f == &FONT_S) return &FONT_SB;
    if (font_bold && f == &FONT_L) return &FONT_LB;
    return f;
}

#define CV_MAX (240u * 124u)      /* the graph strip is 240 x 124 */
static uint16_t cv_px[CV_MAX] __attribute__((section(".pool")));
static uint32_t cv_w, cv_h;
static int32_t cv_oy;            /* y offset for graph drawing */

#define RGB(r, g, b) ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))
/* Legacy names denote background and focus, including in light themes. */
static uint16_t theme_bg, theme_focus = 0xFFFFu, cv_bg;
#define C_BLACK theme_bg
#define C_WHITE theme_focus
/* Semantic shades: rules, inactive, labels, secondary text, values.
 * Keep existing palette indices stable for saved settings. */
typedef struct {
    const char *name;
    uint16_t c[5];
    uint16_t bg;
} palette_t;
static const palette_t PALETTES[] = {
    {"GREEN", {RGB(0, 40, 12), RGB(0, 84, 30), RGB(16, 140, 54), RGB(56, 200, 92), RGB(120, 255, 146)}},
    {"AMBER", {RGB(60, 26, 0), RGB(110, 50, 0), RGB(170, 82, 0), RGB(225, 120, 8), RGB(255, 166, 40)}},
    {"CYAN", {RGB(0, 30, 50), RGB(0, 62, 96), RGB(16, 112, 160), RGB(56, 172, 222), RGB(140, 222, 255)}},
    {"RED", {RGB(52, 8, 8), RGB(100, 18, 14), RGB(170, 36, 26), RGB(226, 64, 48), RGB(255, 112, 92)}},
    {"MONO", {RGB(40, 40, 40), RGB(80, 80, 80), RGB(130, 130, 130), RGB(186, 186, 186), RGB(226, 226, 226)}},
    {"VIOLET", {RGB(36, 20, 56), RGB(76, 44, 112), RGB(150, 110, 194), RGB(190, 148, 232), RGB(222, 192, 255)}},
    {"PINK", {RGB(52, 18, 38), RGB(104, 42, 78), RGB(190, 108, 156), RGB(236, 150, 200), RGB(255, 198, 230)}},
    {"ICE", {RGB(20, 36, 48), RGB(48, 76, 96), RGB(116, 166, 192), RGB(168, 210, 230), RGB(216, 242, 255)}},
    {"WARM", {RGB(42, 34, 24), RGB(86, 72, 52), RGB(166, 148, 116), RGB(214, 194, 158), RGB(250, 234, 204)}},
    {"OCEAN", {RGB(16, 40, 52), RGB(48, 88, 112), RGB(116, 204, 232), RGB(232, 166, 92), RGB(255, 206, 132)}},
    {"DUSK", {RGB(40, 24, 52), RGB(80, 52, 104), RGB(184, 144, 224), RGB(220, 184, 100), RGB(255, 224, 148)}},
    {"HI-CON", {RGB(100, 100, 100), RGB(150, 150, 150), RGB(218, 218, 218), RGB(190, 224, 255), RGB(255, 232, 120)}},
    {"LIGHT", {RGB(120, 132, 144), RGB(72, 80, 88), RGB(54, 64, 76), RGB(120, 52, 8), RGB(0, 76, 128)}, RGB(240, 244, 248)},
    {"PAPER", {RGB(140, 128, 104), RGB(88, 76, 60), RGB(66, 56, 46), RGB(120, 54, 24), RGB(32, 78, 70)}, RGB(248, 240, 220)},
    {"SKY", {RGB(100, 132, 156), RGB(48, 76, 96), RGB(28, 56, 84), RGB(112, 48, 24), RGB(0, 64, 128)}, RGB(224, 240, 255)},
    {"MINT", {RGB(96, 140, 116), RGB(40, 80, 60), RGB(24, 64, 48), RGB(88, 48, 112), RGB(0, 84, 64)}, RGB(224, 248, 236)},
    {"LILAC", {RGB(132, 112, 152), RGB(72, 56, 88), RGB(56, 36, 80), RGB(0, 88, 88), RGB(88, 32, 128)}, RGB(244, 232, 255)},
    {"ROSE", {RGB(152, 108, 120), RGB(88, 52, 64), RGB(80, 32, 48), RGB(0, 80, 104), RGB(136, 24, 64)}, RGB(255, 232, 240)},
    {"SAND", {RGB(144, 124, 88), RGB(80, 60, 36), RGB(64, 44, 20), RGB(0, 72, 112), RGB(120, 48, 0)}, RGB(255, 240, 208)},
    {"L-HICON", {RGB(80, 80, 80), RGB(64, 64, 64), RGB(24, 24, 24), RGB(0, 40, 96), RGB(0, 0, 0)}, RGB(255, 255, 255)},
};
#define NPALETTES (sizeof(PALETTES) / sizeof(PALETTES[0]))
static uint16_t pal[5];
#define C_LINE pal[0]                /* 1 rules, separators */
#define C_DIM pal[1]                 /* 2 inactive, empty steps, units */
#define C_GRAY pal[2]                /* 3 labels */
#define C_AMB pal[3]                 /* 4 secondary text */
#define C_HI pal[4]                  /* 5 values, curves */

static void palette_set(uint32_t i)
{
    uint32_t k;
    theme_bg = PALETTES[i % NPALETTES].bg;
    theme_focus = theme_bg ? RGB(8, 16, 24) : 0xFFFFu;
    for (k = 0; k < 5u; k++)
        pal[k] = PALETTES[i % NPALETTES].c[k];
}

/* Track identity and status stay distinct from the selected theme's values. */
static uint16_t track_color(uint32_t track)
{
    static const uint16_t dark[4] = {RGB(100, 210, 255), RGB(255, 186, 90), RGB(210, 152, 255), RGB(100, 232, 160)};
    static const uint16_t light[4] = {RGB(0, 64, 112), RGB(112, 48, 0), RGB(88, 32, 128), RGB(0, 80, 44)};
    return (theme_bg ? light : dark)[track % 4u];
}
#define C_PLAY (theme_bg ? RGB(0, 80, 44) : RGB(100, 232, 160))
#define C_REC (theme_bg ? RGB(144, 16, 32) : RGB(255, 100, 120))
#define C_WARN (theme_bg ? RGB(112, 48, 0) : RGB(255, 194, 80))
#define C_LINK (theme_bg ? RGB(0, 64, 112) : RGB(100, 210, 255))

static inline uint16_t swap16(uint32_t c) { return (uint16_t)(((c >> 8) & 0xFFu) | ((c & 0xFFu) << 8)); }

static void cv_begin(uint32_t w, uint32_t h, uint16_t bg)
{
    uint32_t i, n;
    if (w * h > CV_MAX)
        h = CV_MAX / w;
    lcd_sync();                     /* the last blit may still read cv_px */
    cv_w = w;
    cv_h = h;
    cv_bg = bg;
    n = w * h;
    for (i = 0; i < n; i++)
        cv_px[i] = swap16(bg);
}

static void cv_blit(uint32_t x, uint32_t y) { lcd_blit(x, y, cv_w, cv_h, cv_px); }

/* canvas rows r0 .. cv_h-1 only, to screen row y + r0 */
static void cv_blit_from(uint32_t x, uint32_t y, uint32_t r0)
{
    if (r0 < cv_h)
        lcd_blit(x, y + r0, cv_w, cv_h - r0, cv_px + r0 * cv_w);
}

static inline void cv_pset(int32_t x, int32_t y, uint16_t c)
{
    y += cv_oy;
    if ((uint32_t)x < cv_w && (uint32_t)y < cv_h)
        cv_px[(uint32_t)y * cv_w + (uint32_t)x] = swap16(c);
}

static void cv_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t c)
{
    int32_t x1 = x + w, y1 = y + h + cv_oy, i;
    uint16_t sc = swap16(c);
    y += cv_oy;                             /* clipped once, then filled row by row */
    if (x < 0)
        x = 0;
    if (y < 0)
        y = 0;
    if (x1 > (int32_t)cv_w)
        x1 = (int32_t)cv_w;
    if (y1 > (int32_t)cv_h)
        y1 = (int32_t)cv_h;
    for (; y < y1; y++)
        for (i = x; i < x1; i++)
            cv_px[(uint32_t)y * cv_w + (uint32_t)i] = sc;
}

static void cv_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint16_t c)
{
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int32_t dy = y1 > y0 ? y0 - y1 : y1 - y0, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx + dy, guard = 2000;
    while (guard--) {                       /* bounded: a line is never longer than 480 px */
        int32_t e2 = 2 * err;               /* both tests use the same error value */
        cv_pset(x0, y0, c);
        if (x0 == x1 && y0 == y1)
            break;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* glyph index of a character: lower case folds to upper case when the font
 * has none, anything missing (and C1 controls) draws as '?' */
static uint32_t glyph(const felucca_font_t *f, uint32_t ch)
{
    if (ch >= 'a' && ch <= 'z' && f->last < 'a')
        ch -= 32u;
    if (ch < f->first || ch > f->last || (ch >= 127u && ch < 160u))
        ch = '?';
    return ch - f->first;
}

/* Text blended against the canvas background, including light themes. */
static int32_t cv_text(int32_t x, int32_t y, const felucca_font_t *f, const char *s, uint16_t c)
{
    uint16_t ramp[16];
    uint32_t r = c >> 11, g = (c >> 5) & 63u, b = c & 31u, a;
    uint32_t br = cv_bg >> 11, bg = (cv_bg >> 5) & 63u, bb = cv_bg & 31u;
    f = font_selected(f);
    for (a = 0; a < 16u; a++)
        ramp[a] = (uint16_t)((((r * a + br * (15u - a)) / 15u) << 11) |
                            (((g * a + bg * (15u - a)) / 15u) << 5) |
                            ((b * a + bb * (15u - a)) / 15u));
    for (; *s; s++) {
        uint32_t gi = glyph(f, (uint8_t)*s), gx, gy, w, bpr;
        const uint8_t *gd;
        w = f->bw[gi];
        bpr = (w + 1u) / 2u;
        gd = f->data + f->off[gi];
        for (gy = 0; gy < f->h; gy++)
            for (gx = 0; gx < w; gx++) {
                uint32_t v = gd[gy * bpr + gx / 2u];
                v = (gx & 1u) ? (v & 15u) : (v >> 4);
                if (v)
                    cv_pset(x - f->pad + (int32_t)gx, y + (int32_t)gy, ramp[v]);
            }
        x += f->adv[gi];
    }
    return x;
}

static int32_t text_w(const felucca_font_t *f, const char *s)
{
    int32_t w = 0;
    f = font_selected(f);
    for (; *s; s++)
        w += f->adv[glyph(f, (uint8_t)*s)];
    return w;
}

/* one-shot: text in a box, cleared to black, blitted */
static void draw_text_box(uint32_t x, uint32_t y, uint32_t w, const felucca_font_t *f, const char *s,
                          uint16_t c, int align)
{
    int32_t tw = text_w(f, s), tx = 0;
    cv_begin(w, f->h, C_BLACK);
    if (align == 1)
        tx = ((int32_t)w - tw) / 2;
    else if (align == 2)
        tx = (int32_t)w - tw;
    cv_text(tx, 0, f, s, c);
    cv_blit(x, y);
    lcd_sync();                     /* one-shots (boot, crash, UBOOT, update) finish here */
}
