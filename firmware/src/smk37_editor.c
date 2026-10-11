/* SPDX-License-Identifier: GPL-3.0-only
 * Wire formats adapted from editor.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. Existing command numbers unchanged.
 * SMK subset: sound, descriptors, steps, presets, projects and four-track mixing.
 * No OTA, sample writes, user-preset flash or unsupported capability claims. */
static uint8_t smk37_ed_out[2048];
static unsigned smk37_ed_n;
static void smk37_ed_byte(unsigned v){if(smk37_ed_n<sizeof smk37_ed_out-1)smk37_ed_out[smk37_ed_n++]=v&127u;}
static void smk37_ed_value(int v){unsigned u=clamp(v,-8192,8191)+8192;smk37_ed_byte(u);smk37_ed_byte(u>>7);}
static int smk37_ed_read(const uint8_t *p){return (p[0]|p[1]<<7)-8192;}
static void smk37_ed_string(const char *s,unsigned max){for(unsigned i=0;s&&s[i]&&i<max;++i)smk37_ed_byte((uint8_t)s[i]);smk37_ed_byte(0);}
static void smk37_ed_begin(unsigned cmd){smk37_ed_n=0;smk37_ed_out[smk37_ed_n++]=0xf0;smk37_ed_byte(0x7d);smk37_ed_byte(0x46);smk37_ed_byte(0x4c);smk37_ed_byte(cmd);}
static unsigned smk37_ed_finish(void){smk37_ed_out[smk37_ed_n++]=0xf7;return smk37_ed_n;}
static void smk37_ed_step(const step_t *s){
    smk37_ed_byte(s->n);for(unsigned i=0;i<4;++i)smk37_ed_byte(s->note[i]);
    smk37_ed_byte(s->time);smk37_ed_byte(s->flags);smk37_ed_byte(s->vel);
    smk37_ed_byte(s->hit);smk37_ed_byte(s->acc);smk37_ed_byte((s->hit>>7)|((s->acc>>7)<<1));smk37_ed_byte(step_chance(s));
}
static void smk37_ed_step_put(step_t *s,const uint8_t *a,unsigned n){
    s->n=a[0]>4?4:a[0];for(unsigned i=0;i<4;++i)s->note[i]=a[1+i];
    s->time=a[5]>ST_REST?ST_REST:a[5];s->flags=a[6]&(SF_ACCENT|SF_SLIDE);s->vel=a[7];
    if(n>=11){s->hit=a[8]|((a[10]&1)<<7);s->acc=(a[9]|((a[10]&2)<<6))&s->hit;}
    if(n>=12)step_set_chance(s,a[11]);
}
static uint8_t smk37_ed_watch,smk37_ed_track,smk37_ed_engine,smk37_ed_preset;
static int16_t smk37_ed_shadow[P_COUNT+G_COUNT];
static step_t smk37_ed_steps[NSTEP];
static void smk37_ed_snapshot(void){
    smk37_ed_track=song.sel;smk37_ed_engine=TSEL->eng_req;smk37_ed_preset=TSEL->preset;
    memcpy(smk37_ed_shadow,TSEL->p,sizeof TSEL->p);
    memcpy(smk37_ed_shadow+P_COUNT,song.g,sizeof song.g);
    memcpy(smk37_ed_steps,TSEL->step,sizeof TSEL->step);
}
static void smk37_ed_load_preset(unsigned engine,unsigned preset){
    if(engine!=ENGI_DIGITAL){TSEL->eng_req=engine;smk37_load_preset(TSEL,preset);return;}
    int16_t p[P_COUNT];uint8_t values[FP_SIZE+1];
    memcpy(p,TSEL->p,sizeof p);fm4_preset_values(p,preset);
    unsigned converted=fm4_convert(p,values),tr=song.sel;
    panic_req|=(uint8_t)(1u<<tr);fm6_set_patch(tr,values);fm6_slot[tr]=p[P_E7];
    memcpy(TSEL->p,p,sizeof p);TSEL->eng_req=ENGI_FM6;TSEL->preset=converted;TSEL->user=0;
}
/* Caller serializes with audio. Returns zero for malformed/unsupported commands.
 * Project IO is deferred to main-loop service, outside the IRQ-off handler. */
