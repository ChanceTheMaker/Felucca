/* SPDX-License-Identifier: GPL-3.0-only */
static unsigned smk37_editor_samples(unsigned cmd,const uint8_t *a,unsigned n){
    if(cmd==15){
        if(n)return 0;smk37_ed_begin(cmd);unsigned slots=smk37_samples_owned?3:0;
        smk37_ed_byte(slots);smk37_ed_byte(slots?80:0);
        for(unsigned k=0;k<slots;++k){const smp_user_hdr_t *h=(const void *)smp_user_xip(k);char name[9]={0};
            for(unsigned i=0;usr_nz[k]&&i<8;++i)name[i]=h->name[i]>=32&&h->name[i]<127?h->name[i]:0;
            smk37_ed_byte(usr_nz[k]);smk37_ed_string(name,8);smk37_ed_byte(usr_nz[k]?(h->data_len+1023)/1024:0);
        }return smk37_ed_finish();
    }
    if(!n||a[0]>=3||((cmd==11||cmd==14)&&n!=1)||(cmd==12&&n<5)||(cmd==13&&n<2))return 0;
    unsigned k=a[0],rc=1,off=0;
    if(cmd==12)off=a[1]|a[2]<<7|a[3]<<14;
    if(smk37_samples_owned && smk37_storage_writable && smk37_up_idle()){
        if(cmd==11||cmd==14){
            smk37_sample_unpublish(k);rc=smk37_sample_begin(&smk37_storage_io,smk37_upload,k,1);
            if(cmd==14){smk37_upload[k].active=0;for(unsigned p=4096;!rc&&p<SMP_USER_SIZE;p+=4096)rc=smk37_storage_erase(smk37_sample_address(k)+p)?2:0;}
            smk37_sample_invalidate(k);
        }else if(cmd==12){unsigned len=smk37_sample_unpack(a+4,n-4,smk37_sample_buffer,256);
            rc=smk37_sample_write(&smk37_storage_io,smk37_upload,k,off,smk37_sample_buffer,len,1);
        }else if(cmd==13)rc=smk37_sample_end(k,a+1,n-1);
    }
    smk37_ed_begin(cmd);smk37_ed_byte(k);
    if(cmd==12){smk37_ed_byte(off);smk37_ed_byte(off>>7);smk37_ed_byte(off>>14);}
    smk37_ed_byte(rc);return smk37_ed_finish();
}
