/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_pedal.h"
int main(void)
{
    smk37_pedal_t s={0};
    for(unsigned i=0;i<20;++i)assert(smk37_pedal_sample(&s,0)==-1);
    assert(!s.armed && !s.down); /* boot while held: release to arm */
    for(unsigned i=0;i<3;++i)assert(smk37_pedal_sample(&s,871)==-1);
    assert(s.armed);
    assert(smk37_pedal_sample(&s,0)==-1);
    assert(smk37_pedal_sample(&s,871)==-1); /* bounce restarts debounce */
    assert(smk37_pedal_sample(&s,0)==-1);
    assert(smk37_pedal_sample(&s,0)==-1);
    assert(smk37_pedal_sample(&s,0)==1);
    for(unsigned i=0;i<20;++i)assert(smk37_pedal_sample(&s,0)==-1);
    for(unsigned i=0;i<20;++i)assert(smk37_pedal_sample(&s,400)==-1);
    assert(s.down); /* middle/noise cannot release */
    assert(smk37_pedal_sample(&s,871)==-1);
    assert(smk37_pedal_sample(&s,UINT32_MAX)==-1);
    assert(smk37_pedal_sample(&s,871)==-1);
    assert(smk37_pedal_sample(&s,871)==-1);
    assert(smk37_pedal_sample(&s,871)==0);
    assert(!s.down);
    smk37_pedal_reset(&s);assert(!s.armed && !s.count);
    puts("SMK pedal: ADC23 polarity, boot-held guard, bounce, hysteresis and release passed");
}
