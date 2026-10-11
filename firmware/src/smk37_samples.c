/* SPDX-License-Identifier: GPL-3.0-only
 * FSMP / pack7 wire format from editor.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments. SMK storage remains separately gated. */
#include "smk37_sample_store.h"
static smk37_sample_upload_t smk37_upload[3];
static uint8_t smk37_sample_buffer[512] __attribute__((aligned(4)));
static void smk37_sample_invalidate(unsigned k);
static int smk37_sample_format(void);
static unsigned smk37_sample_unpack(const uint8_t *a,unsigned n,uint8_t *out,unsigned max){
    unsigned count=0;
    while(n){unsigned mask=*a++,take=--n>7?7:n;if(!take||count+take>max||(mask>>take))return 0;
        for(unsigned i=0;i<take;++i){out[count++]=*a++|((mask>>i)&1)<<7;--n;}}
    return count;
}
static void smk37_sample_unpublish(unsigned k){
    fm1_irq_off();usr_nz[k]=0;for(unsigned j=0;j<16;++j)usr_zone[k][j].n=0;++smp_user_gen;fm1_irq_on();
    slc_user_scan(k,0);
}
static int smk37_sample_header_valid(const smp_user_hdr_t *h){
    if(h->magic!=SMP_USER_MAGIC||h->version!=1||!h->nz||h->nz>16||!h->data_len||h->data_len>SMP_USER_SIZE-SMP_USER_DATA)return 0;
    for(unsigned i=0;i<h->nz;++i){const smp_zone_t *z=&h->zone[i];
        if(z->off>h->data_len||!z->n||z->n>2*SMP_USER_SIZE||(z->n+1)/2>h->data_len-z->off||z->idx>88||!z->rate||z->rate>(4u<<16)||z->ls>z->le||z->le>=z->n||z->lo>z->hi||z->hi>127||z->looped>1)return 0;
    }return 1;
}
static unsigned smk37_sample_end(unsigned k,const uint8_t *a,unsigned n){
    smp_user_hdr_t *h=(void *)smk37_sample_buffer;
    if(smk37_sample_unpack(a,n,smk37_sample_buffer,sizeof *h)!=sizeof *h)return 1;
    if(!smk37_sample_header_valid(h))return 2;
    if(usr_nz[k])return memcmp(h,smp_user_xip(k),sizeof *h)?2:0;
    if(!smk37_upload[k].active||smk37_upload[k].next!=SMP_USER_DATA+h->data_len)return 2;
    smk37_sample_invalidate(k);
    if(smk37_store_crc(smp_user_xip(k)+SMP_USER_DATA,h->data_len)!=h->crc)return 3;
    uint32_t base=smk37_sample_address(k);uint8_t verify[256];
    /* Write the zone tail first. A interrupted header never publishes magic. */
    if(smk37_storage_program(base+256,smk37_sample_buffer+256,sizeof *h-256)||
       smk37_storage_read(base+256,verify,sizeof *h-256)||memcmp(verify,smk37_sample_buffer+256,sizeof *h-256)||
       smk37_storage_program(base+4,smk37_sample_buffer+4,252)||
       smk37_storage_read(base+4,verify,252)||memcmp(verify,smk37_sample_buffer+4,252)||
       smk37_storage_program(base,smk37_sample_buffer,4))return 4;
    smk37_sample_invalidate(k);smp_user_scan(k);
    if(!usr_nz[k])return 5;smk37_upload[k].active=0;return 0;
}
/* Same sample-bound slice record as FM-1; only its physical address differs. */
static int smk37_slice_program(uint32_t a,const void *p,uint32_t n){
    uint8_t verify[256];if(n>sizeof verify||smk37_storage_program(a,p,n)||smk37_storage_read(a,verify,n)||memcmp(p,verify,n))return -1;
    for(unsigned k=0;k<3;++k)if(a>=smk37_sample_address(k)&&a<smk37_sample_address(k)+SMP_USER_SIZE)smk37_sample_invalidate(k);
    return 0;
}
#define SLC_STORE_SLOT(k) smk37_sample_address(k)
#define st_crc32 smk37_store_crc
#define st_erase smk37_storage_erase
#define st_prog smk37_slice_program
#define flash_ok (smk37_samples_owned && smk37_storage_writable)
#define transport_busy() (!smk37_up_idle())
#define slice_page_on() (smk37_page==SMK37_SLICE_PAGE)
#include "slice_store.c"
#undef slice_page_on
#undef transport_busy
#undef flash_ok
#undef st_prog
#undef st_erase
#undef st_crc32
#undef SLC_STORE_SLOT
