/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_keyscan.h"

static unsigned run(smk37_keyscan_t *s, unsigned raw, unsigned ticks,
                    uint32_t *duration)
{
    unsigned events = 0;
    while (ticks--) events += smk37_keyscan_tick(s, raw, duration);
    return events;
}

int main(void)
{
    smk37_keyscan_t s = {0}, keys[37] = {{0}};
    uint32_t duration = 999;
    unsigned i;
    /* Single-tick noise and incomplete presses never generate notes. */
    for (i = 0; i < 100; ++i) {
        assert(!run(&s, 1, 1, &duration));
        assert(!run(&s, 0, 1, &duration));
    }
    assert(!run(&s, 1, 11, &duration));
    assert(!run(&s, 0, 11, &duration));
    assert(!s.held_duration);
    /* First observed tick + ten repeats; intermediate ticks add duration. */
    s = (smk37_keyscan_t){0};
    assert(!run(&s, 1, 11, &duration));
    assert(s.elapsed == 11);
    assert(!run(&s, 1, 20, &duration));
    assert(!run(&s, 3, 10, &duration));
    assert(run(&s, 3, 1, &duration) == 1 && duration == 41);
    assert(!run(&s, 3, 1000, &duration));
    /* Release through contact 1 and bounce back to 3: no duplicate press. */
    assert(!run(&s, 1, 11, &duration));
    assert(!run(&s, 3, 11, &duration));
    assert(run(&s, 0, 11, &duration) == 1 && duration == 0);
    /* Fast simultaneous contacts and subsequent retrigger. */
    assert(run(&s, 3, 11, &duration) == 1 && duration == 10);
    assert(run(&s, 0, 11, &duration) == 1 && duration == 0);
    /* Broken/reversed sequence cannot leave an active note hanging. */
    assert(run(&s, 3, 11, &duration) == 1);
    assert(run(&s, 2, 11, &duration) == 1 && duration == 0);
    assert(!run(&s, 0, 11, &duration));
    s.elapsed = UINT32_MAX;
    s.accepted = s.candidate = 1;
    assert(!run(&s, 1, 20, &duration));
    assert(s.elapsed == UINT32_MAX);
    for (i = 0; i < 37; ++i) {
        assert(run(&keys[i], 3, 11, &duration) == 1);
        assert(keys[i].held_duration == 10);
    }
    for (i = 0; i < 37; ++i)
        assert(run(&keys[i], 0, 11, &duration) == 1 && duration == 0);
    puts("SMK-37 key debounce, timing, release and retrigger: PASS");
    return 0;
}
