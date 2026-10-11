/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#include "../firmware/src/smk37_synth.c"
#include "../firmware/hal/smk37_controls.h"
static unsigned sent_count;
static void midi_out_event(uint32_t packet) { (void)packet; ++sent_count; }
static int32_t output[CTL*2];
static void tick(void) { mix_block(output,CTL); }
static void press(unsigned id) { smk37_button_event(id-1); }
/* Real contact frames -> decoder -> four transitions -> parameter update.
 * Positive is the stock signed-event direction, pending physical CW recheck. */
static void knob_cycle(unsigned encoder, int direction)
{
    uint8_t frame[SMK37_SCAN_SLOTS];
    smk37_controls_t state={0};
    memset(frame,255,sizeof frame);
    smk37_controls_tick(&state,frame);
    const unsigned positive[4]={2,3,1,0}, negative[4]={1,3,2,0};
    for(unsigned n=0;n<4;++n) {
        unsigned phase=direction>0 ? positive[n] : negative[n];
        memset(frame,255,sizeof frame);
        for(unsigned b=0;b<2;++b) if(phase&(1u<<b)) {
            unsigned code=smk37_encoder_contacts[encoder][b];
            frame[code>>4]&=(uint8_t)~(1u<<(code&15));
        }
        smk37_control_changes_t event=smk37_controls_tick(&state,frame);
        for(unsigned i=0;i<8;++i) smk37_encoder_event(i,event.encoder[i]);
    }
}
int main(void)
{
    smk37_synth_init(); tick();
    /* Each physical encoder can edit in its bank; no second sign reversal. */
    smk37_page=3;
    for(unsigned i=0;i<8;++i) {
        smk37_knob_bank=i<4;
        int16_t *value;
        const param_desc_t *desc=page_desc(&smk37_sound_pages[3],i%4,&value);
        assert(desc && value);
        int old=*value; *value=50;
        knob_cycle(i,1); assert(*value==51);
        knob_cycle(i,-1); assert(*value==50);
        *value=old;
    }
    smk37_knob_bank=0;
    smk37_page=0;
    knob_cycle(3,1); assert(smk37_page==1);
    knob_cycle(3,-1); assert(smk37_page==0);
    smk37_patch_mode=1;
    assert(smk37_indicator_status()&(1u<<12));
    /* Each navigation shortcut opens a real parameter page. */
    press(14); assert(smk37_page==1 && !smk37_patch_mode);
    press(4); assert(smk37_page==2); press(3); assert(smk37_page==1);
    press(15); assert(smk37_page==7 && (smk37_indicator_status()&(1u<<14)));
    press(11); assert(smk37_page==18);
    press(16); assert(smk37_page==23);
    press(13); assert(smk37_page==0 && smk37_patch_mode);
    unsigned preset=TSEL->preset;
    press(4); assert(TSEL->preset!=preset); press(3); assert(TSEL->preset==preset);
    tick();
    /* Octave applies only to new piano notes, never steals held ownership. */
    smk37_play_key_event(0,100); assert(kb_note[0]==48 && kb_chn[0]);
    press(6); assert(song.octave==1);
    smk37_play_key_event(1,100); assert(kb_note[1]==61);
    smk37_play_key_event(0,0); assert(!kb_chn[0] && !midi_local_held(TSEL,48));
    smk37_play_key_event(1,0);
    for(unsigned i=0;i<20;++i) press(6);
    assert(song.octave==3 && (smk37_indicator_status()&(1u<<5)));
    for(unsigned i=0;i<20;++i) press(5);
    assert(song.octave==-3 && (smk37_indicator_status()&(1u<<4)));
    song.octave=0;
    /* SCALE initializes a useful major scale; WHITE suppresses black keys.
     * Release still targets the original note even after toggling the scale. */
    press(9); assert(TSEL->p[P_QUANT]==QN_SNAP && TSEL->p[P_SCALE]==1);
    smk37_play_key_event(1,100); assert(kb_note[1]==48);
    press(9); assert(!TSEL->p[P_QUANT]);
    smk37_play_key_event(1,0); assert(!midi_local_held(TSEL,48));
    TSEL->p[P_QUANT]=QN_WHITE;
    smk37_play_key_event(1,100); assert(!kb_chn[1]);
    smk37_play_key_event(2,100); assert(kb_note[2]==50 && kb_chn[2]);
    smk37_play_key_event(2,0); TSEL->p[P_QUANT]=0;
    press(10); assert(TSEL->p[P_CHRD]==3);
    TSEL->p[P_VOICE]=V_POLY; tick();
    smk37_play_key_event(0,100); assert(kb_chn[0]==3);
    press(10); assert(!TSEL->p[P_CHRD]);
    smk37_play_key_event(0,0);
    assert(!midi_local_held(TSEL,48) && !midi_local_held(TSEL,52));
    /* ARP and REPEAT are exclusive. Saved choices are per part. */
    press(7); assert(TSEL->p[P_AMODE]==1); tick();
    smk37_play_key_event(0,100); assert(TSEL->nheld);
    smk37_play_key_event(0,0);
    TSEL->p[P_AMODE]=3; press(7); assert(!TSEL->p[P_AMODE]); tick();
    press(7); assert(TSEL->p[P_AMODE]==3);
    press(8); assert(TSEL->p[P_AMODE]==6);
    assert((smk37_indicator_status()&(3u<<6))==(1u<<7));
    press(8); assert(!TSEL->p[P_AMODE]); tick();
    song.sel=1; press(7); assert(TSEL->p[P_AMODE]==1); press(7); tick(); song.sel=0;
    /* Banking does not change sound until a knob/fader actually moves. */
    press(13); smk37_encoder_event(0,1);
    press(1); assert(smk37_knob_bank && !smk37_encoder_phase[0]);
    int level=TSEL->p[P_LEVEL]; smk37_turn(0,-1); assert(TSEL->p[P_LEVEL]==level-1);
    smk37_turn(4,1); assert(song.sel==1); smk37_turn(4,-1); assert(song.sel==0);
    press(1); assert(!smk37_knob_bank);
    uint16_t raw[6]={400,400,400,400,528,630};
    smk37_analog_playback(raw,63);
    int attack=TSEL->p[P_ATK]; press(2); smk37_analog_playback(raw,63);
    assert(TSEL->p[P_ATK]==attack);
    raw[0]=1023; smk37_analog_playback(raw,63); assert(TSEL->p[P_ATK]==127);
    int volume=trk[0].p[P_LEVEL]; press(2); smk37_analog_playback(raw,63);
    assert(trk[0].p[P_LEVEL]==volume);
    raw[0]=0; smk37_analog_playback(raw,63); assert(trk[0].p[P_LEVEL]==0);
    trk[0].p[P_LEVEL]=100; trk[0].p[P_ATK]=0;
    /* Record a real piano note and release, disarm, stop, replay the pattern. */
    press(18); assert(song.rec==1 && transport_req==1); tick(); assert(song.playing);
    smk37_play_key_event(0,100); tick(); smk37_play_key_event(0,0);
    unsigned notes=0;
    for(unsigned i=0;i<NSTEP;++i) notes+=trk[0].step[i].time==ST_NOTE;
    assert(notes);
    press(18); assert(!song.rec && !trk[0].rh_n);
    press(17); tick(); assert(!song.playing && !song.rec);
    press(17); tick(); assert(song.playing && trk[0].seq_n);
    press(17); tick(); assert(!song.playing && !trk[0].seq_n);
    /* Press edges queued in one block still start/stop in the correct order. */
    press(17); press(17); assert(transport_req==2); tick(); assert(!song.playing);
    press(12); assert(smk37_notice && !(smk37_indicator_status()&(1u<<11)));
    /* Diagnostics never toggle functions or start recording. */
    smk37_page=SMK37_BUTTON_PAGE;
    for(unsigned i=1;i<=18;++i) press(i);
    assert(smk37_page==SMK37_BUTTON_PAGE && !song.rec && !song.playing);
    assert(!smk37_indicator_status());
    puts("SMK buttons: shortcuts, banks, octave/scale/chord releases, arp/repeat, RAM record/replay and diagnostics passed");
}
