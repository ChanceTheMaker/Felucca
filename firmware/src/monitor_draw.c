/* SPDX-License-Identifier: GPL-3.0-only */
static void mon_append(char *out, const char *s)
{
    uint32_t n = str_len(out);
    str_cpy(out + n, s, 32u - n);
}
static void mon_number(char *out, int32_t n)
{
    char s[12];
    fmt_int(s, n);
    mon_append(out, s);
}
static void monitor_text(char *out)
{
    monitor_notes_t notes;
    monitor_event_t event;
    uint32_t mode = monitor_mode;
    out[0] = 0;
    if (!mode) return;
    fm1_irq_off();
    notes = monitor_notes[song.sel];
    event = monitor_event;
    fm1_irq_on();
    if (mode == MON_NOTES) {
        uint32_t count = 0, shown = 0;
        for (uint32_t n = 0; n < 128; n++) {
            char name[8];
            if (!(notes.notes[n / 32u] & (1u << (n % 32u)))) continue;
            count++;
            if (shown == 4u) continue;
            if (shown) mon_append(out, " ");
            note_name(name, n);
            mon_append(out, name);
            shown++;
        }
        if (count > shown) { mon_append(out, " +"); mon_number(out, count - shown); }
        return;
    }
    if (!event.valid || (uint32_t)(fm1_ms - event.ms) >= 800u) return;
    mon_append(out, event.source == 1 ? "USB " : "TRS ");
    if (event.status >= 0xF8u) {
        if (event.status == 0xF8u) {
            mon_append(out, "CLK");
            if (event.source == song.g[G_CLOCK]) { mon_append(out, " "); mon_number(out, song.g[G_BPM]); }
        } else mon_append(out, event.status == 0xFAu ? "START" : event.status == 0xFBu ? "CONT" : "STOP");
    } else {
        uint32_t st = event.status & 0xF0u;
        mon_append(out, "CH"); mon_number(out, (event.status & 15u) + 1u); mon_append(out, " ");
        if (st == 0x90u || st == 0x80u) {
            char name[8]; note_name(name, event.d1); mon_append(out, name);
            if (st == 0x80u || !event.d2) mon_append(out, " OFF");
            else { mon_append(out, " V"); mon_number(out, event.d2); }
        } else if (st == 0xB0u) {
            mon_append(out, "CC"); mon_number(out, event.d1); mon_append(out, " "); mon_number(out, event.d2);
        } else if (st == 0xE0u) {
            mon_append(out, "BEND "); mon_number(out, (int32_t)(event.d1 | event.d2 << 7) - 8192);
        } else {
            mon_append(out, st == 0xC0u ? "PGM " : "PRESS ");
            mon_number(out, st == 0xA0u ? event.d2 : event.d1);
        }
    }
}
static void draw_monitor(void)
{
    char text[32];
    int32_t x;
    monitor_text(text);
    if (!text[0]) return;
    x = 236 - text_w(&FONT_S, text);
    if (x < 4) x = 4;
    cv_rect(x - 2, H_GRAPH - FONT_S.h - 4, 240 - x + 2, FONT_S.h + 4, C_BLACK);
    cv_text(x, H_GRAPH - FONT_S.h - 2, &FONT_S, text, C_HI);
}
