/* SPDX-License-Identifier: GPL-3.0-only */
#define FELUCCA_MONITOR 1
/* Audio ISR owns the active-note set; UI snapshots it with interrupts masked.
 * Track releases already account for MIDI sustain and shared input ownership. */
enum { MON_OFF, MON_EVENTS, MON_NOTES };
static volatile uint8_t monitor_mode;
typedef struct { uint32_t notes[4]; } monitor_notes_t;
typedef struct { uint32_t ms; uint8_t source, status, d1, d2, valid; } monitor_event_t;
static volatile monitor_notes_t monitor_notes[NTRK];
static volatile monitor_event_t monitor_event;
static void monitor_trigger(uint32_t track, uint32_t note, uint32_t velocity)
{
    if (track >= NTRK || note > 127u || !velocity) return;
    monitor_notes[track].notes[note / 32u] |= 1u << (note % 32u);
}
static void monitor_release(uint32_t track, uint32_t note)
{
    if (track >= NTRK || note > 127u) return;
    monitor_notes[track].notes[note / 32u] &= ~(1u << (note % 32u));
}
static void monitor_clear(uint32_t track)
{
    if (track >= NTRK) return;
    for (uint32_t i = 0; i < 4; i++) monitor_notes[track].notes[i] = 0;
}
static void monitor_receive(uint32_t packet, uint32_t source, uint32_t ms)
{
    uint32_t status = (packet >> 8) & 255u;
    if (monitor_mode != MON_EVENTS || source < 1u || source > 2u) return;
    /* Let notes/controllers/transport remain readable over a clock stream. */
    if (status == 0xF8u && monitor_event.valid && monitor_event.status != 0xF8u &&
        ms - monitor_event.ms < 500u) return;
    monitor_event.source = (uint8_t)source;
    monitor_event.status = (uint8_t)status;
    monitor_event.d1 = (uint8_t)((packet >> 16) & 127u);
    monitor_event.d2 = (uint8_t)((packet >> 24) & 127u);
    monitor_event.ms = ms;
    monitor_event.valid = 1;
}
