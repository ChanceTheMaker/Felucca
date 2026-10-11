/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t registers[0x51040 / 4];
#define SMK37_LCD_REG(a) registers[(a)/4]
#include "../firmware/hal/smk37_lcd_hw.h"

int main(void)
{
    uint8_t data[4] = {0,0,0,239};
    registers[0x50000/4] = 0x12345678; /* PA including key input PA2 */
    registers[0x51020/4] = 0xffffffff;
    registers[0x50048/4] = 0xffffffff;
    registers[0x50088/4] = 0xffffffff;
    smk37_lcd_hw_init(8);
    assert(registers[0x11d00/4] == 0x4021);
    assert(registers[0x11d04/4] == 8);
    assert(registers[0x51020/4] == 0xffffffef);
    assert(registers[0x50048/4] == 0xffffff9f);
    assert(registers[0x50088/4] == 0xfffffeff);
    assert(registers[0x5008c/4] == 0x100);
    assert(registers[0x5004c/4] == 0xe0);
    assert(registers[0x50000/4] == 0x12345678);
    assert(!smk37_lcd_data(0, 4, 5));
    assert(!smk37_lcd_data(data, 0, 5));
    assert(!smk37_lcd_data(data, 115201, 5));
    assert(!smk37_lcd_command(0x11, 0));
    /* RAM model does not implement write-one-to-clear pending. Set completion
     * explicitly; these assertions cover programming and failure cleanup. */
    registers[0x11d00/4] |= 0x8000;
    assert(smk37_lcd_command(0x2a, 5));
    assert(registers[0x11d08/4] == 0x2a);
    assert(!(registers[0x50040/4] & 0x20));
    assert(smk37_lcd_data(data, 4, 5));
    assert(registers[0x11d0c/4] == (uint32_t)(uintptr_t)data);
    assert(registers[0x11d10/4] == 4);
    assert(registers[0x50040/4] & 0x20);
    assert(registers[0x50080/4] & 0x100);
    registers[0x11d00/4] &= ~0x8000u;
    assert(!smk37_lcd_command(0x11, 5));
    assert(registers[0x11d00/4] == 0x4000);
    assert(registers[0x50080/4] & 0x100);
    assert(!smk37_lcd_command(0x29, 5));
    puts("SMK-37 LCD register simulation and timeout: PASS");
    return 0;
}
