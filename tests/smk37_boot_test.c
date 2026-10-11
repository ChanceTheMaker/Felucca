/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_BOOT_HOST_TEST 1
#include "../firmware/hal/smk37_boot.h"
int main(void)
{
    uint8_t parameters[24], header[16], info[48];
    unsigned i;
    for (i = 0; i < sizeof parameters; ++i) parameters[i] = (uint8_t)(i+32);
    for (i = 0; i < sizeof header; ++i) header[i] = (uint8_t)(i+128);
    for (i = 0; i < sizeof info; ++i) info[i] = 0xa5;
    smk37_boot_copy(info, parameters, header);
    assert(info[0] == header[13]);
    for (i = 1; i < 16; ++i) assert(info[i] == 0xa5);
    for (i = 0; i < 8; ++i) assert(info[16+i] == parameters[4+i]);
    for (i = 0; i < 4; ++i) assert(info[24+i] == header[8+i]);
    for (i = 0; i < 12; ++i) assert(info[28+i] == parameters[12+i]);
    for (i = 40; i < sizeof info; ++i) assert(info[i] == 0xa5);
    puts("SMK37 SPL handoff: stock field mapping and untouched reserved bytes passed");
    return 0;
}
