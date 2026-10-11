/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_contacts.h"

static void close_contact(uint8_t *frame, unsigned code) {
    frame[code >> 4] &= (uint8_t)~(1u << (code & 15));
}
int main(void) {
    uint8_t frame[16], used[256] = {0};
    unsigned i, side, count = 0;
    smk37_contacts_t s;
    for (i = 0; i < 37; ++i) for (side = 0; side < 2; ++side) {
        unsigned code = smk37_key_contacts[i][side];
        assert((code & 15) < 8 && !used[code]++);
        memset(frame, 255, sizeof(frame)); close_contact(frame, code);
        s = smk37_decode_contacts(frame);
        assert(s.keys_a == (side == 0 ? UINT64_C(1) << i : 0));
        assert(s.keys_b == (side == 1 ? UINT64_C(1) << i : 0));
        assert(!s.buttons && !s.encoders); ++count;
    }
    for (i = 0; i < 8; ++i) for (side = 0; side < 2; ++side) {
        unsigned code = smk37_encoder_contacts[i][side];
        assert((code & 15) < 8 && !used[code]++);
        memset(frame, 255, sizeof(frame)); close_contact(frame, code);
        s = smk37_decode_contacts(frame);
        assert(s.encoders == (1u << (2*i+side)));
        assert(!s.keys_a && !s.keys_b && !s.buttons); ++count;
    }
    for (i = 0; i < 18; ++i) {
        unsigned code = smk37_button_contacts[i];
        assert((code & 15) < 8 && !used[code]++);
        memset(frame, 255, sizeof(frame)); close_contact(frame, code);
        s = smk37_decode_contacts(frame);
        assert(s.buttons == (1u << i));
        assert(!s.keys_a && !s.keys_b && !s.encoders); ++count;
    }
    assert(count == 108);
    /* Unmapped contacts cannot generate controls. */
    for (i = 0; i < 256; ++i) if ((i & 15) < 8 && !used[i]) {
        memset(frame, 255, sizeof(frame)); close_contact(frame, i);
        s = smk37_decode_contacts(frame);
        assert(!s.keys_a && !s.keys_b && !s.buttons && !s.encoders);
    }
    /* All keys held, including 32..36, plus all other controls. */
    memset(frame, 0, sizeof(frame)); s = smk37_decode_contacts(frame);
    assert(s.keys_a == (UINT64_C(1)<<37)-1 && s.keys_b == s.keys_a);
    assert(s.buttons == (1u<<18)-1 && s.encoders == 65535);
    memset(frame, 255, sizeof(frame)); s = smk37_decode_contacts(frame);
    assert(!s.keys_a && !s.keys_b && !s.buttons && !s.encoders);
    puts("SMK37 contacts: 108 isolated contacts, unused inputs, simultaneous hold and release pass");
    return 0;
}
