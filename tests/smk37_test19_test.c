/* SPDX-License-Identifier: GPL-3.0-only */
#define main smk37_completion_baseline
#include "smk37_completion_test.c"
#undef main
int main(int argc,char **argv){
    if(argc>1){
        memset(flash,255,sizeof flash);smk37_synth_init();smk37_storage_ready=smk37_storage_writable=1;assert(!smk37_up_boot());
        for(unsigned i=0;i<SMK37_MENU_COUNT;++i){uint8_t a=i;emit(72,&a,1);}
        uint8_t v[]={18,1,64};emit(73,v,3);return 0;
    }
    smk37_completion_baseline(0,0);
    song.sel=0;smk37_open(SMK37_PHRASE_PAGE);seq_stop();song.rec=0;chain.running=chain.armed=0;
    TSEL->eng_req=0;smk37_load_preset(TSEL,0);memset(TSEL->step,0,sizeof TSEL->step);
    TSEL->p[P_SLEN]=48;smk37_phrase=0;smk37_phrase_load();assert(TSEL->p[P_SLEN]==16);
    uint32_t sig=steps_sig(TSEL);unsigned engine=TSEL->eng_req;
    smk37_pattern_undo();assert(TSEL->p[P_SLEN]==48);smk37_pattern_undo();assert(TSEL->p[P_SLEN]==16 && sig==steps_sig(TSEL));
    TSEL->step[0].note[0]=99;sig=steps_sig(TSEL);smk37_phrase=1;smk37_phrase_load();assert(sig==steps_sig(TSEL));
    smk37_phrase_load();assert(sig!=steps_sig(TSEL) && TSEL->eng_req==engine);
    for(unsigned i=0;i<NPATTERNS;++i){memset(TSEL->step,0,sizeof TSEL->step);smk37_phrase=i;smk37_phrase_load();assert(TSEL->p[P_SLEN]==16);}
    for(unsigned i=11;i<SMK37_PREF_COUNT;++i)smk37_pref_set(i,smk37_pref_max(i));
    assert(!smk37_preferences_save());smk37_style=smk37_large=scope_mix=perf_latch_on=0;assert(!smk37_up_boot());
    assert(smk37_style && smk37_large && scope_mix && perf_latch_on);
    uint8_t setting[]={18,0,64};assert(smk37_editor_menu(73,setting,3));assert(!scope_mix);assert(!smk37_editor_menu(73,setting,2));
    uint8_t absent[]={126,0,64};assert(smk37_editor_menu(73,absent,3));assert(smk37_ed_out[5]==1);
    /* A synthetic user source shares read-only factory bytes for the host test.
     * This does not imply uploads/storage are enabled in the device image. */
    for(unsigned i=0;i<NENGINES;++i)if(ENGINES[i]==&ENG_SLICE)TSEL->eng_req=i;
    smk37_load_preset(TSEL,0);smk37_open(SMK37_SLICE_PAGE);
    TSEL->p[P_E0]=0;smk37_slice_edit(1,1);assert(!smk37_slice_request);
    slc_usr[0]=SLC_BREAK;usr_nz[0]=1;memset(slc_man,0,sizeof slc_man);TSEL->p[P_E0]=1;TSEL->p[P_E1]=0;smk37_slice_sel=1;
    smk37_slice_edit(1,1);assert(smk37_slice_request==1);smk37_slice_service();assert(!smk37_slice_request && TSEL->p[P_E1]==SLC_DIV_MAN);
    unsigned n=smk37_slice_count(),pos=smk37_slice_mark(1);assert(pos>SLC_BREAK.len/4);
    smk37_slice_edit(2,1);smk37_slice_service();assert(smk37_slice_count()==n+1);
    smk37_slice_edit(3,1);smk37_slice_service();assert(smk37_slice_count()==n);
    smk37_slice_sel=0;smk37_slice_edit(3,1);smk37_slice_service();assert(smk37_slice_count()==n);
    smk37_slice_edit(1,1);song.sel=1;smk37_slice_service();assert(!smk37_slice_request && slc_man_of(&slc_usr[0])->n==n);
    puts("Test19: phrases/confirmation/timing undo, settings/menu persistence, manual user slices and selection cancellation passed");
    return 0;
}
