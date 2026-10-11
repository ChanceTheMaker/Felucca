/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_events.h"
#include "../firmware/hal/smk37_keyscan.h"

int main(void)
{
    smk37_events_t q = {0};
    smk37_event_t e;
    smk37_keyscan_t keys[37] = {{0}};
    unsigned held[37] = {0};
    assert(!smk37_events_pop(&q, &e));
    assert(!smk37_events_push(&q, 37, 10));
    /* A full keyboard press/release survives delayed audio consumption,
     * including unsigned counter wrap and the scanner's contact timing. */
    q.write = q.read = UINT32_MAX - 20u;
    for (unsigned cycle = 0; cycle < 100; ++cycle) {
        for (unsigned phase = 0; phase < 2; ++phase)
            for (unsigned tick = 0; tick < 11; ++tick)
                for (unsigned k = 0; k < 37; ++k) {
                    uint32_t duration;
                    if (smk37_keyscan_tick(&keys[k], phase ? 0 : 3, &duration))
                        assert(smk37_events_push(&q, k, duration));
                }
        for (unsigned n = 0; n < 74; ++n) {
            assert(smk37_events_pop(&q, &e));
            assert(e.key == n % 37);
            assert(e.duration == (n < 37 ? 10u : 0u));
            held[e.key] = e.duration != 0;
        }
        assert(!smk37_events_pop(&q, &e));
        for (unsigned k = 0; k < 37; ++k) assert(!held[k]);
    }
    /* Loss latches even when space becomes available: incomplete history
     * cannot be replayed after the consumer's all-local-notes-off action. */
    for (unsigned i = 0; i < SMK37_EVENTS; ++i)
        assert(smk37_events_push(&q, i % 37, UINT32_MAX));
    assert(!smk37_events_push(&q, 0, 0));
    assert(q.overflow);
    assert(!smk37_events_pop(&q, &e));
    assert(!smk37_events_push(&q, 1, 1));
    smk37_events_discard(&q);
    assert(!q.overflow && q.read == q.write);
    assert(!smk37_events_pop(&q, &e));
    assert(smk37_events_push(&q, 36, UINT32_MAX));
    assert(smk37_events_pop(&q, &e));
    assert(e.key == 36 && e.duration == UINT32_MAX);
    assert(smk37_events_push(&q, 36, 0));
    assert(smk37_events_pop(&q, &e) && !e.duration);
    puts("SMK-37 scanner event queue, full-keyboard cycles, wrap and loss recovery: PASS");
    return 0;
}
