/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments */
/* the presets of every engine, then the used user presets, as one list (the PRESETS knob and the PRESETS page browse it) */
static uint32_t preset_all_pos(uint32_t *total)      /* unfiltered list index */
{
    uint32_t n = 0, cur = 0, e;
    for (e = 0; e < NENGINES; e++) {
        if (e == TSEL->eng_req)
            cur = n + TSEL->preset % (ENGINES[e]->npresets ? ENGINES[e]->npresets : 1u);
        n += ENGINES[e]->npresets;
    }
    if (user_of(TSEL) < UP_SLOTS)
        cur = n + up_rank(user_of(TSEL));
    *total = n + up_count();
    return cur;
}

/* list index n (< total) -> engine, *k its preset; NENGINES = user preset, *k its slot */
static uint32_t preset_all_at(uint32_t n, uint32_t *k)
{
    uint32_t e;
    for (e = 0; e < NENGINES && n >= ENGINES[e]->npresets; e++)
        n -= ENGINES[e]->npresets;
    *k = e < NENGINES ? n : up_nth(n);
    return e;
}

static uint32_t preset_pos(uint32_t *total)
{
    uint32_t all, current = preset_all_pos(&all), n = 0, pos = 0xFFFFFFFFu;
    if (!favorites.filter) { *total = all; return current; }
    for (uint32_t i = 0; i < all; i++) {
        uint32_t k, e = preset_all_at(i, &k);
        if (!favorite_has(e, k)) continue;
        if (i == current) pos = n;
        n++;
    }
    *total = n;
    return pos == 0xFFFFFFFFu ? n : pos; /* current sound need not be a favorite */
}
static uint32_t preset_at(uint32_t n, uint32_t *k)
{
    uint32_t all;
    if (!favorites.filter) return preset_all_at(n, k);
    preset_all_pos(&all);
    for (uint32_t i = 0; i < all; i++) {
        uint32_t e = preset_all_at(i, k);
        if (favorite_has(e, *k) && !n--) return e;
    }
    *k = UP_SLOTS;
    return NENGINES;
}
static int preset_favorite(void)
{
    uint32_t k = user_of(TSEL);
    return !is_drum(TSEL) && favorite_has(k < UP_SLOTS ? NENGINES : TSEL->eng_req,
                                        k < UP_SLOTS ? k : TSEL->preset);
}
static void preset_mark(int on)
{
    uint32_t k = user_of(TSEL);
    if (is_drum(TSEL)) return;
    if (favorite_set(k < UP_SLOTS ? NENGINES : TSEL->eng_req, k < UP_SLOTS ? k : TSEL->preset, on)) {
        ui.force = 1;
        settings_save();
    }
}

static void preset_go(uint32_t n)                    /* load list index n into the selected track */
{
    uint32_t k, e = preset_at(n, &k);
    if (is_drum(TSEL))
        return;                                      /* one GM kit: nothing to browse */
    if (e == NENGINES) {
        up_load(k);
        return;
    }
    if (e != TSEL->eng_req)
        select_engine(e);
    apply_preset(k);
    ui.force = 1;
}
static void preset_step(int32_t direction)
{
    uint32_t total, cur = preset_pos(&total);
    if (!total) { ui_message("NO FAVORITES"); return; }
    preset_go(cur >= total ? (direction > 0 ? 0 : total - 1) :
              (cur + (direction > 0 ? 1u : total - 1u)) % total);
}

/* Seven display rows. Favorites use a bounded window, not a repeating carousel.
 * Return total for an empty row; a non-favorite current sound shows the start. */
static uint32_t preset_visible(uint32_t cur, uint32_t total, uint32_t row)
{
    uint32_t first, last;
    if (!total || row >= 7u) return total;
    if (!favorites.filter)
        return (cur + total * 4u + row - 3u) % total;
    first = cur < total && cur > 3u ? cur - 3u : 0u;
    last = total > 7u ? total - 7u : 0u;
    if (first > last) first = last;
    return first + row < total ? first + row : total;
}
