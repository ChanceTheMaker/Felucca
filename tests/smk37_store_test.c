/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../firmware/hal/smk37_store.h"
static uint8_t flash[0x100000],baseline[0x100000],old[3648],new_data[3648],readback[3648],previous[3648];
static int allowance=-1;
static int header_cut=-1;
static int rd(uint32_t a,void *p,uint32_t n){assert(smk37_store_range(a,n));memcpy(p,flash+a,n);return 0;}
static int operation(void){if(allowance<0)return 0;if(!allowance)return -1;--allowance;return 0;}
static int erase(uint32_t a){assert(!(a&4095) && smk37_store_range(a,4096));if(operation())return -1;memset(flash+a,255,4096);return 0;}
static int prog(uint32_t a,const void *p,uint32_t n){assert(smk37_store_range(a,n)&&n<=256&&(a&255)+n<=256);if(operation())return -1;int cut=!(a&4095)&&header_cut>=0;unsigned bytes=cut?(unsigned)header_cut:n;for(unsigned i=0;i<bytes;++i)flash[a+i]&=((const uint8_t *)p)[i];return cut?-1:0;}
int main(void){
    smk37_store_io_t io={rd,erase,prog};
    memset(flash,255,sizeof flash);memset(old,13,sizeof old);memset(new_data,42,sizeof new_data);
    assert(smk37_store_allocation_ok(&io,sizeof old));
    flash[SMK37_STORE_LO+256]=0; /* unowned/interrupted first save: fail closed */
    assert(!smk37_store_allocation_ok(&io,sizeof old));
    flash[SMK37_STORE_LO+256]=255;
    assert(!smk37_store_range(0,4)&&!smk37_store_range(0xc1000,4096)&&!smk37_store_range(0xf3000,1));
    assert(!smk37_store_range(0xeb000,UINT32_MAX));
    for(unsigned slot=0;slot<4;++slot)assert(!smk37_store_save(&io,slot,old,sizeof old,previous));
    memcpy(baseline,flash,sizeof flash);
    assert(smk37_store_allocation_ok(&io,sizeof old));
    /* Cut before each erase/program stage, including the final commit. */
    for(unsigned slot=0;slot<4;++slot)for(int cut=0;cut<19;++cut){
        memcpy(flash,baseline,sizeof flash);allowance=cut;
        int rc=smk37_store_save(&io,slot,new_data,sizeof new_data,previous);
        allowance=-1;
        assert(smk37_store_load(&io,slot,readback,sizeof readback,0)>=0);
        assert(!memcmp(readback,rc?old:new_data,sizeof readback));
        assert(smk37_store_allocation_ok(&io,sizeof old));
        assert(!memcmp(flash,baseline,SMK37_STORE_LO));
        assert(!memcmp(flash+SMK37_STORE_HI,baseline+SMK37_STORE_HI,sizeof flash-SMK37_STORE_HI));
    }
    for(header_cut=0;header_cut<32;++header_cut){
        memcpy(flash,baseline,sizeof flash);
        assert(smk37_store_save(&io,0,new_data,sizeof new_data,previous));
        assert(smk37_store_load(&io,0,readback,sizeof readback,0)>=0);
        assert(!memcmp(readback,old,sizeof old));
    }
    header_cut=-1;memcpy(flash,baseline,sizeof flash);
    assert(!smk37_store_save(&io,0,new_data,sizeof new_data,previous));
    flash[SMK37_STORE_LO+4096+256]^=1; /* newest payload corrupt -> older copy */
    assert(smk37_store_load(&io,0,readback,sizeof readback,0)==0 && !memcmp(readback,old,sizeof old));
    assert(smk37_store_save(&io,4,old,sizeof old,previous));
    /* New FUN10 session, including migration from a committed Test12 copy.
     * Every program call must remain within a physical 256-byte flash page. */
    static uint8_t big[3904], migrated[3904], got[3904], scratch[3904];
    memset(big,73,sizeof big);memset(migrated,0,sizeof migrated);
    memcpy(migrated,old,3584);memcpy(migrated+3840,old+3584,64);
    for(unsigned slot=0;slot<4;++slot)for(int cut=0;cut<20;++cut){
        memcpy(flash,baseline,sizeof flash);allowance=cut;
        int rc=smk37_store_save(&io,slot,big,sizeof big,scratch);allowance=-1;
        assert(smk37_store_load(&io,slot,got,sizeof got,0)>=0);
        assert(!memcmp(got,rc?migrated:big,sizeof got));
        assert(smk37_store_allocation_ok(&io,sizeof big));
        assert(!memcmp(flash,baseline,SMK37_STORE_LO));
        assert(!memcmp(flash+SMK37_STORE_HI,baseline+SMK37_STORE_HI,sizeof flash-SMK37_STORE_HI));
    }
    for(header_cut=0;header_cut<32;++header_cut){
        memcpy(flash,baseline,sizeof flash);
        assert(smk37_store_save(&io,0,big,sizeof big,scratch));
        assert(smk37_store_load(&io,0,got,sizeof got,0)>=0 && !memcmp(got,migrated,sizeof got));
    }
    header_cut=-1;memcpy(flash,baseline,sizeof flash);
    assert(!smk37_store_save(&io,0,big,sizeof big,scratch));
    /* Once both copies use v2, an interrupted replacement still preserves it. */
    assert(!smk37_store_save(&io,0,big,sizeof big,scratch));
    memcpy(baseline,flash,sizeof flash);memset(migrated,91,sizeof migrated);
    for(int cut=0;cut<20;++cut){
        memcpy(flash,baseline,sizeof flash);allowance=cut;
        int rc=smk37_store_save(&io,0,migrated,sizeof migrated,scratch);allowance=-1;
        assert(smk37_store_load(&io,0,got,sizeof got,0)>=0);
        assert(!memcmp(got,rc?big:migrated,sizeof got));
    }
    puts("SMK storage: v1/v2 migration, all interruption stages, page boundaries and protected ranges passed");
}
