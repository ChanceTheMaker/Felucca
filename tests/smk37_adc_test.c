/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static uint32_t regs[0x52000u / 4u];
#define SMK37_ADC_REG(a) regs[(a)/4u]
#define SMK37_SCAN_REG(a) regs[(a)/4u]
#include "../firmware/hal/smk37_adc.h"
#include "../firmware/hal/smk37_scanner.h"
static void isr(void) {}
static void attach(uint32_t n, void (*fn)(void), uint32_t p)
{ assert(n == 37 && fn == isr && p == 5); }
static int tick(smk37_adc_t *s)
{
    int result;
    SMK37_SCAN_CON |= 0x8000;
    result = smk37_adc_spi_done(s);
    smk37_scan_set_analog(smk37_analog_mask(&s->scan));
    (void)smk37_scan_tick();
    assert((smk37_scan_word | 0x38u) == smk37_scan_select[smk37_scan_slot]);
    assert((smk37_scan_word & 0x38u) == (smk37_analog_mask(&s->scan) & 0x38u));
    return result;
}
int main(void)
{
    smk37_adc_t s;
    const uint32_t pa = (1u<<8)|(1u<<10), pc = 1u<<9;
    for (unsigned i = 0; i < sizeof(regs)/sizeof(regs[0]); ++i) regs[i] = UINT32_MAX;
    smk37_adc_init(&s);
    assert(SMK37_ADC_CON == 0 && s.enabled);
    assert(SMK37_ADC_REG(0x50008u) == UINT32_MAX);
    assert(SMK37_ADC_REG(0x5000cu) == (UINT32_MAX & ~pa));
    assert(SMK37_ADC_REG(0x50010u) == (UINT32_MAX & ~pa));
    assert(SMK37_ADC_REG(0x50014u) == (UINT32_MAX & ~pa));
    assert(SMK37_ADC_REG(0x5008cu) == (UINT32_MAX & ~pc));
    assert(SMK37_ADC_REG(0x11900u) == (UINT32_MAX & ~(1u<<14)));
    smk37_scan_set_analog(smk37_analog_mask(&s.scan));
    smk37_scan_init(attach, isr);
    for (unsigned input = 0; input < 48; ++input) {
        unsigned index = s.scan.index, ch = smk37_analog_channel(&s.scan);
        assert(!tick(&s) && !SMK37_ADC_CON);
        assert(!tick(&s) && !SMK37_ADC_CON);
        assert(!tick(&s));
        assert(SMK37_ADC_CON == (0x405du | ch << 8));
        assert(!(SMK37_ADC_CON & 0x20u)); /* no ADC IRQ producer */
        assert(!tick(&s)); /* conversion not done yet */
        SMK37_ADC_REG(0x13104u) = 0xfc00u | input;
        SMK37_ADC_CON |= 0x80u; /* simulate conversion completion */
        assert(tick(&s) == 1);
        assert(!SMK37_ADC_CON && s.scan.raw[index] == input);
    }
    assert(s.samples == 48 && s.scan.valid == 0xffffffu);
    for (unsigned i = 0; i < 3; ++i) assert(!tick(&s));
    for (unsigned i = 0; i < 127; ++i) assert(!tick(&s));
    assert(tick(&s) == -1 && s.timeouts == 1 && !s.enabled);
    assert(!SMK37_ADC_CON);
    for (unsigned i = 0; i < 1000; ++i) assert(!tick(&s));
    assert(s.samples == 48); /* stalled ADC does not stall/restart digital scanner */
    smk37_scan_stop();
    smk37_adc_stop(&s);
    puts("SMK-37 ADC/scanner integration: GPIO, mux preservation, 24 inputs, settling and timeout passed");
    return 0;
}
