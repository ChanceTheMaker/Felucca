/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#include "../firmware/src/smk37_synth.c"
static void midi_out_event(uint32_t packet){(void)packet;}
static void fm1_irq_off(void){}
static void fm1_irq_on(void){}
#include "../firmware/src/smk37_session.c"
#include "../firmware/src/smk37_editor.c"
static uint8_t flash[0x100000];
static int smk37_storage_read(uint32_t a,void *p,uint32_t n){assert(smk37_store_range(a,n));memcpy(p,flash+a,n);return 0;}
static int smk37_storage_erase(uint32_t a){assert(smk37_store_range(a,4096));memset(flash+a,255,4096);return 0;}
static int smk37_storage_program(uint32_t a,const void *p,uint32_t n){assert(smk37_store_range(a,n));for(unsigned i=0;i<n;++i)flash[a+i]&=((const uint8_t *)p)[i];return 0;}
static int32_t audio[CTL*2];
static void tick(void){mix_block(audio,CTL);}
static unsigned request(unsigned cmd,const uint8_t *a,unsigned n){uint8_t f[640]={0x7d,0x46,0x4c};assert(n<636);f[3]=cmd;if(n)memcpy(f+4,a,n);return smk37_editor_handle(f,n+4);}
static void wire(unsigned cmd,const uint8_t *a,unsigned n){
    unsigned count=request(cmd,a,n);assert(count && count<sizeof smk37_ed_out);
    putchar('[');for(unsigned i=0;i<count;++i)printf("%s%u",i?",":"",smk37_ed_out[i]);puts("]");
}
#include "smk37_test12_save.h"
int main(void){
    memset(flash,255,sizeof flash);smk37_synth_init();tick();
    smk37_storage_ready=smk37_storage_writable=1;
    uint8_t old[3648];
    assert(!smk37_store_save(&smk37_storage_io,0,smk37_test12_save,3648,old));
    assert(smk37_store_allocation_ok(&smk37_storage_io,sizeof smk37_session));
    assert(!smk37_session_load(0));
    assert(trk[0].p[P_LEVEL]==92 && song.octave==2 && smk37_velocity_mode==2);
    assert(trk[0].step[19].time==ST_NOTE && trk[0].step[19].note[0]==64);
    assert(trk[0].p[P_ITYPE]==0 && trk[0].p[P_ESYNC]==0 && trk[0].p[P_SQNT]==1);
    /* Re-save as FUN10, then mixed old/new copies choose the newest generation. */
    trk[0].p[P_ITYPE]=3;smk37_palette=20;smk37_bold=1;click_mode=2;cin_bars=1;
    assert(!smk37_learn_set(21,0,P_ITYPE));
    assert(!smk37_session_save(0));trk[0].p[P_ITYPE]=0;smk37_palette=0;
    assert(!smk37_session_load(0));assert(trk[0].p[P_ITYPE]==3 && smk37_palette==20 && smk37_bold==1 && click_mode==2 && cin_bars==1);
    assert(ml_tab[0] || ml_tab[1]);
    /* Piano keys above key31 must retain their own arp mapping; pads stay fixed. */
    song.sel=0;song.octave=0;smk37_pad_bank=0;smk37_page=0;trk[0].p[P_AMODE]=1;trk[0].p[P_AHOLD]=0;trk[0].p[P_QUANT]=0;
    smk37_play_key_event(36,100);uint32_t notes[64],count;
    assert(arp_list(&trk[0],notes,&count) && notes[0]==84);
    trk[0].p[P_TRANS]=3;assert(arp_list(&trk[0],notes,&count) && notes[0]==87);
    smk37_play_key_event(36,0);assert(!trk[0].nheld);
    smk37_pad_event(15,100);assert(arp_list(&trk[0],notes,&count) && notes[0]==47);
    smk37_pad_event(15,0);
    /* Device controls and editor preserve ratchet/nudge when older writes arrive. */
    smk37_page=SMK37_DETAIL_PAGE;smk37_edit_step=0;step_t *s=&trk[0].step[0];
    smk37_turn(5,1);smk37_turn(6,-1);assert(step_ratchet(s)==2 && step_nudge(s)==-1);
    uint8_t args[16]={0,1,60,0,0,0,ST_NOTE,0,90};assert(request(7,args,9));
    assert(step_ratchet(s)==2 && step_nudge(s)==-1);
    args[12]=100;args[13]=4;args[14]=15;
    /* args: index plus fourteen step bytes */
    assert(request(7,args,15));assert(step_ratchet(s)==4 && step_nudge(s)==7);
    uint8_t lock[]={0,5,0,P_LEVEL,90,64};assert(request(64,lock,6));assert(motion.count==1 && (motion.event[0].param&MOTION_LOCK));
    uint8_t learn[]={0,22,1,P_LEVEL};assert(request(79,learn,4));
    midi_event(0xb0,1,22,44);assert(trk[1].p[P_LEVEL]==44);
    unsigned before=trk[1].preset;midi_event(0xc0,1,2,0);smk37_performance_poll();assert(trk[1].preset==2);
    midi_event(0xc0,1,127,0);smk37_performance_poll();assert(trk[1].preset==2);
    assert(smk37_learn_set(64,0,P_LEVEL));
    /* Performance pads own effects, never notes; release works after a page change. */
    smk37_page=SMK37_PERFORM_PAGE;
    smk37_pad_event(12,100);assert(perf_held&(1u<<PF_RTHR));assert(!kb_chn[49]);
    smk37_page=0;smk37_pad_event(12,0);assert(!(perf_held&(1u<<PF_RTHR)));
    smk37_page=SMK37_CLICK_PAGE;smk37_turn(4,-1);assert(click_mode==1);
    click_req=2;memset(audio,0,sizeof audio);click_render(audio,CTL);
    unsigned audible=0;for(unsigned i=0;i<CTL*2;++i)audible|=audio[i]!=0;assert(audible);
    puts("SMK 1.5.1: Test12 migration, saved new effects/preferences, 37-key arp, step detail, locks, learned CC and program change passed");
}
