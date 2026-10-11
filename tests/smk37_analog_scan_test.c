/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_analog_scan.h"
int main(void)
{
    smk37_analog_scan_t s = {0};
    const unsigned muxes[24] = {0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,4,5,0,1,2,3,6,7};
    unsigned visited[16] = {0};
    for (unsigned sweep = 0; sweep < 3; ++sweep)
        for (unsigned index = 0; index < 24; ++index) {
            unsigned ch = smk37_analog_channel(&s);
            uint32_t mask = smk37_analog_mask(&s);
            assert(s.index == index);
            assert(ch == (index < 8 ? 1u : index < 16 ? 8u : 2u));
            assert(((mask >> 3) & 7) == muxes[index]);
            assert((mask & UINT32_C(0xffffffc7)) == UINT32_C(0xffffffc7));
            if (!sweep) {
                assert(!(visited[ch] & (1u << muxes[index])));
                visited[ch] |= 1u << muxes[index];
            }
            assert(!smk37_analog_complete(&s, 999));
            assert(!smk37_analog_spi_done(&s));
            assert(!smk37_analog_spi_done(&s));
            assert(smk37_analog_spi_done(&s));
            for (unsigned n = 0; n < 1000; ++n) assert(!smk37_analog_spi_done(&s));
            assert(smk37_analog_complete(&s, (uint16_t)(index + 100*sweep)));
            assert(s.raw[index] == index + 100*sweep);
            assert(s.valid & (1u << index));
            assert(!smk37_analog_complete(&s, 999));
        }
    assert(!s.index && s.valid == 0xffffffu);
    assert(visited[1] == 255 && visited[8] == 255 && visited[2] == 255);
    puts("SMK-37 analog schedule: 24 external selections, settling, result ownership and wrap passed");
    return 0;
}
