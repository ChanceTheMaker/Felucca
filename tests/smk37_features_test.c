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
static unsigned request(unsigned cmd,const uint8_t *a,unsigned n){uint8_t f[640]={0x7d,0x46,0x4c};assert(n<636);f[3]=cmd;if(n)memcpy(f+4,a,n);return cmd==33 || (cmd>=16&&cmd<=21)?smk37_editor_main_command(f,n+4):smk37_editor_handle(f,n+4);}
static void wire(unsigned cmd,const uint8_t *a,unsigned n){
    unsigned count=request(cmd,a,n);assert(count && count<sizeof smk37_ed_out);
    putchar('[');for(unsigned i=0;i<count;++i)printf("%s%u",i?",":"",smk37_ed_out[i]);puts("]");
}
int main(int argc,char **argv){
    memset(flash,255,sizeof flash);smk37_synth_init();tick();
    if(argc>1 && !strcmp(argv[1],"--wire")){
        wire(1,0,0);wire(4,0,0);wire(15,0,0);wire(27,0,0);
        uint8_t args[4]={0};wire(16,args,2);wire(22,args,1);args[0]=4;wire(33,args,1);args[0]=0;
        for(unsigned e=0;e<NENGINES;++e){
            args[0]=e;wire(10,args,1);TSEL->eng_req=e;
            for(unsigned i=0;i<P_COUNT;++i){args[0]=0;args[1]=i;wire(5,args,2);}
        }
        for(unsigned i=0;i<G_COUNT;++i){args[0]=1;args[1]=i;wire(5,args,2);}
        for(unsigned t=0;t<NTRK;++t){args[0]=t;wire(28,args,1);wire(29,args,1);
            args[1]=0;wire(30,args,2);wire(31,args,2);}
        args[0]=0;wire(6,args,1);return 0;
    }
    TSEL->p[P_VOICE]=V_POLY;
    smk37_play_key_event(0,70);smk37_sustain(1);smk37_play_key_event(0,0);
    assert(kb_chn[0]&&smk37_pedal_pending[0]);
    song.sel=1;smk37_sustain(0);assert(!kb_chn[0]&&!midi_local_held(&trk[0],48));
    smk37_play_key_event(1,80);smk37_sustain(1);smk37_play_key_event(1,0);
    smk37_play_key_event(1,110);assert(kb_chn[1]&&!smk37_pedal_pending[1]);
    smk37_sustain(0);assert(kb_chn[1]);smk37_play_key_event(1,0);
    song.sel=0;smk37_pad_bank=0;smk37_pad_event(0,100);assert(kb_note[37]==40);
    smk37_pad_bank=1;smk37_pad_event(0,0);assert(!kb_chn[37]);
    smk37_pad_event(0,100);assert(kb_note[37]==56);smk37_pad_event(0,0);
    smk37_page=SMK37_STEP_PAGE;smk37_edit_step=16;last_note=64;
    smk37_pad_event(3,90);assert(TSEL->step[19].note[0]==64&&TSEL->step[19].time==ST_NOTE&&!kb_chn[40]);
    smk37_pad_event(3,0);smk37_pad_event(3,90);assert(TSEL->step[19].time==ST_REST);
    smk37_turn(7,1);assert(TSEL->step[19].time==ST_NOTE);
    /* Complete real sound/sequence/settings snapshot roundtrip, including reboot simulation. */
    smk37_storage_ready=smk37_storage_writable=1;
    trk[0].p[P_LEVEL]=87;song.octave=2;smk37_velocity_mode=2;
    assert(!smk37_session_save(2));
    trk[0].p[P_LEVEL]=4;TSEL->step[19].time=ST_REST;song.octave=0;
    assert(!smk37_session_load(2));tick();
    assert(trk[0].p[P_LEVEL]==87&&trk[0].step[19].time==ST_NOTE&&song.octave==2&&smk37_velocity_mode==2);
    trk[0].p[P_LEVEL]=92;assert(!smk37_session_save(0));
    smk37_save_generation=0;trk[0].p[P_LEVEL]=1;
    assert(!smk37_session_resume()&&smk37_project_slot==0&&trk[0].p[P_LEVEL]==92);
    song.playing=1;assert(smk37_session_save(2));song.playing=0;
    smk37_sustain(1);assert(smk37_session_save(2));smk37_sustain(0);
    midi_owners[1]=1;assert(smk37_session_save(2));midi_owners[1]=0;
    assert(smk37_store_allocation_ok(&smk37_storage_io,sizeof smk37_session));
    assert(request(1,0,0)>20);
    uint8_t set[]={0,P_LEVEL,99,64};assert(request(3,set,4)&&TSEL->p[P_LEVEL]==99);
    assert(!request(3,set,3));set[1]=127;assert(!request(3,set,4));
    uint8_t desc[]={0,P_E0};assert(request(5,desc,2));
    uint8_t watch[]={1};assert(request(22,watch,1));TSEL->p[P_LEVEL]=98;
    assert(smk37_editor_changed()&&smk37_ed_out[4]==23);
    TSEL->step[0].vel=61;assert(smk37_editor_changed()&&smk37_ed_out[4]==26&&smk37_ed_n==8);
    uint8_t tr[]={2};assert(request(27,tr,1)&&song.sel==2);
    assert(smk37_editor_changed()&&smk37_ed_out[4]==24);
    /* Reject every truncated write and a non-7-bit data byte. */
    uint8_t step[14]={0,0,1,60,0,0,0,ST_NOTE,0,90,0,0,0,100};
    assert(request(30,step,14));step[13]=101;assert(!request(30,step,14));
    set[0]=128;assert(!request(3,set,4));
    puts("SMK features: sustain ownership, pad banks, step entry, persistent session roundtrip and editor protocol passed");
}
