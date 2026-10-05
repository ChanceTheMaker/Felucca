/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#define NENGINES 3u
#define UP_SLOTS 32u
typedef struct { uint32_t eng_req, preset, user, drum; } track_t;
static track_t track;
#define TSEL (&track)
static struct { unsigned force; } ui;
static struct { uint32_t npresets; } engine[] = {{3}, {2}, {1}};
static typeof(engine[0]) *ENGINES[] = {engine, engine+1, engine+2};
static uint32_t users, saves;
static int is_drum(track_t *t) { return t->drum; }
static uint32_t user_of(track_t *t) { return t->user ? t->user-1 : UP_SLOTS; }
static uint32_t up_count(void) { return __builtin_popcount(users); }
static uint32_t up_rank(uint32_t k) { return __builtin_popcount(users & ((1u << k)-1u)); }
static uint32_t up_nth(uint32_t n) {
    for (unsigned k=0; k<UP_SLOTS; k++) if ((users >> k & 1u) && !n--) return k;
    return UP_SLOTS;
}
static int up_load(uint32_t k) { assert(k < UP_SLOTS && (users >> k & 1)); track.user=k+1; return 1; }
static void select_engine(uint32_t e) { track.eng_req=e; track.user=0; }
static void apply_preset(uint32_t k) { track.preset=k; track.user=0; }
static void settings_save(void) { saves++; }
static void ui_message(const char *s) { (void)s; }
#include "../firmware/src/favorites.c"
#include "../firmware/src/preset_browser.c"
int main(void) {
    uint32_t total, k;
    favorites.filter=1;
    /* Short lists must not repeat; longer lists scroll and keep selection visible. */
    for (unsigned count=0; count<=12; count++) {
        for (unsigned current=0; current<=count; current++) {
            unsigned seen=0, visible=0;
            for (unsigned row=0; row<7; row++) {
                unsigned index=preset_visible(current,count,row);
                if (index==count) continue;
                assert(index<count && !(seen & (1u<<index)));
                seen |= 1u<<index; visible++;
            }
            assert(visible==(count<7 ? count : 7));
            if (current<count) assert(seen & (1u<<current));
        }
    }
    favorites.filter=0;
    assert(preset_visible(0,8,3)==0 && preset_visible(0,8,2)==7);
    users = (1u << 4) | (1u << 31);
    assert(preset_pos(&total)==0 && total==8);
    preset_mark(1); assert(saves==1 && preset_favorite());
    preset_mark(1); assert(saves==1); /* repeated detents do not write flash */
    favorite_set(2,0,1); favorite_set(NENGINES,31,1);
    favorites.filter=1;
    assert(preset_pos(&total)==0 && total==3);
    assert(preset_at(1,&k)==2 && k==0);
    preset_step(-1); assert(track.user==32);
    preset_step(1); assert(track.user==0 && track.eng_req==0 && track.preset==0);
    preset_step(1); assert(track.eng_req==2);
    preset_step(1); assert(track.user==32);
    preset_mark(0); assert(!preset_favorite());
    uint32_t pos = preset_pos(&total);
    assert(pos==total && total==2);
    preset_step(-1); assert(track.user==0 && track.eng_req==2);
    select_engine(1); /* selecting an unstarred engine must not underflow the list */
    preset_step(1); assert(track.eng_req==0);
    favorite_set(NENGINES,4,1); users &= ~(1u<<4);
    preset_pos(&total); assert(total==2); /* deleted slots are absent immediately */
    favorite_set(NENGINES,4,0); users |= 1u<<4;
    assert(!favorite_has(NENGINES,4)); /* reusing a deleted slot does not resurrect its star */
    favorite_set(NENGINES,31,1); /* overwrite/rename retains the slot reference */
    assert(favorite_has(NENGINES,31));
    favorites_t saved=favorites;
    memset(&favorites,0,sizeof favorites); favorites=saved;
    assert(favorites.filter && favorite_has(2,0) && favorite_has(NENGINES,31));
    assert(!favorite_set(NENGINES,32,1) && !favorite_set(0,256,1));
    memset(&favorites,0,sizeof favorites); favorites.filter=1;
    track_t before=track; preset_step(1);
    assert(!memcmp(&before,&track,sizeof track));
    track.drum=1; preset_mark(1); assert(!preset_favorite());
    puts("Favorites: filtering, wrap, empty lists, slot reuse, references, write suppression passed.");
}
