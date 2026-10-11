/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_analog_watch.h"
int main(void)
{
    smk37_analog_watch_t s = {0};
    for (unsigned i=0; i<SMK37_ANALOG_INPUTS; ++i)
        smk37_analog_watch_update(&s, i, 500);
    assert(s.valid == 0xffffffu && !s.changed && !s.last);
    /* Noise does not move the anchor; slow cumulative movement is detected. */
    for (unsigned v=501; v<516; ++v) smk37_analog_watch_update(&s, 17, v);
    assert(!s.changed && !s.last);
    smk37_analog_watch_update(&s, 17, 516);
    assert(s.last == 18 && s.changed == (1u<<17));
    /* A pad hit and release wholly between LCD frames remains visible. */
    smk37_analog_watch_update(&s, 0, 950);
    smk37_analog_watch_update(&s, 0, 500);
    assert(s.current[0] == 500 && s.low[0] == 500 && s.high[0] == 950);
    assert(s.last == 1 && (s.changed & 1u));
    for (unsigned i=0; i<SMK37_ANALOG_INPUTS; ++i) {
        smk37_analog_watch_update(&s, i, 0);
        smk37_analog_watch_update(&s, i, 1023);
        assert(s.low[i] == 0 && s.high[i] == 1023 && s.last == i+1);
    }
    assert(s.changed == 0xffffffu);
    smk37_analog_watch_update(&s, 24, 1);
    smk37_analog_watch_update(&s, 0, 1024);
    assert(s.current[0] == 1023 && s.last == 24);
    puts("SMK analog activity: baseline, drift, brief hit, all inputs and bounds passed");
}
