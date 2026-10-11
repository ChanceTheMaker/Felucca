/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_analog_controls.h"
int main(void)
{
    unsigned seen=0;
    for (unsigned i=0; i<16; ++i) {
        assert(smk37_pad_input[i]<16 && !(seen & (1u<<smk37_pad_input[i])));
        seen |= 1u<<smk37_pad_input[i];
    }
    assert(seen==0xffffu);
    int bend=-8192, mod=127, fader=0;
    for (unsigned raw=0; raw<=1023; ++raw) {
        int b=smk37_pitch_value(raw), m=smk37_mod_value(raw), f=smk37_fader_value(raw);
        assert(b>=bend && b<=8191 && b>=-8192);
        assert(m<=mod && m>=0 && m<=127);
        assert(f>=fader && f<=127);
        bend=b; mod=m; fader=f;
    }
    assert(smk37_pitch_value(404)==-8192 && smk37_pitch_value(639)==8191);
    for (unsigned i=524; i<=532; ++i) assert(!smk37_pitch_value(i));
    assert(smk37_mod_value(630)==0 && smk37_mod_value(411)==127);
    for (unsigned pad=0; pad<16; ++pad) {
        smk37_pad_state_t state={0};
        for (unsigned i=0; i<64; ++i) assert(!smk37_pad_sample(&state,i).velocity);
        assert(!smk37_pad_sample(&state,100).velocity);
        assert(!smk37_pad_sample(&state,350).velocity);
        assert(!smk37_pad_sample(&state,960).velocity);
        assert(smk37_pad_sample(&state,500).velocity==127);
        for (unsigned i=0; i<20; ++i) assert(!smk37_pad_sample(&state,350).velocity);
        assert(!smk37_pad_sample(&state,33).release);
        assert(smk37_pad_sample(&state,32).release);
        assert(!smk37_pad_sample(&state,0).release);
        /* Quick tap must preserve both edges, even before peak window ends. */
        smk37_pad_sample(&state,350);
        smk37_pad_change_t tap=smk37_pad_sample(&state,0);
        assert(tap.velocity==46 && tap.release && !state.held);
    }
    puts("SMK analog controls: mapping, monotonic scaling, deadband, pad peak/hold/release passed");
}
