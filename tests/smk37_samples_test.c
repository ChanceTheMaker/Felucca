/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define SMK37_SYNTH_HOST_TEST 1
#define SMK37_ENABLE_SAMPLES 1
#include "../firmware/src/smk37_synth.c"
static void midi_out_event(uint32_t p){(void)p;}
static unsigned locked;
static void fm1_irq_off(void){locked=1;}
static void fm1_irq_on(void){locked=0;}
#include "../firmware/src/smk37_session.c"
#include "../firmware/src/smk37_editor.c"
static uint8_t flash[1<<20],backup[1<<20],sample_before[SMP_USER_SIZE];
static int remaining=-1,tear=-1;static uint32_t tear_address,writes;
static int operation(void){if(!remaining)return -1;if(remaining>0)--remaining;++writes;return 0;}
static int smk37_storage_read(uint32_t a,void *p,uint32_t n){assert(!locked&&a<sizeof flash&&n<=sizeof flash-a);memcpy(p,flash+a,n);return 0;}
static int smk37_storage_erase(uint32_t a){assert(!locked&&!(a&4095)&&SMK37_SAMPLE_RANGE(a,4096));if(operation())return -1;memset(flash+a,255,4096);return 0;}
static int smk37_storage_program(uint32_t a,const void *p,uint32_t n){assert(!locked&&n<=256-(a&255));assert(SMK37_SAMPLE_RANGE(a,n)||SMK37_SAMPLE_JOURNAL_RANGE(a,n));if(operation())return -1;
    unsigned count=tear>=0&&a==tear_address?(unsigned)tear:n;for(unsigned i=0;i<count;++i)flash[a+i]&=((const uint8_t *)p)[i];return count==n?0:-1;}
