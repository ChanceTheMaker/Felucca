/* SPDX-License-Identifier: GPL-3.0-only */
#define main smk37_test21_baseline
#include "smk37_completion_test.c"
#undef main
int main(void){
    smk37_test21_baseline(0,0);seq_stop();song.rec=transport_req=0;chain.running=chain.armed=0;
    uint8_t eq[]={9,2,64};assert(req(73,eq,3));assert(fx_lowcut==2&&!smk37_ed_out[5]);
    fx_lowcut=0;assert(!smk37_up_boot()&&fx_lowcut==2);
    uint8_t route[]={21,1,64};assert(req(73,route,3));assert(song.g[G_ROUTE]==1&&!smk37_ed_out[5]);
    uint8_t leds[]={4,3,64};assert(req(73,leds,3));assert(smk37_led_level==3&&!smk37_ed_out[5]);
    /* Test20 saves have no EQ preference; do not read their reserved tail. */
    smk37_up.prefs[30]=0x20;smk37_up.prefs[21]=2;fx_lowcut=0;smk37_prefs_restore();assert(!fx_lowcut);
    smk37_trs_enabled=0;smk37_page=33;smk37_workflow_turn(6,1);assert(!smk37_trs_enabled); /* build without physical UART */
    puts("Test21 UI: speaker EQ persistence/migration, MIDI input routing, LED settings and disabled-build TRS gate passed");
}
