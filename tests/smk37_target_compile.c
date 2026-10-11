/* SPDX-License-Identifier: GPL-3.0-only */
/* Freestanding target compilation probe; no startup, MMIO, or flash writes.
 * External wrappers force code emission for the SMK pure control components.
 */
#include "../firmware/hal/smk37_contacts.h"
#include "../firmware/hal/smk37_response.h"
#include "../firmware/hal/smk37_trim.h"
#include "../firmware/hal/smk37_keyscan.h"
#include "../firmware/hal/smk37_audio.h"
#include "../firmware/hal/smk37_lcd_hw.h"
#include "../firmware/hal/smk37_panel.h"

smk37_contacts_t smk37_probe_decode(const uint8_t frame[SMK37_SCAN_SLOTS])
{
    return smk37_decode_contacts(frame);
}

uint8_t smk37_probe_velocity(unsigned response, enum smk37_response_curve curve,
                             uint8_t first, uint8_t last)
{
    return smk37_velocity_range(smk37_response_velocity(response, curve), first, last);
}

int smk37_probe_note(unsigned index, int8_t octave, int8_t transpose)
{
    return smk37_key_note(index, octave, transpose);
}

int smk37_probe_trim(const uint8_t *record, size_t size)
{
    return smk37_trim_keys_valid(record, size);
}

int smk37_probe_response(const uint8_t *record, unsigned key, uint32_t duration)
{
    return smk37_trim_key_response(record, key, duration);
}

int smk37_probe_tick(smk37_keyscan_t *state, unsigned raw, uint32_t *duration)
{
    return smk37_keyscan_tick(state, raw, duration);
}

int smk37_probe_audio(int32_t *buffer, uint32_t half_words, void (*isr)(void),
                      uint32_t priority, const smk37_audio_hooks_t *hooks)
{
    return smk37_audio_init(buffer, half_words, isr, priority, hooks);
}

unsigned smk37_probe_audio_irq(void)
{
    uint8_t pending = smk37_audio_pending();
    unsigned half = smk37_audio_free_half();
    smk37_audio_ack_aux(pending);
    if (pending & SMK37_AUDIO_HALF) smk37_audio_ack_half();
    return half;
}

void smk37_probe_audio_stop(void) { smk37_audio_stop(); }

int smk37_probe_lcd(uint32_t baud, const void *data, uint32_t bytes)
{
    smk37_lcd_hw_init(baud);
    return smk37_lcd_command(0x2c, 100000u) &&
           smk37_lcd_data(data, bytes, 100000u);
}

int smk37_probe_panel(const smk37_panel_io_t *io)
{
    return smk37_panel_init(io);
}