static const uint8_t *smk37_sample_xip(unsigned k){return smk37_samples_owned?flash+smk37_sample_address(k):smk37_empty_sample;}
static void smk37_sample_invalidate(unsigned k){(void)k;}
static int smk37_sample_format(void){return -1;}
static void protected(void){for(unsigned i=0;i<sizeof flash;++i)if(!SMK37_SAMPLE_RANGE(i,1)&&!SMK37_SAMPLE_JOURNAL_RANGE(i,1))assert(flash[i]==backup[i]);}
static unsigned pack(const uint8_t *p,unsigned n,uint8_t *out){unsigned size=0;for(unsigned i=0;i<n;){unsigned at=size++;out[at]=0;for(unsigned b=0;b<7&&i<n;++b,++i){out[at]|=(p[i]>>7)<<b;out[size++]=p[i]&127;}}return size;}
static unsigned command(unsigned cmd,const uint8_t *a,unsigned n){uint8_t f[650]={125,70,76};f[3]=cmd;memcpy(f+4,a,n);unsigned result=smk37_editor_main_command(f,n+4);assert(result);return smk37_ed_out[result-2];}
int main(int argc,char **argv){
    for(unsigned i=0;i<sizeof flash;++i)flash[i]=(i*31u+17u)>>3;
    memset(flash+SMK37_SAMPLE_JOURNAL,255,4096);memcpy(backup,flash,sizeof flash);
    uint32_t crcs[3];for(unsigned k=0;k<3;++k)crcs[k]=smk37_store_crc(flash+smk37_sample_address(k),SMP_USER_SIZE);
    assert(smk37_sample_owner_state(&smk37_storage_io,123)==0);
    assert(!SMK37_SAMPLE_RANGE(0xc1000,1)&&!SMK37_SAMPLE_RANGE(0xe9fff,2)&&!SMK37_SAMPLE_RANGE(0x9d000,UINT32_MAX));
    flash[0x9d123]^=1;assert(smk37_sample_initialize(&smk37_storage_io,123,crcs));assert(!writes);flash[0x9d123]^=1;
    /* Every failed erase/program, including every byte of both commit records,
     * resumes with the same unit and never writes protected regions. */
    for(int stop=0;stop<5;++stop){memcpy(flash,backup,sizeof flash);remaining=stop;assert(smk37_sample_initialize(&smk37_storage_io,123,crcs));remaining=-1;
        assert(!smk37_sample_initialize(&smk37_storage_io,123,crcs));assert(smk37_sample_owner_state(&smk37_storage_io,123)==2);protected();}
    for(unsigned h=0;h<2;++h)for(int cut=0;cut<32;++cut){memcpy(flash,backup,sizeof flash);tear_address=SMK37_SAMPLE_JOURNAL+h*256;tear=cut;
        assert(smk37_sample_initialize(&smk37_storage_io,123,crcs));tear=-1;assert(!smk37_sample_initialize(&smk37_storage_io,123,crcs));protected();}
    unsigned before=writes;assert(!smk37_sample_initialize(&smk37_storage_io,123,crcs));assert(writes==before);
    assert(smk37_sample_initialize(&smk37_storage_io,456,crcs));assert(writes==before);
    smk37_synth_init();smk37_storage_writable=1;uint8_t a[640]={0};
    assert(command(11,a,1));assert(writes==before);smk37_samples_owned=1;
    song.playing=1;assert(command(11,a,1));song.playing=0;slc_store_boot();
    uint8_t data[4097];for(unsigned i=0;i<sizeof data;++i)data[i]=i*7;
    for(unsigned k=0;k<3;++k){a[0]=k;assert(!command(11,a,1));assert(!usr_nz[k]);
        for(unsigned off=0;off<sizeof data;){unsigned n=sizeof data-off;if(n>256)n=256;unsigned addr=512+off;
            a[0]=k;a[1]=addr&127;a[2]=(addr>>7)&127;a[3]=addr>>14;unsigned count=pack(data+off,n,a+4);
            assert(!command(12,a,4+count));assert(!command(12,a,4+count));off+=n;}
        smp_user_hdr_t h={0};h.magic=SMP_USER_MAGIC;h.version=1;h.nz=1;memcpy(h.name,"TEST PCM",8);h.data_len=sizeof data;h.crc=smk37_store_crc(data,sizeof data);
        h.zone[0]=(smp_zone_t){.n=8194,.le=8193,.rate=65536,.root16=960,.hi=127};
        a[0]=k;unsigned count=pack((void *)&h,sizeof h,a+1);
        flash[smk37_sample_address(k)+600]^=1;assert(command(13,a,count+1)==3);flash[smk37_sample_address(k)+600]^=1;
        if(!k){
            memcpy(sample_before,flash+smk37_sample_address(k),SMP_USER_SIZE);
            for(int stop=0;stop<3;++stop){memcpy(flash+smk37_sample_address(k),sample_before,SMP_USER_SIZE);remaining=stop;assert(command(13,a,count+1)==4);remaining=-1;smp_user_scan(k);assert(!usr_nz[k]);}
            for(int cut=0;cut<4;++cut){memcpy(flash+smk37_sample_address(k),sample_before,SMP_USER_SIZE);tear_address=smk37_sample_address(k);tear=cut;assert(command(13,a,count+1)==4);tear=-1;smp_user_scan(k);assert(!usr_nz[k]);}
            memcpy(flash+smk37_sample_address(k),sample_before,SMP_USER_SIZE);
        }
        assert(!command(13,a,count+1));assert(usr_nz[k]==1);assert(!command(13,a,count+1));
        if(argc>1){command(15,0,0);putchar('[');for(unsigned i=0;i<smk37_ed_n;++i)printf("%s%u",i?",":"",smk37_ed_out[i]);puts("]");}
        assert(!memcmp(flash+smk37_sample_address(k)+512,data,sizeof data));
        smp_user_scan(k);assert(usr_nz[k]==1); /* restart's read path */
        slc_man_t *m=slc_man_begin(k);assert(m);m->n=2;m->pos[0]=0;m->pos[1]=4000;m->end=8000;slc_man_commit(k);
        assert(slc_store_write(k)==3);smp_user_scan(k);
        const slc_man_t *restored=slc_man_of(slc_get(k+1));assert(restored&&restored->n==2&&restored->pos[1]==4000&&restored->end==8000);
        assert(!slc_store_write(k)); /* unchanged: no flash wear */
        flash[smk37_sample_address(k)+SLC_REC_OFF+20]^=1;smp_user_scan(k);assert(!slc_man_of(slc_get(k+1))); /* torn record loses slices only */
        assert(usr_nz[k]==1);
        a[0]=k;assert(!command(14,a,1));assert(!usr_nz[k]);protected();
    }
    a[0]=0;assert(!command(11,a,1));a[1]=0;a[2]=6;a[3]=0;unsigned n=pack(data,256,a+4);before=writes;
    assert(command(12,a,n+4)==1&&writes==before); /* 768 before 512 */
    a[2]=4;a[4]=127;assert(command(12,a,6)==1&&writes==before); /* malformed pack7 */
    assert(!smk37_editor_main_command((uint8_t[]){125,70,76,11,3},5));
    smk37_open(SMK37_STORAGE_PAGE);smk37_samples_owned=0;smk37_workflow_button(3);assert(!smk37_samples_request);
    smk37_workflow_button(2);smk37_open(SMK37_STORAGE_PAGE);smk37_workflow_button(3);assert(!smk37_samples_request);
    smk37_workflow_button(3);assert(smk37_samples_request);
    if(argc==1)puts("SMK samples: ownership, interrupted initialization, unit/range protection, 3-slot protocol upload, duplicates, CRC, restart, erase and UI confirmation passed");
}
