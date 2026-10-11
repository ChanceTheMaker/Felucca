/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_controls.h"
static void contact(uint8_t *f, unsigned code, unsigned closed)
{
    unsigned bit = 1u << (code & 15u);
    if (closed) f[code >> 4] &= (uint8_t)~bit;
    else f[code >> 4] |= (uint8_t)bit;
}
static void phases(uint8_t *f, unsigned p)
{
    for (unsigned i = 0; i < 8; ++i)
        for (unsigned b = 0; b < 2; ++b)
            contact(f, smk37_encoder_contacts[i][b], (p >> b) & 1u);
}
int main(void)
{
    uint8_t frame[16];
    smk37_controls_t s = {0};
    smk37_control_changes_t c;
    memset(frame, 255, sizeof frame);
    phases(frame, 3); /* Startup in any phase must not invent rotation. */
    c = smk37_controls_tick(&s, frame);
    for (unsigned i = 0; i < 8; ++i) assert(!c.encoder[i]);
    for (unsigned i = 0; i < 18; ++i) {
        contact(frame, smk37_button_contacts[i], 1);
        c = smk37_controls_tick(&s, frame);
        assert(!c.pressed && !c.released);
        contact(frame, smk37_button_contacts[i], 0);
        c = smk37_controls_tick(&s, frame);
        assert(!c.pressed && !c.released);
        contact(frame, smk37_button_contacts[i], 1);
        for (unsigned n = 0; n < 3; ++n) {
            c = smk37_controls_tick(&s, frame);
            assert(c.pressed == (n == 2 ? 1u << i : 0));
        }
        c = smk37_controls_tick(&s, frame);
        assert(!c.pressed && !c.released);
    }
    assert(s.buttons == (1u << 18)-1);
    for (unsigned i = 0; i < 18; ++i) contact(frame, smk37_button_contacts[i], 0);
    for (unsigned n = 0; n < 3; ++n) {
        c = smk37_controls_tick(&s, frame);
        assert(c.released == (n == 2 ? (1u << 18)-1 : 0));
    }
    assert(!s.buttons);
    /* All eight encoders: signs match Pro016 counter6436-counter6440.
     * Physical clockwise labels must also be checked on Test08 hardware. */
    const unsigned forward[] = {2,0,1,3}, reverse[] = {1,0,2,3};
    for (unsigned dir = 0; dir < 2; ++dir)
        for (unsigned n = 0; n < 4; ++n) {
            phases(frame, dir ? reverse[n] : forward[n]);
            c = smk37_controls_tick(&s, frame);
            for (unsigned i = 0; i < 8; ++i) assert(c.encoder[i] == (dir ? 1 : -1));
            c = smk37_controls_tick(&s, frame);
            for (unsigned i = 0; i < 8; ++i) assert(!c.encoder[i]);
        }
    phases(frame, 0); /* Invalid 11 -> 00 skips, then resynchronizes. */
    c = smk37_controls_tick(&s, frame);
    for (unsigned i = 0; i < 8; ++i) assert(!c.encoder[i]);
    phases(frame, 1);
    c = smk37_controls_tick(&s, frame);
    for (unsigned i = 0; i < 8; ++i) assert(c.encoder[i] == -1);
    phases(frame, 0); /* A bounce back cancels the preceding quarter step. */
    c = smk37_controls_tick(&s, frame);
    for (unsigned i = 0; i < 8; ++i) assert(c.encoder[i] == 1);
    puts("SMK-37 controls: 18 buttons, debounce, 8 encoders, reversal and resync passed");
    return 0;
}
