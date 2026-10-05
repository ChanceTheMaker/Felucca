/* SPDX-License-Identifier: GPL-3.0-only
 * Browser adapter for the original Felucca DSP. No USB, flash, or radio drivers. */
#include <stdint.h>
#include "felucca_tables.h"
#include "../../firmware/src/libc.c"
static struct { volatile uint32_t notes, buttons; } fm1_in;
static const uint8_t empty_sample[512];
#define SMP_USER_XIP(k) (empty_sample)
#include "../../firmware/src/core.h"
#include "../../firmware/src/engines.c"
#include "../../firmware/src/drums.c"
#include "../../firmware/src/params.c"
#include "../../firmware/src/voice.c"
#include "../../firmware/src/slicer.c"
#include "../../firmware/src/fx.c"
#define MQ 64u
static uint32_t midi_in_q[MQ], midi_in_ms[MQ], mi_r, mi_w, fm1_ms;
static uint8_t midi_in_src[MQ];
static void midi_out_event(uint32_t p) { (void)p; }
#include "../../firmware/src/seq.c"
#define API __attribute__((visibility("default")))
static int32_t output[CTL * 2];
static uint64_t frames;
API void synth_init(void) {
    for (unsigned i=0;i<G_COUNT;i++) song.g[i]=GP[i].def;
    for (unsigned k=0;k<NTRK;k++) {
        for (unsigned i=0;i<P_E0;i++) trk[k].p[i]=TP[i].def;
        for (unsigned i=0;i<NSTEP;i++) trk[k].step[i].time=ST_REST;
    }
    song.master_q12=4096;
    song.g[G_DRCH]=0;
}
API void synth_engine(unsigned e) { if(e<NENGINES) trk[0].eng_req=e; }
API void synth_param(unsigned i,int v) {
    if(i>=P_COUNT) return;
    const param_desc_t *d=i<P_E0 ? &TP[i] : &ENGINES[trk[0].eng_req]->edit[i-P_E0];
    trk[0].p[i]=v<d->min?d->min:v>d->max?d->max:v;
}
API void synth_global(unsigned i,int v) {
    if(i<G_COUNT && i!=G_CLOCK && i!=G_DRCH) song.g[i]=v<GP[i].min?GP[i].min:v>GP[i].max?GP[i].max:v;
}
API void synth_midi(unsigned status,unsigned d1,unsigned d2) {
    /* This preview is one selected sound, regardless of the keyboard channel. */
    midi_event(status&0xf0u,0,d1&127,d2&127);
}
API int32_t *synth_render(void) {
    fm1_ms=(uint32_t)(frames*1000/FS);
    mix_block(output,CTL); frames+=CTL; return output;
}
API unsigned preset_count(unsigned e) { return e<NENGINES?ENGINES[e]->npresets:0; }
API const char *preset_name(unsigned e,unsigned p) {
    return e<NENGINES && p<ENGINES[e]->npresets?ENGINES[e]->presets[p].name:"";
}
API int preset_value(unsigned e,unsigned p,unsigned i) {
    if(e>=NENGINES || p>=ENGINES[e]->npresets || i>=P_COUNT) return 0;
    const preset_t *pr=&ENGINES[e]->presets[p];
    if(i>=P_E0) return pr->e[i-P_E0];
    if(i>=P_ATK && i<=P_REL) return pr->env[i-P_ATK];
    if(i==P_ED_FLT) return pr->fenv;
    if(i==P_VOICE) return pr->mono?V_LEGATO:V_POLY;
    if(i>=P_DIST && i<=P_REV) {
        const uint8_t defaults[]={0,24,28,36};
        return pr->fx[i-P_DIST]?pr->fx[i-P_DIST]-1:defaults[i-P_DIST];
    }
    if(i>=P_AMODE && i<=P_AGATE && pr->arp[i-P_AMODE]) return pr->arp[i-P_AMODE]-1;
    return TP[i].def;
}
