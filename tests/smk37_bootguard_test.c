/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_bootguard.h"
int main(void)
{
    smk37_bootguard_t state = {0};
    assert(!smk37_bootguard_begin(&state, 0));
    assert(state.pending && state.failures == 0);
    state.stage = SMK37_BOOT_DISPLAY;
    assert(!smk37_bootguard_begin(&state, 0));
    assert(state.failures == 1 && state.previous_stage == SMK37_BOOT_DISPLAY);
    state.stage = SMK37_BOOT_AUDIO;
    assert(smk37_bootguard_begin(&state, 0));
    assert(state.failures == 2 && state.previous_stage == SMK37_BOOT_AUDIO);
    for (unsigned i = 0; i < 10; ++i) assert(smk37_bootguard_begin(&state, 0));
    assert(!smk37_bootguard_begin(&state, 1)); /* cold power-on */
    assert(state.failures == 0);
    smk37_bootguard_healthy(&state);
    assert(!smk37_bootguard_begin(&state, 0));
    assert(state.failures == 0 && state.previous_stage == SMK37_BOOT_RUNNING);
    smk37_bootguard_update(&state);
    assert(!smk37_bootguard_begin(&state, 0) && !state.failures);
    state.inverse = 0;
    state.failures = 2;
    assert(!smk37_bootguard_begin(&state, 0));
    state.failures = 100;
    assert(!smk37_bootguard_begin(&state, 0));
    state.pending = 17;
    assert(!smk37_bootguard_begin(&state, 0));
    state.stage = 0xffffffffu;
    assert(!smk37_bootguard_begin(&state, 0));
    puts("SMK37 boot guard: failed retries, fallback latch, healthy boot and corrupt RAM passed");
    return 0;
}
