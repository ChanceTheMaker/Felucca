#include <assert.h>
#include <string.h>
#include "../firmware/hal/smk37_rawscan.h"
int main(void) {
 smk37_rawscan_t s={0}; uint8_t f[16]; memset(f,255,sizeof f);
 smk37_rawscan_update(&s,f); assert(s.ready && s.changed_frames==0);
 for(unsigned i=0;i<16;i++) assert(s.current[i]==255 && s.changed_bits[i]==0);
 smk37_rawscan_update(&s,f); assert(s.changed_frames==0);
 for(unsigned i=0;i<16;i++) {
  f[i]^=1; smk37_rawscan_update(&s,f);
  f[i]^=1; smk37_rawscan_update(&s,f);
  assert(s.current[i]==255 && s.changed_bits[i]==1);
 }
 assert(s.changed_frames==32);
 f[0]^=2;f[15]^=128;smk37_rawscan_update(&s,f);
 assert(s.changed_frames==33 && s.changed_bits[0]==3 && s.changed_bits[15]==129);
 s.changed_frames=UINT32_MAX; f[0]^=4;smk37_rawscan_update(&s,f);assert(s.changed_frames==UINT32_MAX);
 return 0;
}
