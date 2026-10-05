/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define __attribute__(x)
#define NENGINES 9u
#define UP_SLOTS 32u
#define NPALETTES 20u
static uint8_t fx_lowcut, font_bold, monitor_mode;
static void fm1_led_key(unsigned k, int on) { (void)k; (void)on; }
static int fm1_enc_take(unsigned k) { (void)k; return 0; }
static void palette_set(unsigned p) { (void)p; }
#include "../firmware/src/panel.c"
static void settings_save(void) {}
#include "../firmware/src/favorites.c"
#include "../firmware/src/settings_persist.c"
int main(void) {
    persist_t p = {0};
    p.magic=PERSIST_MAGIC; p.palette=19; p.bold=p.lowcut=p.zoom=1; p.monitor=2;
    p.panel=PANEL_DEFAULT; p.panel.enc[0]=3;
    favorite_set(8,0,1); favorite_set(NENGINES,31,1); favorites.filter=1;
    memcpy(&p.favorites, &favorites, sizeof favorites);
    memset(&settings,0,sizeof settings); memset(&favorites,0,sizeof favorites);
    assert(settings_import(&p,sizeof p)==1); settings_init();
    assert(settings.palette==19 && font_bold && fx_lowcut && settings.zoom);
    assert(favorite_has(8,0) && favorite_has(NENGINES,31) && favorites.filter);
    assert(panel.enc[0]==3);
    assert(settings.monitor==2 && monitor_mode==2);
    settings_export(&p);
    assert(p.monitor == 2 && p.bold == 1 && p.favorites.user == (1u << 31));
    p.magic=0x50455234u;
    assert(settings_import(&p,sizeof p-sizeof p.monitor)==2);
    assert(!settings.monitor && favorites.user && favorites.filter);
    p.magic=0x50455233u;
    assert(settings_import(&p,sizeof p-sizeof p.monitor-sizeof p.favorites)==2);
    assert(settings.bold==1 && panel.enc[0]==3 && !favorites.user && !favorites.filter);
    p.magic=0x50455232u;
    assert(settings_import(&p,sizeof p-sizeof p.monitor-sizeof p.favorites-sizeof p.bold)==2);
    assert(!settings.bold && settings.zoom && settings.lowcut && settings.palette==19);
    p.magic=0x50455231u;
    memcpy((uint8_t *)&p+8,&PANEL_DEFAULT,sizeof(panel_t));
    assert(settings_import(&p,8+sizeof(panel_t))==2);
    assert(!settings.bold && !settings.zoom && !settings.lowcut && panel.magic==PANEL_MAGIC);
    assert(settings_import(&p,3)==0 && settings_import(&p,-1)==0);
    puts("Settings: favorites/font round trip; PER1/PER2/PER3 migration preserves palette and calibration.");
}
