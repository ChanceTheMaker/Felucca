/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include "../firmware/hal/smk37_panel.h"
static unsigned calls, fail_at, black_bytes, on, delays, command_count;
static uint8_t last, commands[32];
static int step(void) { return ++calls != fail_at; }
static int command(uint8_t c)
{
    if (!step()) return 0;
    assert(command_count < 32);
    commands[command_count++] = last = c;
    if (c == 0x29) { assert(black_bytes == 115200); ++on; }
    return 1;
}
static int data(const void *p, uint32_t n)
{
    const uint8_t *b = p;
    unsigned i;
    if (!step()) return 0;
    if (last == 0x2a || last == 0x2b) {
        assert(n == 4 && b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == 239);
    } else if (last == 0x2c) {
        for (i = 0; i < n; ++i) assert(b[i] == 0);
        black_bytes += n;
    } else if (last == 0x3a) assert(n == 1 && b[0] == 5);
    return 1;
}
static void delay(uint32_t n)
{
    static const unsigned expected[] = {100,120,50};
    assert(delays < 3 && n == expected[delays++]);
}
static void reset(unsigned failure)
{
    calls = black_bytes = on = delays = command_count = 0;
    fail_at = failure;
}
int main(void)
{
    const smk37_panel_io_t io = {command,data,delay};
    unsigned total, failure;
    reset(0);
    assert(smk37_panel_init(&io));
    total = calls;
    assert(on == 1 && delays == 3 && black_bytes == 115200);
    assert(commands[0] == 0x11 && commands[command_count-1] == 0x29);
    for (failure = 1; failure <= total; ++failure) {
        reset(failure);
        assert(!smk37_panel_init(&io));
        assert(calls == failure && on == 0);
    }
    reset(0);
    assert(!smk37_panel_window(&io,240,0,1,1));
    assert(!smk37_panel_window(&io,0,0,0,240));
    assert(!smk37_panel_window(&io,239,0,2,1));
    assert(!smk37_panel_window(&io,0,0,~0u,1));
    assert(calls == 0);
    assert(!smk37_panel_init(0));
    puts("SMK-37 panel sequence, clear, bounds and injected failures: PASS");
    return 0;
}
