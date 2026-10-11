/* SPDX-License-Identifier: GPL-3.0-only */
#define main smk37_completion_baseline
#include "smk37_completion_test.c"
#undef main
#include "../firmware/hal/smk37_expanded_presets.h"
int main(void){
    smk37_completion_baseline(0,0);
    seq_stop();song.rec=transport_req=0;chain.running=chain.armed=0;song.sel=0;
    TSEL->eng_req=0;smk37_load_preset(TSEL,0);memset(TSEL->step,0,sizeof TSEL->step);
    TSEL->step[0].time=ST_NOTE;TSEL->step[0].n=1;TSEL->step[0].note[0]=69;TSEL->step[0].vel=96;
    TSEL->p[P_SLEN]=8;TSEL->p[P_SDIV]=3;TSEL->p[P_SSWING]=17;TSEL->p[P_SGATE]=60;
    assert(!smk37_up_store(2,"PHRASE"));assert(smk37_user_phrase[2].used);
    unsigned p=NPATTERNS;while(smk37_phrase_slot(p)!=2)++p;smk37_phrase=p;
    TSEL->p[P_SLEN]=32;TSEL->p[P_ATK]=91;memset(TSEL->step,0,sizeof TSEL->step);smk37_phrase_load();
    assert(TSEL->step[0].note[0]==69&&TSEL->p[P_SLEN]==8&&TSEL->p[P_SDIV]==3&&TSEL->p[P_SSWING]==17&&TSEL->p[P_ATK]==91);
    smk37_pattern_undo();assert(TSEL->p[P_SLEN]==32&&seq_is_empty(TSEL));smk37_pattern_undo();
    /* The same user grid format as FM-1, including accents and clearing tail. */
    TSEL->eng_req=ENGI_DRUM;smk37_load_preset(TSEL,0);memset(TSEL->step,0,sizeof TSEL->step);TSEL->step[0].time=ST_NOTE;TSEL->step[0].hit=5;TSEL->step[0].acc=4;
    assert(!smk37_up_store(2,"GRID"));assert(smk37_user_phrase[2].grid);memset(TSEL->step,0,sizeof TSEL->step);smk37_phrase=p;smk37_phrase_load();assert(TSEL->step[0].hit==5&&TSEL->step[0].acc==4);
    assert(!smk37_up_put(2,0));assert(!smk37_user_phrase[2].used);
    smk37_open(SMK37_STEP_PAGE);smk37_step_preview=1;aud_req=0;smk37_knob_bank=0;smk37_edit_step=0;
    smk37_turn(4,1);assert(smk37_edit_step==1&&aud_req==257);aud_req=0;song.playing=1;smk37_turn(4,1);assert(!aud_req);song.playing=0;
    smk37_pref_set(20,0);smk37_turn(4,1);assert(!aud_req);
    smk37_pref_set(20,1);assert(!smk37_preferences_save());smk37_step_preview=0;assert(!smk37_up_boot()&&smk37_step_preview);
    uint8_t tune[]={22,29,64};assert(req(73,tune,3));assert(song.g[G_TUNE]==29&&smk37_ed_out[5]==0);
    uint8_t negative[]={22,106,63};assert(req(73,negative,3));assert(song.g[G_TUNE]==-22);
    static smk37_expanded_presets_t expanded;
    assert(!smk37_expand_presets(&expanded,&smk37_up,sizeof smk37_up));
    assert(!memcmp(expanded.record,smk37_up.record,sizeof smk37_up.record));assert(!memcmp(expanded.fm6,smk37_up.fm6,sizeof smk37_up.fm6));
    assert(!memcmp(expanded.prefs,smk37_up.prefs,32));assert(expanded.magic==0x32505553u);
    for(unsigned i=8;i<32;++i)assert(!up_valid(&expanded.record[i]));
    assert(!smk37_expand_presets(&expanded,&smk37_up,2564));assert(!expanded.prefs[31]);
    assert(smk37_expand_presets(&expanded,&smk37_up,100));
    puts("Test20: saved melodic/drum phrases and timing undo, step preview, tuning, persistence and 8-to-32 preset migration passed");
}
