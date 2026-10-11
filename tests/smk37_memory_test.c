/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_memory.h"
int main(void)
{
    uint32_t ram[64], image[16];
    for (unsigned i = 0; i < 64; ++i) ram[i] = 0xa5a50000u + i;
    for (unsigned i = 0; i < 16; ++i) image[i] = 0x12340000u + i;
    smk37_memory_zero(ram+8, ram+24);
    smk37_memory_copy(ram+32, ram+48, image);
    for (unsigned i = 0; i < 64; ++i)
        assert(ram[i] == (i >= 8 && i < 24 ? 0u :
               i >= 32 && i < 48 ? image[i-32] : 0xa5a50000u+i));
    /* Empty RAM-text/data sections may share a linker boundary. */
    smk37_memory_zero(ram+24, ram+24);
    smk37_memory_copy(ram+24, ram+24, image);
    assert(ram[24] == 0xa5a50018u);
    puts("SMK-37 startup memory: copy, zero, empty sections and untouched guards passed");
    return 0;
}
