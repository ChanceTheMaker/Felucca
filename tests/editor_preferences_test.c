/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#ifndef FEATURES
#define FEATURES 15
#endif
#define FELUCCA_FLASH 1
#if FEATURES & 2
#define FELUCCA_FONT_PREF 1
#endif
#if FEATURES & 4
#define FELUCCA_MONITOR 1
#endif
#if FEATURES & 8
#define FELUCCA_FAVORITES 1
#endif
#define NPALETTES 3u
#define NENGINES 2u
#define UP_SLOTS 32u
enum { ED_UI_STATE = 33, ED_UI_SET, ED_UI_PALETTES, ED_FAV_GET, ED_FAV_SET };
static struct { uint32_t palette, bold, monitor; } settings;
static struct { uint32_t force; } ui;
static uint8_t font_bold, monitor_mode;
static struct { uint32_t valid; } monitor_event;
static uint32_t up_gen, writes, used = 1u << 31, flash_ok = 1, write_fail;
static struct { const char *name; } PALETTES[] = {{"MONO"}, {"LIGHT"}, {"L-HICON"}};
static struct engine { uint32_t npresets; } engines[] = {{128}, {3}};
static struct engine *ENGINES[] = {&engines[0], &engines[1]};
static struct favorites { uint8_t factory[16][32]; uint32_t user, filter; } favorites;
static struct { uint32_t palette, bold, monitor; struct favorites favorites; } persist_saved;
static void palette_set(uint32_t n) { (void)n; }
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static int up_used(uint32_t n) { return !!(used & (1u << n)); }
static int favorite_has(uint32_t e, uint32_t n)
{ return e == NENGINES ? !!(favorites.user & (1u << n)) : !!(favorites.factory[e][n / 8] & (1u << (n % 8))); }
static int favorite_set(uint32_t e, uint32_t n, int on)
{
    int changed = favorite_has(e, n) != on;
    if (e == NENGINES) {
        if (on) favorites.user |= 1u << n; else favorites.user &= ~(1u << n);
    } else {
        if (on) favorites.factory[e][n / 8] |= 1u << (n % 8);
        else favorites.factory[e][n / 8] &= ~(1u << (n % 8));
    }
    return changed;
}
static void settings_save(void)
{
    if (!flash_ok || write_fail) return;
    if (persist_saved.palette == settings.palette && persist_saved.bold == settings.bold &&
        persist_saved.monitor == settings.monitor && !memcmp(&persist_saved.favorites, &favorites, sizeof favorites)) return;
    writes++;
    persist_saved.palette = settings.palette; persist_saved.bold = settings.bold;
    persist_saved.monitor = settings.monitor; persist_saved.favorites = favorites;
}
static uint8_t out[600];
static uint32_t out_n;
static void ed_b(uint32_t n) { assert(n < 128 && out_n < sizeof out); out[out_n++] = n; }
static void ed_v(int32_t n) { uint32_t u = n + 8192; ed_b(u & 127); ed_b((u >> 7) & 127); }
static int32_t ed_rv(const uint8_t *a) { return (int32_t)(a[0] | a[1] << 7) - 8192; }
static void ed_str(const char *s, uint32_t n) { while (*s && n--) ed_b((uint8_t)*s++); ed_b(0); }
#include "../firmware/src/editor_preferences.c"
static void call(uint32_t cmd, const uint8_t *a, uint32_t n)
{ out_n = 0; assert(ed_ui_handle(cmd, a, n)); }
int main(void)
{
    uint8_t a[] = {0, 2, 0, 0};
    assert(ed_ui_caps() == FEATURES);
    call(ED_UI_STATE, a, 0); assert(out_n == 13 && out[0] == FEATURES);
    call(ED_UI_PALETTES, a, 0); assert(out[0] == NPALETTES);
    call(ED_UI_SET, a, 2); assert(out[0] == 0 && settings.palette == 2 && writes == 1);
    call(ED_UI_SET, a, 2); assert(writes == 1); /* idempotent request */
    call(ED_UI_SET, a, 1); assert(out[0] == 1 && writes == 1);
    a[1] = 127; call(ED_UI_SET, a, 2); assert(out[0] == 1 && writes == 1);
    a[1] = 1; write_fail = 1; call(ED_UI_SET, a, 2);
    assert(out[0] == 3 && settings.palette == 1 && persist_saved.palette == 2);
    write_fail = 0; call(ED_UI_SET, a, 2); assert(out[0] == 0 && writes == 2);
    for (uint32_t id = 1; id < 4; id++) {
        a[0] = id; a[1] = 1; call(ED_UI_SET, a, 2);
        assert(out[0] == ((FEATURES & (1u << id)) ? 0 : 2));
    }
#if FEATURES & 8
    a[0] = NENGINES; a[1] = 31; a[2] = 64; a[3] = 1;
    call(ED_FAV_SET, a, 4); assert(out[0] == 0 && favorite_has(NENGINES, 31));
    call(ED_FAV_GET, a, 4); assert(out_n == 6 && out[5] == 1);
    a[1] = 30; call(ED_FAV_SET, a, 4); assert(out[0] == 1); /* empty slot */
    a[0] = 0; a[1] = 127; call(ED_FAV_SET, a, 4); assert(out[0] == 0 && favorite_has(0, 127));
    a[3] = 32; call(ED_FAV_GET, a, 4); assert(out[0] == 1); /* no overrun */
    a[1] = 96; call(ED_FAV_GET, a, 4); assert(out_n == 37 && out[36] == 1);
    call(ED_FAV_SET, a, 3); assert(out[0] == 1);
    a[0] = 127; call(ED_FAV_GET, a, 4); assert(out[0] == 1);
#else
    call(ED_FAV_GET, a, 0); assert(out[0] == 2);
    call(ED_FAV_SET, a, 0); assert(out[0] == 2);
#endif
    puts("Editor preferences: capabilities, bounds, persistence failures, idempotence and favorites passed.");
}
