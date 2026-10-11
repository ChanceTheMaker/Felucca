/* SPDX-License-Identifier: GPL-3.0-only */
/* Register-memory simulation: checks programming, not physical audio timing. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static uint32_t registers[0x51040 / 4];
#define SMK37_AUDIO_R8(a) (*(uint8_t *)&registers[(a)/4])
#define SMK37_AUDIO_R16(a) (*(uint16_t *)&registers[(a)/4])
#define SMK37_AUDIO_R32(a) registers[(a)/4]
#include "../firmware/hal/smk37_audio.h"

static unsigned delays, attachments;
static void isr(void) {}
static void delay(uint32_t ms) { assert(ms == 5); ++delays; }
static void attach(uint32_t irq, void (*fn)(void), uint32_t priority)
{
    assert(irq == 11 && fn == isr && priority == 3);
    assert(!(SMK37_AUDIO_R16(0x12e00) & 0x800));
    ++attachments;
}

int main(void)
{
    int32_t buffer[256] = {0};
    const smk37_audio_hooks_t hooks = {delay, attach};
    assert(!smk37_audio_init(0, 128, isr, 3, &hooks));
    assert(!smk37_audio_init(buffer, 127, isr, 3, &hooks));
    assert(!smk37_audio_init(buffer, 65536, isr, 3, &hooks));
    assert(!smk37_audio_init(buffer, 128, 0, 3, &hooks));
    assert(!smk37_audio_init(buffer, 128, isr, 8, &hooks));
    assert(!smk37_audio_init(buffer, 128, isr, 3, 0));
    assert(!delays && !attachments && !SMK37_AUDIO_R16(0x12e00));
    SMK37_AUDIO_R32(0x51030) = 0xffffffffu;
    SMK37_AUDIO_R32(0x10014) = 0xffffffffu;
    SMK37_AUDIO_R32(0x50088) = 0xffffffffu;
    SMK37_AUDIO_R32(0x50090) = 0xa55a; /* pull-ups are not DIE */
    assert(smk37_audio_init(buffer, 128, isr, 3, &hooks));
    assert(delays == 2 && attachments == 1);
    assert(SMK37_AUDIO_R32(0x5008c) == 0x47);
    assert(SMK37_AUDIO_R32(0x50080) == 0x47);
    assert(SMK37_AUDIO_R32(0x50088) == (0xffffffffu & ~0x47u));
    assert(SMK37_AUDIO_R32(0x50090) == 0xa55a);
    assert(SMK37_AUDIO_R32(0x51030) == 0xffffff3fu);
    assert(SMK37_AUDIO_R32(0x10014) == 0xfffff0ffu);
    assert(SMK37_AUDIO_R16(0x12e00) == 0x980);
    assert(SMK37_AUDIO_R16(0x12e04) == 0x5000);
    assert(SMK37_AUDIO_R8(0x12e0c) == 0x83);
    assert(SMK37_AUDIO_R16(0x12e20) == 128);
    assert(SMK37_AUDIO_R32(0x12e1c) == (uint32_t)(uintptr_t)buffer);
    SMK37_AUDIO_R8(0x12e08) = 0xf0;
    assert(smk37_audio_pending() == 0xf0);
    smk37_audio_ack_aux(0xf0);
    smk37_audio_ack_half();
    assert(SMK37_AUDIO_R8(0x12e08) == 0xff);
    assert(smk37_audio_free_half() == 1);
    SMK37_AUDIO_R16(0x12e00) |= 0x8000;
    assert(smk37_audio_free_half() == 0);
    smk37_audio_stop();
    assert(!(SMK37_AUDIO_R16(0x12e00) & 0x800));
    puts("SMK-37 audio register simulation: PASS (not hardware validation)");
    return 0;
}
