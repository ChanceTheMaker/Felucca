/* SPDX-License-Identifier: GPL-3.0-only */
#define main sample_baseline
#include "smk37_samples_test.c"
#undef main
static unsigned smk37_analog_failed,smk37_light_failed,smk37_last_velocity=100;
static uint16_t pixels[240*240];
static void lcd_sync(void){}
static void lcd_blit(uint32_t x,uint32_t y,uint32_t w,uint32_t h,const uint16_t *p){
    assert(!locked&&x+w<=240&&y+h<=240);for(unsigned r=0;r<h;++r)memcpy(pixels+(y+r)*240+x,p+r*w,w*2);
}
#include "../firmware/src/gfx.c"
#include "../firmware/src/smk37_draw.c"
static void render(const char *name){
    cv_begin(240,120,T_BG);cv_blit(0,0);cv_blit(0,120);smk37_sound_status();
    char path[256];snprintf(path,sizeof path,"build/smk37-tests/%s.ppm",name);FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 240\n255\n");
    for(unsigned i=0;i<240*240;++i){unsigned p=swap16(pixels[i]);fputc((p>>11)*255/31,f);fputc(((p>>5)&63)*255/63,f);fputc((p&31)*255/31,f);}fclose(f);
}
int main(void){
    smk37_synth_init();palette_set(0);font_set(0);smk37_open(SMK37_STORAGE_PAGE);render("test21-storage");
    smk37_storage_confirm=1;smk37_storage_confirm_at=fm1_ms;render("test21-confirm");
    smk37_samples_owned=1;render("test21-ready");smk37_open(33);render("test21-midi");
    puts("Test21 sample initialization/confirmation/ready and MIDI screens rendered within LCD bounds");
}
