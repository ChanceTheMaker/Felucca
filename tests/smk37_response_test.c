/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_response.h"

int main(void)
{
    unsigned curve, response, i, a, b;
    for (curve = 0; curve < 4; ++curve) {
        unsigned previous = 0;
        assert(smk37_response_velocity(0, curve) == 0);
        for (response = 1; response <= 1023; ++response) {
            unsigned v = smk37_response_velocity(response, curve);
            assert(v >= 1 && v <= 127 && v >= previous);
            previous = v;
        }
        assert(previous == 127);
        assert(smk37_response_velocity(65535, curve) == 127);
    }
    assert(smk37_response_velocity(512, SMK37_RESPONSE_LINEAR) == 64);
    assert(smk37_response_velocity(512, SMK37_RESPONSE_BOOST_32) == 80);
    assert(smk37_response_velocity(512, SMK37_RESPONSE_BOOST_64) == 96);
    assert(smk37_response_velocity(1, SMK37_RESPONSE_FIXED) == 127);
    for (a = 0; a <= 127; ++a) for (b = 0; b <= 127; ++b) {
        unsigned previous = smk37_velocity_range(1, a, b);
        assert(smk37_velocity_range(0, a, b) == 0);
        for (i = 2; i <= 127; ++i) {
            unsigned v = smk37_velocity_range(i, a, b);
            assert(v >= 1 && v <= 127);
            assert(b >= a ? v >= previous : v <= previous);
            previous = v;
        }
    }
    assert(smk37_velocity_range(64, 0, 127) == 64);
    assert(smk37_velocity_range(127, 0, 127) == 127);
    assert(smk37_velocity_range(127, 127, 0) == 1);
    for (i = 0; i < 37; ++i) assert(smk37_key_note(i, 0, 0) == (int)i + 48);
    assert(smk37_key_note(0, 1, -1) == 59);
    assert(smk37_key_note(36, 127, 127) == 120);
    assert(smk37_key_note(0, -128, -128) == 0);
    assert(smk37_key_note(37, 0, 0) == -1);
    puts("SMK-37 response conversion: PASS");
    return 0;
}
