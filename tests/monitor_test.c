/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define __attribute__(x)
#define NTRK 4u
enum { G_CLOCK, G_BPM };
static struct { unsigned sel; int g[2]; } song;
static uint32_t fm1_ms;
static void fm1_irq_off(void) {}
static void fm1_irq_on(void) {}
static void lcd_sync(void) {}
static void lcd_blit(unsigned x, unsigned y, unsigned w, unsigned h, const uint16_t *p)
{ (void)x; (void)y; (void)w; (void)h; (void)p; }
static unsigned str_len(const char *s) { return (unsigned)strlen(s); }
static void str_cpy(char *d, const char *s, unsigned n) { if (n) snprintf(d,n,"%s",s); }
static void fmt_int(char *s, int32_t v) { sprintf(s,"%d",v); }
static void note_name(char *s, unsigned n) {
    static const char *const names[]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    sprintf(s,"%s%d",names[n%12],(int)n/12-1);
}
#include "../firmware/src/monitor.c"
#include "../firmware/src/gfx.c"
#define H_GRAPH 124
#include "../firmware/src/monitor_draw.c"
static void expect(const char *s) { char text[32]; monitor_text(text); assert(!strcmp(text,s)); }
static void packet(unsigned src, unsigned status, unsigned a, unsigned b, unsigned ms) {
    fm1_ms=ms; monitor_receive(status<<8 | a<<16 | b<<24,src,ms);
}
int main(int argc, char **argv) {
    monitor_trigger(0,60,100); expect("");
    monitor_mode=MON_NOTES; fm1_ms=100; expect("C4"); /* held before enabling */
    monitor_trigger(0,64,100); fm1_ms=2000; monitor_trigger(0,67,100);
    expect("C4 E4 G4"); /* rolled chord and long holds do not expire */
    monitor_trigger(0,67,100); expect("C4 E4 G4");
    monitor_release(0,64); expect("C4 G4");
    monitor_trigger(0,64,0); expect("C4 G4");
    monitor_trigger(1,72,100); song.sel=1; expect("C5"); song.sel=0; expect("C4 G4");
    monitor_release(0,60); monitor_release(0,67); expect("");
    for(unsigned i=0;i<6;i++) monitor_trigger(0,60+i,100);
    expect("C4 C#4 D4 D#4 +2");
    monitor_clear(0); expect(""); song.sel=1; expect("C5"); song.sel=0;
    fm1_ms=0xfffffff0u; monitor_trigger(0,60,100);
    fm1_ms=900; expect("C4"); monitor_release(0,60); expect("");
    monitor_mode=MON_EVENTS; song.g[G_CLOCK]=2; song.g[G_BPM]=120;
    packet(2,0x90,60,100,1000); expect("TRS CH1 C4 V100");
    packet(2,0xf8,0,0,1010); expect("TRS CH1 C4 V100");
    packet(2,0xf8,0,0,1500); expect("TRS CLK 120");
    packet(1,0xb1,1,64,1510); expect("USB CH2 CC1 64");
    packet(1,0x90,60,0,1520); expect("USB CH1 C4 OFF");
    packet(2,0xfa,0,0,1530); expect("TRS START");
    packet(2,0xfb,0,0,1540); expect("TRS CONT");
    packet(2,0xfc,0,0,1550); expect("TRS STOP");
    packet(1,0xef,0,0,1560); expect("USB CH16 BEND -8192");
    fm1_ms=2360; expect("");
    /* Actual graphics preview: held chord, lower-right inside a waveform. */
    palette_set(NPALETTES-1); font_bold=1;
    cv_begin(240,H_GRAPH,T_SURF);
    for(int x=1;x<240;x++) cv_line(x-1,40+((x-1)%40),x,40+(x%40),T_TEXT);
    monitor_mode=MON_NOTES; monitor_clear(0); fm1_ms=3000;
    monitor_trigger(0,60,100); monitor_trigger(0,64,100); monitor_trigger(0,67,100);
    draw_monitor();
    if(argc>1) {
        FILE *f=fopen(argv[1],"wb"); assert(f); fprintf(f,"P6\n240 124\n255\n");
        for(unsigned i=0;i<240*124;i++) {
            unsigned c=swap16(cv_px[i]); fputc((c>>11)*255/31,f);
            fputc(((c>>5)&63)*255/63,f); fputc((c&31)*255/31,f);
        }
        fclose(f);
    }
    puts("Monitor: held chords, releases, track isolation, event timeout, timer wrap, USB/TRS events and clock suppression passed.");
}
