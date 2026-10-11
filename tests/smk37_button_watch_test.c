/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_button_watch.h"
int main(void)
{
    smk37_button_watch_t s={0};
    smk37_control_changes_t c={0};
    for (unsigned i=0; i<18; ++i) {
        c.pressed=1u<<i; c.released=0;
        smk37_button_watch_update(&s,&c);
        assert(s.held==(1u<<i) && s.down && s.last_press==i+1);
        c.pressed=0; c.released=1u<<i;
        smk37_button_watch_update(&s,&c);
        assert(!s.held && !s.down && s.last_press==i+1 && s.last_edge==i+1);
        assert(s.presses==i+1 && s.releases==i+1);
    }
    assert(s.seen==0x3ffff);
    c.pressed=0x20001; c.released=0;
    smk37_button_watch_update(&s,&c);
    assert(s.held==0x20001 && s.presses==20);
    c.pressed=0; c.released=0;
    smk37_button_watch_update(&s,&c);
    assert(s.held==0x20001 && s.presses==20);
    puts("SMK buttons: all IDs, press/release counts, sticky history and simultaneous holds passed");
}
