/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define __attribute__(x)
#define NENGINES 9u
#define UP_SLOTS 32u
#define NPALETTES 20u
static uint8_t fx_lowcut, font_bold;
static void fm1_led_key(unsigned k, int on) { (void)k; (void)on; }
static int fm1_enc_take(unsigned k) { (void)k; return 0; }
static void palette_set(unsigned p) { (void)p; }
#include "../firmware/src/panel.c"
static void settings_save(void) {}
#if __has_include("../firmware/src/favorites.c")
#include "../firmware/src/favorites.c"
#endif
#include "../firmware/src/settings_persist.c"
int main(void)
{
    persist_t original = {0}, p;
    original.magic = PERSIST_MAGIC;
    original.palette = 4; original.bold = original.lowcut = original.zoom = 1;
    original.panel = PANEL_DEFAULT; original.panel.enc[0] = 3;
    original.favorites.factory[8][0] = 1;
    original.favorites.user = 1u << 31; original.favorites.filter = 1;
    p = original;
    assert(settings_import(&p, sizeof p) == 1); settings_init();
    assert(settings.palette == 4 && fx_lowcut && settings.zoom && panel.enc[0] == 3);
#ifdef FELUCCA_FONT_PREF
    assert(settings.bold && font_bold);
#endif
#ifdef FELUCCA_FAVORITES
    assert(favorite_has(8, 0) && favorite_has(NENGINES, 31) && favorites.filter);
#endif
    settings.lowcut = 0;
    settings_export(&p);
    assert(!p.lowcut && p.bold == 1 && p.favorites.user == (1u << 31));
    assert(p.favorites.factory[8][0] == 1 && p.favorites.filter == 1);
    assert(p.panel.enc[0] == 3); /* saving one feature preserves the other */
    p = original; p.magic = 0x50455233u;
    assert(settings_import(&p, sizeof p - sizeof p.favorites) == 2);
    assert(p.bold == 1 && !p.favorites.user && !p.favorites.filter);
    settings_export(&p); assert(p.bold == 1); /* favorites-only preserves PER3 font */
    p = original; p.magic = 0x50455232u;
    assert(settings_import(&p, sizeof p - sizeof p.favorites - sizeof p.bold) == 2);
    assert(!p.bold && !p.favorites.user && settings.zoom && panel.enc[0] == 3);
    p = original; p.magic = 0x50455231u;
    memcpy((uint8_t *)&p + 8, &PANEL_DEFAULT, sizeof(panel_t));
    assert(settings_import(&p, 8 + sizeof(panel_t)) == 2);
    assert(!p.bold && !p.zoom && !p.lowcut && !p.favorites.user && panel.magic == PANEL_MAGIC);
    assert(settings_import(&p, 3) == 0 && settings_import(&p, -1) == 0);
    assert(settings_import(&p, sizeof p - 1) == 0);
    puts("Settings: PER1/PER2/PER3 migration, calibration and independent feature preservation passed.");
}
