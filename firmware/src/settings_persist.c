/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared PER4 layout: each feature updates only its own fields, preserving
 * the other feature's saved preferences when either is built independently.
 * PER1/PER2 are upstream; PER3 added bold; PER4 added favorites. */
typedef struct {
    uint32_t magic, palette, lowcut, zoom;
    panel_t panel;
    uint32_t bold;
    struct { uint8_t factory[16][32]; uint32_t user, filter; } favorites;
} persist_t;
#define PERSIST_MAGIC 0x50455234u

/* Normalize in place; 1 = current, 2 = migrated, 0 = invalid. */
static int settings_import(persist_t *p, int n)
{
    int current = n == (int)sizeof *p && p->magic == PERSIST_MAGIC;
    int old3 = n == (int)(sizeof *p - sizeof p->favorites) && p->magic == 0x50455233u;
    int old2 = n == (int)(sizeof *p - sizeof p->favorites - sizeof p->bold) && p->magic == 0x50455232u;
    int old1 = n == (int)(8u + sizeof(panel_t)) && p->magic == 0x50455231u;
    if (!(current || old3 || old2 || old1)) return 0;
    if (old1) {
        panel_t old;
        memcpy(&old, (uint8_t *)p + 8, sizeof old);
        p->panel = old;
        p->lowcut = p->zoom = 0;
    }
    if (old1 || old2) p->bold = 0;
    if (!current) memset(&p->favorites, 0, sizeof p->favorites);
    p->magic = PERSIST_MAGIC;
    settings.magic = SETTINGS_MAGIC;
    settings.palette = p->palette;
    settings.lowcut = p->lowcut;
    settings.zoom = p->zoom;
#ifdef FELUCCA_FONT_PREF
    settings.bold = p->bold == 1u;
#endif
#ifdef FELUCCA_FAVORITES
    memcpy(&favorites, &p->favorites, sizeof favorites);
    favorites.filter = favorites.filter == 1u;
#endif
    if (p->panel.magic == PANEL_MAGIC) panel = p->panel;
    return current ? 1 : 2;
}

/* Start with the last imported/saved object, including fields owned by a
 * feature absent from this build. No save-on-boot or extra flash writes. */
static void settings_export(persist_t *p)
{
    p->magic = PERSIST_MAGIC;
    p->palette = settings.palette;
    p->lowcut = settings.lowcut;
    p->zoom = settings.zoom;
    p->panel = panel;
#ifdef FELUCCA_FONT_PREF
    p->bold = settings.bold;
#endif
#ifdef FELUCCA_FAVORITES
    memcpy(&p->favorites, &favorites, sizeof favorites);
#endif
}
