/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_trim.h"

int main(void)
{
    /* Synthetic fixture using stock defaults, NOT the owner's calibration. */
    static const unsigned points[] = {5,640,800,920,980};
    uint8_t record[SMK37_TRIM_RECORD_BYTES], before[SMK37_TRIM_RECORD_BYTES];
    unsigned key, j, duration;
    memset(record, 0xa5, sizeof(record));
    memcpy(record + SMK37_TRIM_DATA_BYTES, "trimdat", 8);
    for (key = 0; key < 37; ++key) for (j = 0; j < 5; ++j) {
        unsigned offset = SMK37_TRIM_KEYS_OFFSET + key * 10 + j * 2;
        record[offset] = points[j] & 255;
        record[offset + 1] = points[j] >> 8;
    }
    memcpy(before, record, sizeof(record));
    assert(smk37_trim_keys_valid(record, sizeof(record)));
    assert(!smk37_trim_keys_valid(NULL, sizeof(record)));
    for (j = 0; j < sizeof(record); ++j)
        assert(!smk37_trim_keys_valid(record, j));
    for (key = 0; key < 37; ++key) {
        int previous = 1023;
        assert(smk37_trim_key_response(record, key, 0) == 0);
        for (duration = 1; duration < 2048; ++duration) {
            int response = smk37_trim_key_response(record, key, duration);
            assert(response >= 1 && response <= 1023 && response <= previous);
            previous = response;
        }
        assert(smk37_trim_key_response(record, key, 1023-640) == 256);
        assert(smk37_trim_key_response(record, key, 1023-800) == 512);
        assert(smk37_trim_key_response(record, key, 1023-920) == 768);
        assert(smk37_trim_key_response(record, key, 1023-980) == 1023);
        assert(smk37_trim_key_response(record, key, UINT32_MAX) == 1);
    }
    assert(memcmp(record, before, sizeof(record)) == 0);
    assert(smk37_trim_key_response(record, 37, 100) == -1);
    for (j = 0; j < 8; ++j) {
        record[548+j] ^= 1;
        assert(!smk37_trim_keys_valid(record, sizeof(record)));
        record[548+j] ^= 1;
    }
    for (key = 0; key < 37; ++key) {
        unsigned offset = SMK37_TRIM_KEYS_OFFSET + key * 10;
        record[offset + 2] = record[offset];
        record[offset + 3] = record[offset + 1];
        assert(!smk37_trim_keys_valid(record, sizeof(record)));
        assert(smk37_trim_key_response(record, key, 100) == -1);
        memcpy(record, before, sizeof(record));
    }
    memset(record + SMK37_TRIM_KEYS_OFFSET, 0xff, 370);
    assert(!smk37_trim_keys_valid(record, sizeof(record)));
    puts("SMK-37 trim decoding and key response: PASS");
    return 0;
}
