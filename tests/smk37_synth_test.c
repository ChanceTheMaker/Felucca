/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#include "../firmware/src/smk37_synth.c"
#include "../firmware/hal/smk37_events.h"
static uint32_t sent[256], sent_count;
static void midi_out_event(uint32_t packet)
{
    assert(sent_count < 256);
    sent[sent_count++] = packet;
}
static int gated(unsigned track, unsigned note)
{
    for (unsigned i = 0; i < NVOICE; ++i)
        if (trk[track].v[i].gate && trk[track].v[i].note == note) return 1;
    return 0;
}
int main(void)
{
    int32_t out[CTL * 2];
    unsigned i, before, audible = 0;
    smk37_synth_init();
    mix_block(out, CTL);
    /* Each of the 37 physical indices can play and release its own note. */
    for (i = 0; i < 37; ++i) {
        smk37_key_event(i, 48+i, 40+i);
        assert(kb_chn[i] == 1 && midi_local_held(&trk[0], 48+i));
        assert((sent[sent_count-1] >> 24) == 40+i);
        assert(((sent[sent_count-1] >> 16) & 127) == 48+i);
        assert(gated(0, 48+i));
        for (unsigned b = 0; b < 8; ++b) {
            mix_block(out, CTL);
            for (unsigned x = 0; x < CTL*2; ++x) audible |= out[x] != 0;
        }
        smk37_key_event(i, 48+i, 0);
        assert(!kb_chn[i] && !midi_local_held(&trk[0], 48+i));
        assert(!gated(0, 48+i));
    }
    assert(audible);
    /* Upper keys participate in duplicate-note ownership, not a 32-bit mask. */
    smk37_key_event(0, 60, 91);
    before = sent_count;
    smk37_key_event(36, 60, 110);
    assert(sent_count == before);
    smk37_key_event(0, 60, 0);
    assert(gated(0, 60) && sent_count == before);
    song.sel = 1;
    smk37_key_event(36, 84, 0); /* release original track/note */
    assert(!gated(0, 60) && sent_count == before+1);
    assert(((sent[sent_count-1] >> 16) & 127) == 60);
    smk37_key_event(36, 84, 127);
    assert(gated(1, 84));
    before = sent_count;
    smk37_key_event(36, 83, 100); /* duplicate event while down */
    smk37_key_event(37, 84, 100);
    smk37_key_event(2, 128, 100);
    smk37_key_event(2, 60, 128);
    assert(sent_count == before);
    smk37_key_event(36, 84, 0);
    assert(!gated(1, 84));
    /* Simulate losing a release burst between scanner and audio. The same
     * local-key release API used by the overflow handler must clear every
     * physical key's ownership, including notes beyond the voice limit. */
    smk37_events_t queue = {0};
    song.sel = 0;
    for (i = 0; i < 37; ++i) smk37_key_event(i, 48+i, 100);
    for (i = 0; i < SMK37_EVENTS; ++i)
        assert(smk37_events_push(&queue, i % 37, 0));
    assert(!smk37_events_push(&queue, 36, 0) && queue.overflow);
    smk37_events_discard(&queue);
    for (i = 0; i < 37; ++i) smk37_key_event(i, 48+i, 0);
    for (i = 0; i < 37; ++i) {
        assert(!kb_chn[i] && !midi_local_held(&trk[0], 48+i));
        assert(!gated(0, 48+i));
    }
    smk37_key_event(36, 84, 100);
    assert(gated(0, 84));
    smk37_key_event(36, 84, 0);
    assert(!gated(0, 84));
    /* Encoder bounce cancels, complete detents select tracks, invalid input
     * cannot mutate selection. Track selection preserves a held key's owner. */
    song.sel = 0;
    smk37_key_event(36, 84, 100);
    smk37_encoder_event(0, 1);
    smk37_encoder_event(0, -1);
    assert(song.sel == 0);
    for (i = 0; i < 3; ++i) smk37_encoder_event(0, 1);
    assert(song.sel == 0);
    smk37_encoder_event(0, 1);
    assert(song.sel == 1);
    smk37_encoder_event(8, 1);
    smk37_encoder_event(0, 127);
    assert(song.sel == 1);
    smk37_key_event(36, 84, 0);
    assert(!gated(0, 84));
    smk37_turn(0, -1);
    assert(song.sel == 0);
    /* Exercise every selectable engine and every preset with real DSP. */
    unsigned first_engine = TSEL->eng_req, visited = 0;
    do {
        const engine_t *e = ENGINES[TSEL->eng_req];
        assert(FELUCCA_FM4 || TSEL->eng_req != ENGI_DIGITAL);
        for (unsigned p = 0; p < e->npresets; ++p) {
            smk37_turn(2, 1);
            assert(TSEL->preset < e->npresets);
            fm6_poll();
            /* Allow engine fade/switch to finish, then exercise actual voices. */
            for (unsigned b = 0; b < 32; ++b) mix_block(out, CTL);
            sent_count = 0;
            smk37_key_event(36, 84, 100);
            for (unsigned b = 0; b < 8; ++b) mix_block(out, CTL);
            smk37_key_event(36, 84, 0);
            assert(!kb_chn[36] && !midi_local_held(TSEL, 84));
        }
        smk37_turn(1, 1);
        ++visited;
        assert(visited <= NENGINES);
    } while (TSEL->eng_req != first_engine);
    assert(visited == NENGINES - !FELUCCA_FM4);
    /* Replacing the sound while held must allow release and a fresh attack. */
    sent_count = 0;
    smk37_load_preset(TSEL, 0);
    for (i = 0; i < 32; ++i) mix_block(out, CTL);
    smk37_key_event(36, 84, 100);
    smk37_turn(2, 1);
    mix_block(out, CTL);
    smk37_key_event(36, 84, 0);
    assert(!kb_chn[36] && !midi_local_held(TSEL, 84));
    smk37_load_preset(TSEL, 0);
    for (i = 0; i < 32; ++i) mix_block(out, CTL);
    smk37_key_event(36, 84, 100);
    assert(gated(song.sel, 84));
    smk37_key_event(36, 84, 0);
    /* All sound pages clamp edits. Empty controls and diagnostic page do
     * nothing; page navigation wraps in both directions. */
    for (smk37_page = 0; smk37_page < SMK37_STEP_PAGE; ++smk37_page) {
        for (unsigned slot = 0; slot < 4; ++slot) {
            int16_t *v;
            const param_desc_t *d = page_desc(&smk37_sound_pages[smk37_page], slot, &v);
            if (d && v) {
                *v = d->max;
                smk37_turn(slot+4, 1);
                assert(*v <= d->max && *v >= d->min);
                *v = d->min;
                smk37_turn(slot+4, -1);
                assert(*v <= d->max && *v >= d->min);
            } else smk37_turn(slot+4, 1);
        }
    }
    smk37_page=SMK37_ANALOG_PAGE;
    smk37_turn(4, 1);
    smk37_turn(3, 1);
    assert(smk37_page == SMK37_BUTTON_PAGE);
    unsigned saved_track=song.sel, saved_engine=TSEL->eng_req, saved_preset=TSEL->preset;
    for (i=0; i<8; ++i) if (i!=3) smk37_turn(i,1);
    assert(song.sel==saved_track && TSEL->eng_req==saved_engine && TSEL->preset==saved_preset);
    smk37_turn(3, 1);
    assert(smk37_page == SMK37_LIGHT_PAGE);
    for (i=0; i<36; ++i) smk37_turn(0,1);
    assert(!smk37_light_target);
    smk37_turn(0,-1); assert(smk37_light_target==35);
    smk37_turn(1,-1); assert(smk37_light_color==2);
    for (i=0; i<20; ++i) smk37_turn(2,1);
    assert(smk37_light_level==8);
    for (i=0; i<20; ++i) smk37_turn(2,-1);
    assert(smk37_light_level==1);
    assert(song.sel==saved_track && TSEL->eng_req==saved_engine && TSEL->preset==saved_preset);
    smk37_turn(3,1); assert(smk37_page==SMK37_PEDAL_PAGE);
    smk37_turn(3,1); assert(smk37_page==0);
    smk37_turn(3, -1);
    assert(smk37_page == SMK37_PEDAL_PAGE);
    sent_count = 0;
    song.sel = 0;
    smk37_load_preset(TSEL, 0);
    for (i=0; i<32; ++i) mix_block(out, CTL);
    /* Pads have distinct ownership from overlapping physical keys. */
    smk37_key_event(0, 48, 100);
    smk37_pad_event(4, 90); /* physical pad 5 also plays 48 */
    smk37_key_event(0, 48, 0);
    assert(midi_local_held(&trk[0],48));
    song.sel = 1;
    smk37_pad_event(4, 0);
    assert(!midi_local_held(&trk[0],48));
    for (i=0; i<16; ++i) {
        smk37_pad_event(i, 80);
        assert(kb_chn[37+i] && kb_trk[37+i]==1);
        smk37_pad_event(i, 0);
        assert(!kb_chn[37+i]);
    }
    uint16_t analog[6]={0,0,0,0,528,630};
    int level=trk[0].p[P_LEVEL];
    smk37_analog_playback(analog,0x3f);
    assert(trk[0].p[P_LEVEL]==level); /* startup does not mute */
    analog[0]=1023; analog[4]=639; analog[5]=411;
    smk37_analog_playback(analog,0x3f);
    assert(trk[0].p[P_LEVEL]==127 && midi_bend_target[1]==512 && trk[1].mw==127);
    analog[0]=0; analog[4]=404; analog[5]=630;
    smk37_analog_playback(analog,0x3f);
    assert(trk[0].p[P_LEVEL]==0 && midi_bend_target[1]==-512 && trk[1].mw==0);
    analog[4]=528; smk37_analog_playback(analog,0x3f);
    assert(!midi_bend_target[1]);
    puts("SMK37 controls: detents, track ownership, all engines/presets and page bounds passed");
    puts("SMK37 real synth: all 37 keys, velocity, audio, ownership and release passed");
    return 0;
}
