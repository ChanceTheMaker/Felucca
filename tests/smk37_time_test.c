/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static uint32_t timer[3];
#define SMK37_TIME_REG(a) timer[((a)-0x10800u)/4u]
#include "../firmware/hal/smk37_time.h"
int main(void)
{
    /* Same source/mode, wrong inherited divider and short period. */
    timer[0] = 0x00f9; timer[1] = 123; timer[2] = 999;
    smk37_time_init();
    assert(timer[0] == 0x4009 && timer[1] == 0 && timer[2] == 0xffffffffu);
    timer[0] = 0; timer[1] = timer[2] = 0;
    smk37_time_init();
    assert(timer[0] == 0x4009 && timer[1] == 0 && timer[2] == 0xffffffffu);
    puts("SMK37 timebase: inherited prescaler/period replaced, counter reset passed");
    return 0;
}
