/* SPDX-License-Identifier: GPL-3.0-only */
/* Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hugelton Instruments */
typedef struct {
    uint32_t magic, palette, lowcut, zoom;
    panel_t panel;
    uint32_t bold;                                /* appended: PER2 prefix stays intact */
    favorites_t favorites;
} persist_t;
#define PERSIST_MAGIC 0x50455234u                  /* "PER4" */

/* Returns 1 for current format, 2 for migration, 0 for invalid data. */
static int settings_import(const persist_t *p, int n)
{
    int old3 = n == (int)(sizeof *p - sizeof p->favorites) && p->magic == 0x50455233u;
    int legacy = n == (int)(sizeof *p - sizeof p->favorites - sizeof p->bold) && p->magic == 0x50455232u;
    if ((n == (int)sizeof *p && p->magic == PERSIST_MAGIC) || legacy || old3) {
        settings.magic = SETTINGS_MAGIC;
        settings.palette = p->palette;
        settings.lowcut = p->lowcut;
        settings.zoom = p->zoom;
        settings.bold = legacy ? 0u : p->bold == 1u;
        memset(&favorites, 0, sizeof favorites);
        if (!legacy && !old3) {
            favorites = p->favorites;
            favorites.filter = favorites.filter == 1u;
        }
        if (p->panel.magic == PANEL_MAGIC)
            panel = p->panel;
        return !legacy && !old3 ? 1 : 2;
    } else if (n == (int)(8u + sizeof(panel_t)) && p->magic == 0x50455231u) {   /* "PER1": palette, panel */
        const uint32_t *w = (const uint32_t *)p;
        panel_t old;
        memcpy(&old, w + 2, sizeof old);
        settings.magic = SETTINGS_MAGIC;
        settings.palette = w[1];
        settings.lowcut = 0;
        settings.zoom = 0;
        settings.bold = 0;
        if (old.magic == PANEL_MAGIC)
            panel = old;
        memset(&favorites, 0, sizeof favorites);
        return 2;
    }
    return 0;
}
