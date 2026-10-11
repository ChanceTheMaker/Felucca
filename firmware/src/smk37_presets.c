/* SPDX-License-Identifier: GPL-3.0-only
 * Record/protocol helpers from upreset.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. SMK allocation is independent of FM-1. */
#define UP_HOST 1
#include "upreset.c"
#undef UP_HOST
static void up_values(const up_rec_t *r, int16_t *v)   /* mapped and clamped for its engine */
{
    int16_t def[P_COUNT];
    uint32_t i;
    for (i = 0; i < P_COUNT; i++)
        def[i] = param_desc_of(r->engine, i)->def;
    up_params(r, v, def);
    if (r->ver == 1u && ENGINES[r->engine] == &ENG_PHYS)   /* (before 1.0: MODEL 2 was DUST) */
        phys_legacy(&v[P_E0]);
    for (i = 0; i < P_COUNT; i++)
        v[i] = (int16_t)param_fit(param_desc_of(r->engine, i), v[i]);   /* (a retired KIT: the kit it plays) */
}


#define SMK37_UP_SLOTS 8u
#define SMK37_UP_MAGIC 0x31505553u
static struct {uint32_t magic;up_rec_t record[SMK37_UP_SLOTS];uint8_t fm6[SMK37_UP_SLOTS][FM6_PACKED];}
    smk37_up __attribute__((section(".pool"))),smk37_up_work __attribute__((section(".pool"))),smk37_up_previous __attribute__((section(".pool")));
_Static_assert(sizeof smk37_up==2564,"SMK eight-preset bank size");
static uint8_t smk37_up_ready,smk37_up_writable;
/* Reuse tested A/B commit machinery with a private virtual slot0 mapping.
 * Neither mapping can reach project sectors or cross either 4KB boundary. */
