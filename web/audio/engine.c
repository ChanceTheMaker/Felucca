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
#include "../../firmware/src/params.c"
#include "../../firmware/src/mod.c"
#include "../../firmware/src/voice.c"
#include "../../firmware/src/slicer.c"
#include "../../firmware/src/fx.c"
#define MQ 64u
static uint32_t midi_in_q[MQ], midi_in_ms[MQ], mi_r, mi_w;
static uint8_t midi_in_source[MQ], midi_in_overflow;
static void midi_out_event(uint32_t p) { (void)p; }
#include "../../firmware/src/song_chain.c"
#include "../../firmware/src/seq.c"
#define API __attribute__((visibility("default")))
static int32_t output[CTL * 2];
static uint64_t frames;
static unsigned browser_target;
API void synth_target(unsigned k) { if(k<NTRK) browser_target=k; }
API void synth_select(unsigned k) { if(k<NTRK) song.sel=k; }
static uint8_t browser_fm6[FM6_PACKED];
API uint8_t *synth_fm6_buffer(void) { return browser_fm6; }
API void synth_fm6_apply(void) {
    uint8_t voice[FP_SIZE + 1u];
    fm6_unpack(browser_fm6, voice);
    fm6_set_patch(browser_target, voice);
    fm6_slot[browser_target] = (uint8_t)trk[browser_target].p[P_E7];
}
API void synth_init(void) {
    for (unsigned i=0;i<G_COUNT;i++) song.g[i]=GP[i].def;
    for (unsigned k=0;k<NTRK;k++) {
        for (unsigned i=0;i<P_E0;i++) trk[k].p[i]=TP[i].def;
        for (unsigned i=0;i<NSTEP;i++) trk[k].step[i].time=ST_REST;
    }
    song.master_q12=4096;
    fm6_init();
}
API void synth_engine(unsigned e) { if(e<NENGINES && eng_ok(e)) trk[browser_target].eng_req=e; }
API void synth_param(unsigned i,int v) {
    if(i>=P_COUNT) return;
    const param_desc_t *d=i<P_E0 ? &TP[i] : &ENGINES[trk[browser_target].eng_req]->edit[i-P_E0];
    trk[browser_target].p[i]=v<d->min?d->min:v>d->max?d->max:v;
}
API void synth_global(unsigned i,int v) {
    if(i<G_COUNT && i!=G_CLOCK) song.g[i]=v<GP[i].min?GP[i].min:v>GP[i].max?GP[i].max:v;
}
API void synth_midi(unsigned status,unsigned d1,unsigned d2) {
    midi_event(status&0xf0u,status&15u,d1&127,d2&127);
}
API void synth_step(unsigned k,unsigned i,unsigned n,unsigned time,unsigned flags,unsigned vel,unsigned hit,unsigned acc,unsigned chance,unsigned n0,unsigned n1,unsigned n2,unsigned n3) {
    if(k>=NTRK || i>=NSTEP) return;
    step_t *s=&trk[k].step[i];
    s->n=n>4?4:n;s->time=time>2?2:time;s->flags=flags&3;s->vel=vel&127;
    s->hit=hit&255;s->acc=acc&s->hit;step_set_chance(s,chance>100?100:chance);
    s->note[0]=n0&127;s->note[1]=n1&127;s->note[2]=n2&127;s->note[3]=n3&127;
}
API void synth_motion_clear(unsigned k) { if(k<NTRK) motion_clear(&trk[k]); }
API void synth_motion_event(unsigned k,unsigned step,unsigned id,int value) { if(k<NTRK) motion_set_event(&trk[k],step,id,value); }
API void synth_motion_on(unsigned k,unsigned on) { if(k<NTRK) motion_set_enabled(&trk[k],on); }
API void synth_transport(unsigned op) { if(op==2){transport_req=0;seq_stop();}else if(op==1||op==3)transport_req=op; }
API unsigned synth_playing(void) { return song.playing; }
API unsigned synth_position(unsigned k) { return k<NTRK?trk[k].seq_idx:0; }
API void synth_chain_begin(void) { seq_stop();memset(&chain.config,0,sizeof chain.config);memset(chain.source,0,sizeof chain.source); }
API uint8_t *synth_chain_steps(unsigned slot) { return slot<4?(uint8_t*)chain.source[slot].step:0; }
_Static_assert(sizeof(step_t)==11,"browser chain step layout");
API void synth_chain_timing(unsigned slot,unsigned k,int len,int div,int swing,int gate) {
    if(slot>=4||k>=NTRK)return;
    int16_t *p=chain.source[slot].timing[k];p[0]=clamp(len,1,64);p[1]=clamp(div,0,9);p[2]=clamp(swing,0,100);p[3]=clamp(gate,1,127);
}
API void synth_chain_motion(unsigned slot,unsigned k,unsigned on,unsigned step,unsigned id,int value) {
    if(slot>=4||k>=NTRK)return;
    motion_store_t *m=&chain.source[slot].motion;
    if(on)m->on|=1u<<k;
    if(step>=64||id>=P_COUNT||m->count>=MOTION_MAX)return;
    m->event[m->count++]=(motion_event_t){(uint8_t)(k<<6|step),(uint8_t)id,(int16_t)value};
}
API void synth_chain_row(unsigned row,unsigned slot,unsigned repeat) {
    if(row>=16||slot>=4||repeat<1||repeat>16)return;
    chain.config.row[row]=(chain_row_t){slot,repeat};chain.config.count=row+1;
}
API void synth_chain_play(void) { if(chain.config.count&&chain_valid(&chain.config)){chain.armed=1;transport_req=1;} }
API unsigned synth_chain_status(void) {return chain.running|chain.row<<8|chain.remaining<<16;}
API int32_t *synth_render(void) {
    fm1_ms=(uint32_t)(frames*1000/FS);
    fm6_poll();
    mix_block(output,CTL); frames+=CTL; return output;
}
API unsigned engine_count(void) { return NENGINES; }
API unsigned param_count(void) { return P_COUNT; }
API unsigned preset_count(unsigned e) { return e<NENGINES && eng_ok(e)?ENGINES[e]->npresets:0; }
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
    return TP[i].def;
}
