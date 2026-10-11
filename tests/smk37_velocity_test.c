/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_velocity.h"
int main(void) {
    for(unsigned mode=0;mode<4;++mode) {
        assert(!smk37_key_velocity(0,2400,mode));
        unsigned previous=127;
        for(unsigned frames=1;frames<10000;++frames) {
            unsigned v=smk37_key_velocity(frames,2400,mode);
            assert(v>=1 && v<=127 && v<=previous); previous=v;
        }
        assert(smk37_key_velocity(100,0,mode)==100);
    }
    assert(smk37_key_velocity(20,2400,0)==127);
    assert(smk37_key_velocity(600,2400,0)==1);
    assert(smk37_key_velocity(UINT32_MAX,UINT32_MAX,0)==1);
    assert(smk37_key_velocity(100,2400,0)==smk37_key_velocity(200,1200,0));
    puts("SMK velocity: release, timebase equivalence, all curves, bounds and overflow passed");
}
