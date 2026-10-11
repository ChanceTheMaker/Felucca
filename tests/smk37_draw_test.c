/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#include "../firmware/src/smk37_synth.c"
static void midi_out_event(uint32_t p){(void)p;}
static unsigned locked,transfers;
static void fm1_irq_off(void){assert(!locked);locked=1;}
static void fm1_irq_on(void){assert(locked);locked=0;}
#include "../firmware/src/smk37_session.c"
static int smk37_storage_read(uint32_t a,void *p,uint32_t n){(void)a;memset(p,255,n);return 0;}
static int smk37_storage_erase(uint32_t a){(void)a;return -1;}
static int smk37_storage_program(uint32_t a,const void *p,uint32_t n){(void)a;(void)p;(void)n;return -1;}
static unsigned smk37_analog_failed,smk37_light_failed,smk37_last_velocity=100;
static uint16_t pixels[240*240];
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p){
    assert(!locked && x+w<=240 && y+h<=240);++transfers;
    for(unsigned r=0;r<h;++r)memcpy(pixels+(y+r)*240+x,p+r*w,w*2);
}
#include "../firmware/src/gfx.c"
#include "../firmware/src/smk37_draw.c"
static void save(const char *name){
    char path[256];snprintf(path,sizeof path,"build/smk37-tests/%s.ppm",name);
    FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(unsigned i=0;i<240*240;++i){unsigned p=swap16(pixels[i]);fputc((p>>11)*255/31,f);fputc(((p>>5)&63)*255/63,f);fputc((p&31)*255/31,f);}fclose(f);
}
static void render(unsigned page){
    smk37_page=page;cv_begin(240,120,T_BG);cv_blit(0,0);cv_blit(0,120);
    smk37_sound_status();assert(!locked);
}
static uint32_t panel_hash(void){
    uint32_t h=2166136261u;
    for(unsigned i=76*240;i<198*240;++i)h=(h^pixels[i])*16777619u;
    return h;
}
static void graph_responds(unsigned page,unsigned param,int a,int b){
    int old=TSEL->p[param];TSEL->p[param]=a;render(page);uint32_t h=panel_hash();
    TSEL->p[param]=b;render(page);assert(h!=panel_hash());TSEL->p[param]=old;
}
int main(void){
    smk37_synth_init();
    for(unsigned i=0;i<SCOPE_N;++i)scope_buf[i]=(int16_t)((i%48)*1100-26000);
    TSEL->step[0]=(step_t){.time=ST_NOTE,.n=1,.note={60},.vel=100};
    TSEL->step[4]=TSEL->step[0];TSEL->step[7]=TSEL->step[0];TSEL->step[12]=TSEL->step[0];
    for(unsigned palette=0;palette<NPALETTES;++palette)for(unsigned bold=0;bold<2;++bold){
        palette_set(palette);font_set(bold);
        for(unsigned page=0;page<SMK37_SOUND_PAGES;++page)render(page);
    }
    for(unsigned engine=0;engine<NENGINES;++engine){
        TSEL->eng_req=engine;smk37_load_preset(TSEL,0);
        for(unsigned page=1;page<=2;++page){
            render(page);
            for(unsigned i=P_E0;i<P_COUNT;++i)TSEL->p[i]=param_desc_of(engine,i)->max;
            render(page);
        }
    }
    for(unsigned style=0;style<2;++style)for(unsigned large=0;large<2;++large){
        smk37_style=style;smk37_large=large;
        for(unsigned page=0;page<SMK37_SOUND_PAGES;++page)render(page);
    }
    smk37_style=smk37_large=0;
    smk37_large=1;render(3);save("test19-large");smk37_large=0;
    smk37_style=1;render(3);save("test19-line");smk37_style=0;
    render(SMK37_PHRASE_PAGE);save("test19-phrases");
    smk37_pref_row=12;render(SMK37_PREF_PAGE);save("test19-settings");
    TSEL->eng_req=0;smk37_load_preset(TSEL,0);palette_set(0);font_set(0);
    render(0);save("test15-mix");render(1);save("test15-engine");render(3);save("test15-envelope");render(SMK37_STEP_PAGE);save("test15-steps");
    render(SMK37_DISPLAY_PAGE);save("test15-display");
    /* Only the center panel is hashed: changing card digits cannot pass these. */
    graph_responds(13,P_SCALE,0,1);
    graph_responds(7,P_REV,0,127);
    graph_responds(14,P_CHRD,0,3);
    graph_responds(27,P_ITYPE,0,1);
    graph_responds(17,P_SLCR,0,1);
    TSEL->p[P_M1SRC]=1;TSEL->p[P_M1DST]=1;graph_responds(19,P_M1AMT,0,75);
    TSEL->eng_req=ENGI_FM6;smk37_load_preset(TSEL,0);graph_responds(1,P_E0,1,32);
    TSEL->eng_req=0;smk37_load_preset(TSEL,0);
    render(SMK37_STEP_PAGE);uint32_t empty=panel_hash();
    TSEL->step[1]=(step_t){.time=ST_NOTE,.n=2,.note={64,67},.vel=100};render(SMK37_STEP_PAGE);assert(empty!=panel_hash());
    /* Last piano key and pad owners are not truncated to the FM-1's 27 keys. */
    proll.lo=48;kb_chn[36]=1;kb_trk[36]=song.sel;kb_chord[36][0]=60;assert(pr_held()&(1u<<8));kb_chn[36]=0;
    kb_chn[52]=1;kb_trk[52]=song.sel;kb_chord[52][0]=60;assert(pr_held()&(1u<<8));kb_chn[52]=0;
    const unsigned pages[]={7,13,14,17,19,27,30,32,39,40,42,25,8,12,24,26};
    TSEL->p[P_CHRD]=3;TSEL->p[P_SLCR]=1;TSEL->p[P_ITYPE]=1;TSEL->p[P_SCALE]=1;
    chain_config.count=3;for(unsigned i=0;i<3;++i)chain_config.row[i]=chain_row_of(i,2);
    motion_set_event(TSEL,0,P_LEVEL,75);motion_set_lock(TSEL,4,P_PAN,20);
    for(unsigned i=0;i<sizeof pages/sizeof *pages;++i){char name[40];render(pages[i]);snprintf(name,sizeof name,"test17-page-%02u",pages[i]);save(name);}
    TSEL->eng_req=ENGI_FM6;smk37_load_preset(TSEL,0);render(1);save("test17-fm6");
    for(unsigned a=1;a<=32;++a){TSEL->p[P_E0]=a;render(1);}
    TSEL->eng_req=ENGI_DRUM;smk37_load_preset(TSEL,0);TSEL->step[0].hit=5;render(SMK37_STEP_PAGE);save("test17-drums");
    TSEL->eng_req=0;smk37_load_preset(TSEL,0);
    palette_set(14);render(3);save("test15-light");
    smk37_analog_failed=1;render(0);smk37_analog_failed=0;
    smk37_light_failed=1;render(0);smk37_light_failed=0;

    /* Render the new pages from actual C, including each FM6 operator/stage. */
    palette_set(0);TSEL->eng_req=ENGI_FM6;smk37_load_preset(TSEL,0);
    for(unsigned p=SMK37_NAV_PAGE;p<SMK37_SOUND_PAGES;++p){char name[40];render(p);snprintf(name,sizeof name,"test18-page-%02u",p);save(name);}
    for(smk37_op=0;smk37_op<6;++smk37_op)for(smk37_op_stage=0;smk37_op_stage<4;++smk37_op_stage)render(SMK37_OPENV_PAGE);
    smk37_op=smk37_op_stage=0;
    for(unsigned i=0;i<NENGINES;++i)if(ENGINES[i]==&ENG_SLICE){TSEL->eng_req=i;smk37_load_preset(TSEL,0);for(unsigned j=0;j<12;++j)render(SMK37_SLICE_PAGE);save("test18-slices");break;}
    /* Rolling digits: first frame differs, final frame matches animation OFF.
       Digit count changes and page changes snap. Values never get delayed. */
    smk37_animation=1;fm1_ms=2000;TSEL->p[P_ATK]=50;render(3);fm1_ms+=150;TSEL->p[P_ATK]=51;render(3);
    assert(smk37_roll[0].from[0]);save("test18-rolling");fm1_ms+=180;render(3);assert(!smk37_roll[0].from[0]);
    fm1_ms+=150;smk37_draw_card(0,"VALUE","99","",-1);fm1_ms+=150;smk37_draw_card(0,"VALUE","100","",-1);assert(!smk37_roll[0].from[0]);
    smk37_animation=0;fm1_ms+=150;TSEL->p[P_ATK]=101;render(3);assert(!smk37_roll[0].from[0]);
    /* Master volume has its own odometer, independent of BPM/cards. */
    smk37_animation=1;smk37_knob_bank=0;song.master_q12=2048;fm1_ms=7000;render(0);
    fm1_ms+=150;smk37_turn(0,1);assert(song.master_q12==2080);render(0);
    assert(smk37_roll[ROLL_MASTER].from[0]);save("test20-master-roll");
    fm1_ms+=180;render(0);assert(!smk37_roll[ROLL_MASTER].from[0]);save("test20-master-settled");
    smk37_animation=0;fm1_ms+=150;smk37_turn(0,-1);render(0);assert(!smk37_roll[ROLL_MASTER].from[0]);
    smk37_animation=1;fm1_ms+=150;song.master_q12=4096;render(0);assert(!smk37_roll[ROLL_MASTER].from[0]);
    assert(transfers>1000);puts("SMK display: all pages/palettes/fonts, engine extremes, LCD bounds and IRQ checks passed");
}
