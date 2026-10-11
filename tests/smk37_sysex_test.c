/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#define RING_PUBLISH() __asm__ volatile("" ::: "memory")
#define FELUCCA_OTA 0
#define FELUCCA_SYSEX 1
#define FELUCCA_CDC 0
static void fm1_delay_ms(uint32_t ms){(void)ms;}
#include "../firmware/src/usb.c"
static uint32_t now;
static uint32_t ota_now_ms(void){return now;}
static void ota_idle(void){++now;}
static void feed(const uint8_t *p,unsigned n){for(unsigned i=0;i<n;++i)sysex_byte(p[i]);}
int main(void){
    const uint8_t request[]={0xf0,0x7d,0x46,0x4c,1,0xf7};
    feed(request,sizeof request);assert(sx_ready&&sx_frame_len==4&&sx_frame[3]==1);ota_frame_done();
    const uint8_t upgrade[]={0xf0,0x22,0x24,0x35,0x7f,0xf7};
    feed(upgrade,sizeof upgrade);assert(!usb.ota_req);ota_frame_done();
    const uint8_t interrupted[]={0xf0,0x7d,0x46,0x90,1,0xf7};
    feed(interrupted,sizeof interrupted);assert(!sx_ready);
    uint8_t oversized[650];memset(oversized,1,sizeof oversized);oversized[0]=0xf0;oversized[649]=0xf7;
    feed(oversized,sizeof oversized);assert(!sx_ready);
    midi_in_event(0x643c9009);assert(mi_w==1&&midi_in_q[0]==0x643c9009);
    usb.config=1;assert(!ota_wire_send(request,sizeof request));assert(so_w==2);
    assert(sx_out_q[0]==0x467df004 && sx_out_q[1]==0xf7014c07);
    puts("SMK USB: editor SysEx with OTA disabled, malformed frames, MIDI input and reply framing passed");
}