static uint32_t smk37_up_address(uint32_t a,uint32_t n){
    if(a<SMK37_STORE_LO || a>=SMK37_STORE_LO+8192u || !n)return 0;
    uint32_t d=a-SMK37_STORE_LO;if(n>4096u-(d&4095u))return 0;
    return (d<4096u?SMK37_PRESET_A:SMK37_PRESET_B)+(d&4095u);
}
static int smk37_up_read(uint32_t a,void *p,uint32_t n){a=smk37_up_address(a,n);return a?smk37_storage_read(a,p,n):-1;}
static int smk37_up_erase(uint32_t a){a=smk37_up_address(a,4096);return a&&smk37_up_writable?smk37_storage_erase(a):-1;}
static int smk37_up_program(uint32_t a,const void *p,uint32_t n){a=smk37_up_address(a,n);return a&&smk37_up_writable?smk37_storage_program(a,p,n):-1;}
static const smk37_store_io_t smk37_up_io={smk37_up_read,smk37_up_erase,smk37_up_program};
static int smk37_up_boot(void){
    smk37_up_ready=smk37_up_writable=0;
    int copy=smk37_store_load(&smk37_up_io,0,&smk37_up,sizeof smk37_up,0);
    if(copy>=0){if(smk37_up.magic!=SMK37_UP_MAGIC)return -1;}
    else {
        for(uint32_t a=SMK37_STORE_LO;a<SMK37_STORE_LO+8192u;a+=256){
            if(smk37_up_read(a,smk37_store_scratch,256))return -1;
            for(unsigned i=0;i<256;++i)if(smk37_store_scratch[i]!=255)return -1;
        }
        memset(&smk37_up,0,sizeof smk37_up);smk37_up.magic=SMK37_UP_MAGIC;
    }
    for(unsigned i=0;i<SMK37_UP_SLOTS;++i)up_migrate(&smk37_up.record[i]);
    smk37_up_ready=1;smk37_up_writable=smk37_storage_writable;return 0;
}
static int smk37_up_used(unsigned k){return smk37_up_ready && k<SMK37_UP_SLOTS && up_valid(&smk37_up.record[k]);}
static int smk37_up_idle(void){
    if(song.playing || song.rec || transport_req || smk37_pedal_down || smk37_song_loading)return 0;
    for(unsigned k=0;k<FELUCCA_KEY_COUNT;++k)if(kb_chn[k])return 0;
    for(unsigned k=0;k<NTRK;++k)if(midi_owners[k])return 0;
    return 1;
}
/* RAM mirror changes only after a fully verified commit. Main loop only. */
static int smk37_up_commit(void){
    if(!smk37_up_writable || !smk37_up_idle())return 2;
    if(smk37_store_save(&smk37_up_io,0,&smk37_up_work,sizeof smk37_up_work,&smk37_up_previous))return 2;
    smk37_up=smk37_up_work;return 0;
}
static int smk37_up_put(unsigned k,const up_rec_t *r){
    if(k>=SMK37_UP_SLOTS || (r&&!up_valid(r)))return 1;
    smk37_up_work=smk37_up;
    if(r)smk37_up_work.record[k]=*r;else memset(&smk37_up_work.record[k],0,sizeof(up_rec_t));
    /* Importing/renaming the same FM6 slot keeps its OWN patch; a new slot gets factory 0. */
    if(!r || r->engine!=ENGI_FM6 || !smk37_up_used(k) || smk37_up.record[k].engine!=ENGI_FM6)
        memcpy(smk37_up_work.fm6[k],FM6_FACTORY[0],FM6_PACKED);
    return smk37_up_commit();
}
static int smk37_up_store(unsigned k,const char *name){
    if(k>=SMK37_UP_SLOTS)return 1;
    fm1_irq_off();
    if(!smk37_up_idle()){fm1_irq_on();return 2;}
    smk37_up_work=smk37_up;up_rec_t *r=&smk37_up_work.record[k];memset(r,0,sizeof *r);
    r->used=UP_USED;r->ver=UP_VER;r->engine=TSEL->eng_req;r->np=P_COUNT;
    if(name&&*name)memcpy(r->name,name,str_len(name)>12?12:str_len(name));
    else {const char *src=ENGINES[TSEL->eng_req]->presets[TSEL->preset%ENGINES[TSEL->eng_req]->npresets].name;memcpy(r->name,src,str_len(src)>12?12:str_len(src));}
    for(unsigned i=0;i<P_COUNT;++i)up_set_value(r,i,motion_base_value(TSEL,i));
    up_pat_from(r,TSEL->step);
    if(drum_track(TSEL)){unsigned hits=0;for(unsigned i=0;i<16;++i)hits|=step_lanes(&TSEL->step[i]);
        if(hits){r->ver=UP_VER_GRID;for(unsigned i=0;i<16;++i){r->note[i]=step_lanes(&TSEL->step[i]);r->flags[i]=step_accents(&TSEL->step[i]);}}}
    fm6_pack(fm6_patch[song.sel],smk37_up_work.fm6[k]);fm1_irq_on();return smk37_up_commit();
}
static int smk37_up_load(unsigned k){
    if(!smk37_up_used(k))return 1;
    int16_t v[P_COUNT];up_values(&smk37_up.record[k],v);unsigned e=smk37_up.record[k].engine;uint8_t fp[FP_SIZE+1];
    if(e==ENGI_DIGITAL){fm4_convert(v,fp);e=ENGI_FM6;}else fm6_unpack(smk37_up.fm6[k],fp);
    fm1_irq_off();track_t *t=TSEL;unsigned tr=song.sel;panic_req|=1u<<tr;
    motion_restore(t);
    for(unsigned i=0;i<P_COUNT;++i)if(!(i==P_LEVEL || i==P_PAN || i==P_MUTE || (i>=P_AMODE&&i<=P_SGATE) || (i>=P_SLCR&&i<=P_SLDEPTH) || i==P_CHRD || i==P_VOIC || i==P_SQNT))t->p[i]=v[i];
    t->eng_req=e;t->preset=0;t->user=k+1;
    if(e==ENGI_FM6){fm6_set_patch(tr,fp);fm6_adopt(tr);}fm1_irq_on();return 0;
}
