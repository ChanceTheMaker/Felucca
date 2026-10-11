/* SPDX-License-Identifier: GPL-3.0-only */
#define main smk37_completion_baseline
#include "smk37_completion_test.c"
#undef main
int main(int argc,char **argv){
    if(argc>1){memset(flash,255,sizeof flash);smk37_synth_init();smk37_storage_ready=smk37_storage_writable=1;assert(!smk37_up_boot());
        uint8_t setting[]={2,MON_NOTES},fav[]={0,2,64,1},get[]={0,0,64,4};
        emit(35,setting,2);emit(38,fav,4);emit(34,0,0);emit(36,0,0);emit(37,get,4);return 0;}

    smk37_completion_baseline(0,0);
    seq_stop();song.rec=transport_req=0;smk37_pedal_down=0;chain.running=chain.armed=0;
    for(unsigned k=0;k<FELUCCA_KEY_COUNT;++k)if(kb_chn[k])key_off(k,&trk[kb_trk[k]]);
    for(unsigned k=0;k<NTRK;++k)midi_owners[k]=0;
    /* Old bank data migrates in RAM; newest-copy selection survives a partial
       first upgrade. Every address outside the original sectors is untouched. */
    memset(flash+SMK37_PRESET_A,255,4096);memset(flash+SMK37_PRESET_B,255,4096);
    smk37_up_work=smk37_up;
    assert(!smk37_store_save(&smk37_up_io,0,&smk37_up_work,SMK37_PRESET_V1_SIZE,&smk37_up_previous));
    assert(!smk37_up_boot());assert(!memcmp(&smk37_up,&smk37_up_work,SMK37_PRESET_V1_SIZE));
    assert(!smk37_up.prefs[31]);
    assert(favorite_set(0,0,1));assert(favorite_set(0,3,1));smk37_pref_set(2,MON_NOTES);smk37_animation=0;
    memcpy(saved,flash,sizeof flash);
    for(int cut=0;cut<17;++cut){memcpy(flash,saved,sizeof flash);assert(!smk37_up_boot());favorite_set(0,3,1);monitor_mode=MON_NOTES;smk37_animation=0;
        allowance=cut;int rc=smk37_preferences_save();allowance=-1;assert(!smk37_up_boot());assert(smk37_up_used(7));
        if(!rc){assert(favorite_has(0,3));assert(monitor_mode==MON_NOTES && !smk37_animation);}protected();}
    assert(!smk37_preferences_save());
    /* Empty favorites do not loop or load another sound. One favorite is one
       selectable result, regardless of repeated wraparound. */
    memset(&favorites,0,sizeof favorites);smk37_browser_engine=0;smk37_browser_fav=1;smk37_browser_index=0;
    assert(!smk37_browser_count());for(unsigned i=0;i<500;++i)smk37_browser_move(1);assert(!smk37_browser_count());
    favorite_set(0,2,1);smk37_browser_fix();assert(smk37_browser_count()==1 && smk37_browser_index==2);
    for(unsigned i=0;i<500;++i){smk37_browser_move(i&1?1:-1);assert(smk37_browser_index==2);}
    favorite_set(0,2,0);smk37_browser_fix();assert(!smk37_browser_count());
    /* Held keys open their layer; release cannot trigger an unrelated tap. */
    smk37_open(0);smk37_button_input(10,1);fm1_ms+=700;smk37_hold_poll();assert(smk37_page==SMK37_NAV_PAGE);
    smk37_button_input(10,0);assert(smk37_page==SMK37_NAV_PAGE);
    smk37_button_input(13,1);fm1_ms+=10;smk37_button_input(13,0);assert(smk37_page==SMK37_PROJECT_PAGE && !smk37_project_request);
    /* A gesture is one undo; another track is unaffected; undo itself is redo. */
    smk37_knob_bank=0;song.sel=0;TSEL->eng_req=0;smk37_load_preset(TSEL,0);smk37_open(3);
    TSEL->p[P_ATK]=10;smk37_sound_undo.valid=0;int other=trk[1].p[P_ATK];
    smk37_turn(4,1);smk37_turn(4,1);assert(TSEL->p[P_ATK]==12);smk37_sound_swap();assert(TSEL->p[P_ATK]==10);smk37_sound_swap();assert(TSEL->p[P_ATK]==12 && trk[1].p[P_ATK]==other);
    /* Browser engine/preset load undo restores the PREVIOUS engine too. */
    smk37_browser_fav=0;smk37_browser_engine=ENGI_FM6;smk37_browser_index=0;smk37_open(SMK37_BROWSER_PAGE);smk37_button_event(3);
    assert(TSEL->eng_req==ENGI_FM6);smk37_sound_swap();assert(TSEL->eng_req==0);smk37_sound_swap();assert(TSEL->eng_req==ENGI_FM6);
    smk37_open(SMK37_OP_PAGE);smk37_op=0;unsigned b=5*FP_OP+FP_OL;fm6_patch[0][b]=50;
    smk37_turn(7,1);assert(fm6_patch[0][b]==51 && TSEL->p[P_E7]==FM6_OWN);smk37_sound_swap();assert(fm6_patch[0][b]==50);smk37_sound_swap();assert(fm6_patch[0][b]==51);
    smk37_open(SMK37_OPENV_PAGE);smk37_turn(5,1);assert(smk37_op_stage==1);for(unsigned i=0;i<200;++i)smk37_turn(6,1);assert(fm6_patch[0][5*FP_OP+FP_R1+1]==99);
    /* All group rows point at valid pages; non-FM6 navigation skips operators. */
    for(smk37_nav_group=0;smk37_nav_group<8;++smk37_nav_group)for(unsigned r=0;r<smk37_nav_count();++r)assert(smk37_group_pages[smk37_nav_group][r]<SMK37_PAGE_COUNT);
    smk37_nav_group=0;TSEL->eng_req=0;assert(smk37_next_page(SMK37_BROWSER_PAGE,1)==SMK37_UNDO_PAGE);
    /* Actual note ownership retains pedal-held notes in the monitor. */
    smk37_load_preset(TSEL,0);monitor_mode=MON_NOTES;smk37_open(0);smk37_play_key_event(12,100);
    assert(monitor_notes[0].notes[60/32]&(1u<<(60%32)));smk37_sustain(1);smk37_play_key_event(12,0);
    assert(monitor_notes[0].notes[60/32]&(1u<<(60%32)));smk37_sustain(0);assert(!(monitor_notes[0].notes[60/32]&(1u<<(60%32))));
    puts("SMK workflow: old-bank migration/failure safety, favorites, hold layers, sound undo, FM6 and sustained monitor passed");
}
