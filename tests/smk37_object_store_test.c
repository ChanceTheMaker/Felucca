/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_store.h"
#include "../firmware/hal/smk37_object_store.h"
static uint8_t flash[1<<20],backup[1<<20],old[11000],fresh[11000],out[11000];
static int remaining=-1,tear=-1,writes;
static int read_flash(uint32_t a,void *p,uint32_t n){assert(a<sizeof flash&&n<=sizeof flash-a);memcpy(p,flash+a,n);return 0;}
static void range(uint32_t a,uint32_t n){assert(a>=0xb1000&&a<0xbb000&&n<=0xbb000-a);}
static int erase(uint32_t a){range(a,4096);assert(!(a&4095));if(remaining==0)return -1;if(remaining>0)--remaining;++writes;memset(flash+a,255,4096);return 0;}
static int program(uint32_t a,const void *p,uint32_t n){range(a,n);assert(n<=256-(a&255));if(remaining==0)return -1;if(remaining>0)--remaining;++writes;
    unsigned count=n;if(tear>=0&&n==32)count=tear;
    for(unsigned i=0;i<count;++i){assert((flash[a+i]&((const uint8_t*)p)[i])==((const uint8_t*)p)[i]);flash[a+i]&=((const uint8_t*)p)[i];}
    return count==n?0:-1;
}
static const smk37_store_io_t io={read_flash,erase,program};
int main(void){
    memset(flash,0x69,sizeof flash);memset(old,0x11,sizeof old);memset(fresh,0x22,sizeof fresh);
    assert(smk37_object_save(&io,&smk37_object_presets,old,sizeof old,0));assert(!writes);
    smk37_object_t bad=smk37_object_presets;bad.a=0xc1000;
    assert(smk37_object_save(&io,&bad,old,sizeof old,1));assert(!writes);
    assert(!smk37_object_save(&io,&smk37_object_presets,old,sizeof old,1));memcpy(backup,flash,sizeof flash);
    for(int stop=0;stop<49;++stop){memcpy(flash,backup,sizeof flash);remaining=stop;
        int rc=smk37_object_save(&io,&smk37_object_presets,fresh,sizeof fresh,1);remaining=-1;
        assert(smk37_object_load(&io,&smk37_object_presets,out,sizeof out)>=0);assert(!memcmp(out,rc?old:fresh,sizeof out));
        assert(!memcmp(flash,backup,0xb1000));assert(!memcmp(flash+0xb7000,backup+0xb7000,sizeof flash-0xb7000));
    }
    for(tear=0;tear<32;++tear){memcpy(flash,backup,sizeof flash);assert(smk37_object_save(&io,&smk37_object_presets,fresh,sizeof fresh,1));assert(smk37_object_load(&io,&smk37_object_presets,out,sizeof out)>=0);assert(!memcmp(out,old,sizeof out));}
    tear=-1;memcpy(flash,backup,sizeof flash);assert(!smk37_object_save(&io,&smk37_object_presets,fresh,sizeof fresh,1));
    flash[smk37_object_presets.b+300]^=1;assert(smk37_object_load(&io,&smk37_object_presets,out,sizeof out)==0);assert(!memcmp(out,old,sizeof out));
    assert(!smk37_object_save(&io,&smk37_object_fm6,fresh,4096,1));assert(smk37_object_load(&io,&smk37_object_fm6,out,4096)>=0);assert(!memcmp(out,fresh,4096));
    /* Sequence wrap still chooses the newly committed generation. */
    smk37_object_header_t *h=(void*)(flash+smk37_object_presets.a);h->sequence=UINT32_MAX;h->header_crc=smk37_object_crc(h,28);
    memset(flash+smk37_object_presets.b,255,3*4096);assert(!smk37_object_save(&io,&smk37_object_presets,fresh,sizeof fresh,1));
    assert(smk37_object_load(&io,&smk37_object_presets,out,sizeof out)==1);assert(!memcmp(out,fresh,sizeof out));
    puts("SMK expanded objects: ownership gate, bounds, multi-sector interruptions, torn commit, CRC fallback, FM6 and generation wrap passed");
}
