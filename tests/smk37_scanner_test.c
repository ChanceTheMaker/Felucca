/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
static uint32_t registers[0x52000u / 4u];
#define SMK37_SCAN_REG(a) registers[(a)/4u]
#include "../firmware/hal/smk37_scanner.h"
static void isr(void) {}
static void attach(uint32_t number, void (*handler)(void), uint32_t priority)
{
    assert(number == 37 && handler == isr && priority == 5);
    assert(!(SMK37_SCAN_CON & 1u));
}
static int tick(uint8_t rows)
{
    SMK37_SCAN_REG(0x50004u) = ((uint32_t)(rows&1)<<2) |
        ((uint32_t)(rows&14)<<4) | ((uint32_t)(rows&16)<<5);
    SMK37_SCAN_REG(0x501c4u) = ((uint32_t)(rows&32)<<1) |
        ((uint32_t)(rows&192)<<2);
    SMK37_SCAN_CON |= 0x8000u; /* model DMA completion; plain RAM has no W1C */
    return smk37_scan_tick();
}
int main(void)
{
    unsigned i;
    SMK37_SCAN_REG(0x51020u) = 0x10u | (1u<<16);
    /* Boot can leave PA0 high/input. Stock scanner explicitly overrides both. */
    SMK37_SCAN_REG(0x50000u) = 1u | (1u<<12);
    SMK37_SCAN_REG(0x50008u) = 1u | (1u<<12);
    smk37_scan_init(attach, isr);
    assert(!(SMK37_SCAN_REG(0x50000u) & 1u));
    assert(!(SMK37_SCAN_REG(0x50008u) & 1u));
    assert(SMK37_SCAN_REG(0x50000u) & (1u<<12));
    assert(SMK37_SCAN_REG(0x50008u) & (1u<<12));
    assert(SMK37_SCAN_CON == 0x6021u);
    assert(smk37_scan_word == 0xfeffffffu);
    assert(SMK37_SCAN_REG(0x11e10u) == 4);
    assert(SMK37_SCAN_REG(0x11e04u) == 5);
    assert(SMK37_SCAN_REG(0x51020u) == (0x10u | (1u<<17))); /* preserve SPI1 route */
    assert(SMK37_SCAN_REG(0x50008u) & (1u<<2));
    assert(SMK37_SCAN_REG(0x501c8u) & (1u<<8));
    assert(!smk37_scan_tick()); /* no pending IRQ, no state advance */
    for (i = 0; i < 16; ++i) assert(!tick(0xff)); /* discard startup frame */
    for (i = 0; i < 16; ++i) {
        assert(tick((uint8_t)(i*17)) == (i == 15));
        assert(smk37_scan_word == smk37_scan_select[(i+1)&15]);
        assert(!(SMK37_SCAN_REG(0x50000u) & 2u));
    }
    for (i = 0; i < 16; ++i) assert(smk37_scan_frame[i] == i*17);
    /* Every row bit through both ports, not just symmetric test patterns. */
    for (i = 0; i < 16; ++i) (void)tick((uint8_t)(1u<<(i&7)));
    for (i = 0; i < 16; ++i) assert(smk37_scan_frame[i] == (1u<<(i&7)));
    /* Indicators share the shift word; never perturb scan selection/ADC mux.
     * Enable the common only for the word just latched, not the next DMA. */
    SMK37_SCAN_REG(0x51000u)=0xabcdef;
    for (unsigned light=1; light<=19; ++light) {
        smk37_scan_indicator=light;
        smk37_scan_set_analog(0xffffffe7u);
        unsigned active=0;
        for (unsigned n=0; n<8; ++n) {
            unsigned pending=smk37_indicator_pending;
            tick(0xff);
            assert((SMK37_SCAN_REG(0x16a0cu)&0x60000u)==
                   (pending ? 0x60000u&~(1u<<(16+pending)) : 0x60000u));
            assert((smk37_scan_word&~0x7f07u)==
                   ((smk37_scan_select[smk37_scan_slot]&0xffffffe7u)&~0x7f07u));
            if (smk37_indicator_pending) {
                ++active;
                assert(smk37_indicator_pending==1+(smk37_indicator_order[light-1]>>4));
                assert((smk37_scan_word&0x7f07u)==(smk37_indicator_mask(light-1)&0x7f07u));
            } else assert((smk37_scan_word&0x7f07u)==0x7f07u);
        }
        assert(active==2);
    }
    smk37_scan_indicator=0;
    tick(0xff);
    assert((SMK37_SCAN_REG(0x16a0cu)&0x60000u)==0x60000u);
    assert(SMK37_SCAN_REG(0x51000u)==0xabcdef);
    /* Performance indicators can span both commons. Blank phases retain
     * 25% duty per bank; bits never leak to the other bank or ADC/key rows. */
    smk37_scan_indicator_bits=0x7ffff;
    unsigned bank_active[3]={0,0,0};
    for(unsigned n=0;n<8;++n) {
        unsigned pending=smk37_indicator_pending;
        tick(0xff);
        assert((SMK37_SCAN_REG(0x16a0cu)&0x60000u)==
               (pending ? 0x60000u&~(1u<<(16+pending)) : 0x60000u));
        ++bank_active[smk37_indicator_pending];
        uint32_t expected=smk37_scan_select[smk37_scan_slot]&0xffffffe7u;
        if(smk37_indicator_pending) for(unsigned id=0;id<19;++id)
            if((smk37_indicator_order[id]>>4)==smk37_indicator_pending-1u)
                expected &= smk37_indicator_mask(id);
        assert(smk37_scan_word==expected);
    }
    assert(bank_active[0]==4 && bank_active[1]==2 && bank_active[2]==2);
    smk37_scan_indicator_bits=0;
    tick(0xff);
    assert((SMK37_SCAN_REG(0x16a0cu)&0x60000u)==0x60000u);
    smk37_scan_stop();
    assert(SMK37_SCAN_CON == 0x4000u);
    assert((SMK37_SCAN_REG(0x16a0cu)&0x60000u)==0x60000u);
    puts("SMK37 scanner: route, GPIO packing, selection cycle and startup discard passed");
    return 0;
}
