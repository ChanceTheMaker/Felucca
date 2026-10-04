/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the real canvas/text renderer and write an optional palette sheet. */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#define __attribute__(x)
static void lcd_sync(void) {}
static void lcd_blit(uint32_t x, uint32_t y, uint32_t w, uint32_t h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
#include "../firmware/src/gfx.c"

#define SHEET_W 960u
#define SHEET_H (((NPALETTES + 3u) / 4u) * 124u)
static uint16_t sheet[SHEET_W * SHEET_H];
static unsigned channel(uint16_t c, unsigned n)
{ return n == 0 ? c >> 11 : n == 1 ? (c >> 5) & 63 : c & 31; }
static double luminance(uint16_t color)
{
    const double weight[] = {0.2126, 0.7152, 0.0722};
    double result = 0;
    for (unsigned c = 0; c < 3; c++) {
        double v = channel(color, c) / (c == 1 ? 63.0 : 31.0);
        result += weight[c] * (v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4));
    }
    return result;
}
static double contrast(uint16_t fg, uint16_t bg)
{
    double a = luminance(fg), b = luminance(bg);
    return a > b ? (a + 0.05) / (b + 0.05) : (b + 0.05) / (a + 0.05);
}

int main(int argc, char **argv)
{
    const felucca_font_t *regular[] = {&FONT_S, &FONT_L}, *bold[] = {&FONT_SB, &FONT_LB};
    for (unsigned size = 0; size < 2; size++) {
        assert(regular[size]->h == bold[size]->h);
        assert(regular[size]->first == bold[size]->first && regular[size]->last == bold[size]->last);
        for (unsigned ch = 0; ch <= regular[size]->last - regular[size]->first; ch++)
            assert(regular[size]->adv[ch] == bold[size]->adv[ch]);
    }
    font_bold = argc > 2;
    static const uint8_t adv[] = {3}, bw[] = {3}, data[] = {0x08, 0xf0};
    static const uint16_t off[] = {0};
    const felucca_font_t probe = {1, 0, 'A', 'A', adv, bw, off, data};
    for (unsigned p = 0; p < NPALETTES; p++) {
        palette_set(p);
        if (theme_bg) {
            uint16_t ink[] = {C_DIM, C_GRAY, C_AMB, C_HI, C_WHITE,
                C_PLAY, C_REC, C_WARN, C_LINK,
                track_color(0), track_color(1), track_color(2), track_color(3)};
            double min = 100;
            for (unsigned i = 0; i < sizeof ink / sizeof ink[0]; i++) {
                double ratio = contrast(ink[i], C_BLACK);
                if (ratio < min) min = ratio;
                assert(ratio >= 7.0);
            }
            assert(contrast(C_LINE, C_BLACK) >= 3.0);
            printf("%s: minimum text/accent contrast %.2f:1\n", PALETTES[p].name, min);
        }
        cv_begin(3, 1, C_BLACK);
        cv_text(0, 0, &probe, "A", C_HI);
        assert(swap16(cv_px[0]) == C_BLACK); /* transparent pixel */
        assert(swap16(cv_px[2]) == C_HI);    /* solid pixel */
        for (unsigned c = 0; c < 3; c++) {
            unsigned a = channel(C_BLACK, c), b = channel(C_HI, c);
            unsigned m = channel(swap16(cv_px[1]), c);
            assert(m >= (a < b ? a : b) && m <= (a > b ? a : b));
        } /* catches dark halos on pale backgrounds */
        assert(C_BLACK != C_WHITE);
        assert(text_w(&FONT_S, PALETTES[p].name) <= 70); /* COLOR menu swatch starts at x160 */
        cv_begin(240, 124, C_BLACK);
        cv_text(4, 2, &FONT_S, PALETTES[p].name, C_HI);
        for (unsigned k = 0; k < 5; k++) cv_rect(160 + k * 14, 5, 10, 10, pal[k]);
        cv_text(4, 24, &FONT_S, "LABEL", C_GRAY);
        cv_text(80, 24, &FONT_S, "VALUE", C_HI);
        cv_text(160, 24, &FONT_S, "FOCUS", C_WHITE);
        cv_text(4, 44, &FONT_S, "INACTIVE", C_DIM);
        cv_text(108, 44, &FONT_S, "SECONDARY", C_AMB);
        for (unsigned t = 0; t < 4; t++) {
            char name[] = "TRK1"; name[3] += t;
            cv_text(4 + t * 60, 64, &FONT_S, name, track_color(t));
        }
        cv_text(4, 84, &FONT_S, "PLAY", C_PLAY);
        cv_text(64, 84, &FONT_S, "REC", C_REC);
        cv_text(124, 84, &FONT_S, "LOW", C_WARN);
        cv_text(184, 84, &FONT_S, "USB", C_LINK);
        cv_rect(4, 108, 232, 1, C_LINE);
        for (unsigned y = 0; y < 124; y++)
            for (unsigned x = 0; x < 240; x++)
                sheet[(p / 4 * 124 + y) * 960 + p % 4 * 240 + x] = swap16(cv_px[y * 240 + x]);
    }
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        fprintf(f, "P6\n%u %u\n255\n", (unsigned)SHEET_W, (unsigned)SHEET_H);
        for (unsigned i = 0; i < SHEET_W * SHEET_H; i++) {
            fputc(channel(sheet[i], 0) * 255 / 31, f);
            fputc(channel(sheet[i], 1) * 255 / 63, f);
            fputc(channel(sheet[i], 2) * 255 / 31, f);
        }
        fclose(f);
    }
    puts("Theme text blending, focus contrast, and menu fit passed.");
    return 0;
}