static unsigned smk37_editor_handle(const uint8_t *f,unsigned n){
    if(n<4 || f[0]!=0x7d || f[1]!=0x46 || f[2]!=0x4c)return 0;
    for(unsigned i=0;i<n;++i)if(f[i]&128)return 0;
    unsigned cmd=f[3],na=n-4,i;const uint8_t *a=f+4;
    smk37_ed_begin(cmd);
    switch(cmd){
    case 1:
        if(na)return 0;
#if SMK37_ENABLE_USB_UPDATE
        smk37_ed_string("FELUCCA " SMK37_UPDATE_VERSION,24);
#elif SMK37_ENABLE_SAVES
        smk37_ed_string("FELUCCA SMK37 TEST10 SAVE",24);
#else
        smk37_ed_string("FELUCCA SMK37 TEST10",24);
#endif
        smk37_ed_byte(NENGINES);smk37_ed_byte(P_COUNT);smk37_ed_byte(G_COUNT);smk37_ed_byte(NSTEP);smk37_ed_byte(P_E0);
        for(i=0;i<NENGINES;++i)smk37_ed_string(ENGINES[i]->name,8);
        smk37_ed_byte(NTRK); /* no SONG/UI/backup/FM6-bank capabilities advertised */
        break;
    case 2:case 3:case 5:{
        if(na!=(cmd==3?4u:2u) || a[0]>1 || a[1]>=(a[0]?G_COUNT:P_COUNT))return 0;
        int16_t *v=a[0]?&song.g[a[1]]:&TSEL->p[a[1]];
        const param_desc_t *d=a[0]?&GP[a[1]]:param_desc_of(TSEL->eng_req,a[1]);
        if(cmd==3){
            int val=clamp(smk37_ed_read(a+2),d->min,d->max);
            if(a[0] && a[1]==G_ENGSEL){smk37_ed_load_preset((unsigned)val,0);}
            else if(a[0] && (a[1]==G_SAVE || a[1]==G_LOAD))return 0; /* explicit PROJECT command only */
            else *v=enum_orig(d,val);
        }
        smk37_ed_byte(a[0]);smk37_ed_byte(a[1]);
        if(cmd==5){smk37_ed_byte(d->fmt);smk37_ed_value(d->min);smk37_ed_value(d->max);smk37_ed_value(d->def);
            smk37_ed_string(d->label,8);smk37_ed_string(d->unit,8);
            if(d->fmt==F_ENUM && d->names)for(i=0;i<24u&&i<=(unsigned)(d->max-d->min);++i)smk37_ed_string(d->names[i],8);
        }else smk37_ed_value(a[0] && a[1]==G_ENGSEL ? TSEL->eng_req : *v);
        break;}
    case 4:
        if(na)return 0;smk37_ed_byte(TSEL->eng_req);smk37_ed_byte(TSEL->preset);
        for(i=0;i<P_COUNT;++i)smk37_ed_value(TSEL->p[i]);for(i=0;i<G_COUNT;++i)smk37_ed_value(song.g[i]);break;
    case 6:case 7:case 30:{
        unsigned off=cmd==30?2:1;
        if(na<off)return 0;
        unsigned tr=cmd==30?a[0]:song.sel,index=a[off-1],len=na-off;
        if(tr>=NTRK || index>=NSTEP || (cmd==6&&len) || (cmd==7&&!len) ||
           (len && len!=8 && len!=11 && len!=12) || (len==12&&a[off+11]>100))return 0;
        step_t *s=&trk[tr].step[index];if(len)smk37_ed_step_put(s,a+off,len);
        if(cmd==30)smk37_ed_byte(tr);smk37_ed_byte(index);smk37_ed_step(s);break;}
    case 8:
        if(na!=2 || a[0]>=NENGINES)return 0;
        smk37_ed_load_preset(a[0],a[1]);
        smk37_ed_byte(TSEL->eng_req);smk37_ed_byte(TSEL->preset);break;
    case 10:
        if(na!=1 || a[0]>=NENGINES)return 0;
        smk37_ed_byte(a[0]);smk37_ed_byte(ENGINES[a[0]]->npresets);
        for(i=0;i<ENGINES[a[0]]->npresets;++i)smk37_ed_string(ENGINES[a[0]]->presets[i].name,12);
        for(i=0;i<2;++i)smk37_ed_string(ENGINES[a[0]]->page_title[i],8);break;
    case 15:if(na)return 0;smk37_ed_byte(0);smk37_ed_byte(0);break;
    case 16:if(na!=2)return 0;smk37_ed_byte(a[0]);smk37_ed_byte(0);smk37_ed_byte(0);break;
    case 22:if(na!=1)return 0;smk37_ed_watch=a[0]&1;smk37_ed_snapshot();smk37_ed_byte(smk37_ed_watch);break;
    case 25:if(na)return 0;smk37_ed_byte(0);break;
    case 27:
        if(na>1 || (na&&a[0]>=NTRK))return 0;if(na)song.sel=a[0];
        smk37_ed_byte(song.sel);smk37_ed_byte(NTRK);
        for(i=0;i<NTRK;++i){smk37_ed_byte(trk[i].eng_req);smk37_ed_byte(trk[i].preset);smk37_ed_value(trk[i].p[P_LEVEL]);smk37_ed_byte(!!trk[i].p[P_MUTE]);smk37_ed_byte((song.rec>>i)&1);}
        break;
    case 28:
        if((na!=1&&na!=4)||a[0]>=NTRK)return 0;
        if(na==4){trk[a[0]].p[P_LEVEL]=clamp(smk37_ed_read(a+1),0,127);trk[a[0]].p[P_MUTE]=!!a[3];}
        smk37_ed_byte(a[0]);smk37_ed_value(trk[a[0]].p[P_LEVEL]);smk37_ed_byte(!!trk[a[0]].p[P_MUTE]);break;
    case 29:
        if(na!=1||a[0]>=NTRK)return 0;smk37_ed_byte(a[0]);smk37_ed_byte(trk[a[0]].eng_req);smk37_ed_byte(trk[a[0]].preset);
        for(i=0;i<P_COUNT;++i)smk37_ed_value(trk[a[0]].p[i]);break;
    case 31:{
        if((na!=2&&na!=4)||a[0]>=NTRK||a[1]>=P_COUNT)return 0;
        track_t *t=&trk[a[0]];const param_desc_t *d=param_desc_of(t->eng_req,a[1]);
        if(na==4)t->p[a[1]]=enum_orig(d,clamp(smk37_ed_read(a+2),d->min,d->max));
        smk37_ed_byte(a[0]);smk37_ed_byte(a[1]);smk37_ed_value(t->p[a[1]]);break;}
    default:return 0;
    }
    return smk37_ed_finish();
}
/* One push per pass. Existing v2 selected-track protocol, not v4 other-track pushes. */
static unsigned smk37_editor_changed(void){
    if(!smk37_ed_watch)return 0;
    if(smk37_ed_track!=song.sel || smk37_ed_engine!=TSEL->eng_req || smk37_ed_preset!=TSEL->preset){
        smk37_ed_snapshot();smk37_ed_begin(24);smk37_ed_byte(TSEL->eng_req);smk37_ed_byte(TSEL->preset);smk37_ed_byte(song.sel);return smk37_ed_finish();
    }
    for(unsigned i=0;i<P_COUNT+G_COUNT;++i){int v=i<P_COUNT?TSEL->p[i]:song.g[i-P_COUNT];if(v!=smk37_ed_shadow[i]){
        smk37_ed_shadow[i]=v;smk37_ed_begin(23);smk37_ed_byte(i>=P_COUNT);smk37_ed_byte(i<P_COUNT?i:i-P_COUNT);smk37_ed_value(v);return smk37_ed_finish();}}
    for(unsigned i=0;i<NSTEP;++i)if(memcmp(&smk37_ed_steps[i],&TSEL->step[i],sizeof(step_t))){
        smk37_ed_steps[i]=TSEL->step[i];smk37_ed_begin(26);smk37_ed_byte(i);smk37_ed_byte(song.sel);return smk37_ed_finish();}
    return 0;
}
