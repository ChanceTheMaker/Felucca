/* SPDX-License-Identifier: GPL-3.0-only
 * Manual slice semantics from ui_slice.c, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hugelton Instruments; original slice editing by andreahaku.
 * Main-loop SMK adapter. User sample storage remains a separate allocation. */
static void smk37_slice_service(void){
    unsigned action=smk37_slice_request;if(!action)return;
    unsigned tr=smk37_slice_track,src=smk37_slice_src,div=smk37_slice_div,j=smk37_slice_marker;
    int direction=smk37_slice_direction;track_t *t=&trk[tr];
    const slc_src_t *s=slc_get(src);unsigned k=src-1u;
    if(smk37_page!=SMK37_SLICE_PAGE || tr!=song.sel || ENGINES[t->eng_req]!=&ENG_SLICE || slc_src_of(t->p)!=src || t->p[P_E1]!=div || !s || k>=SMP_USER_SLOTS){smk37_slice_request=0;return;}
    if(div!=SLC_DIV_MAN && slc_man_of(s)){t->p[P_E1]=SLC_DIV_MAN;smk37_slice_request=0;ui_message("DIV MAN");return;}
    slc_man_t *m=slc_man_begin(k);unsigned n=slc_count(s,div),a,b,st;
    if(!m || !n){smk37_slice_request=0;return;}
    if(div!=SLC_DIV_MAN){m->n=0;m->end=0;
        for(unsigned i=0;i<n;++i){slc_bounds(s,div,i,&a,&b,&st);
            if(a<s->len && s->len-a>=SLC_MIN && (!m->n || a-m->pos[m->n-1]>=SLC_MIN)){m->pos[m->n]=a;m->st[m->n++]=st;}}
    }
    if(!m->n){smk37_slice_request=0;ui_message("SAMPLE TOO SHORT");return;}
    j=j<n?(j<m->n?j:m->n-1):m->n;
    if(action==1){
        a=j?m->pos[j-1]:0;b=j+1<m->n?m->pos[j+1]:s->len;
        int d=direction*(int)((b-a)/216u?(b-a)/216u:1u);
        if(j<m->n)slc_man_move(s,m,j,d);else slc_man_move_end(s,m,d);
    }else if(j>=m->n){smk37_slice_request=0;ui_message("PICK A SLICE");return;}
    else if(action==2){unsigned old=m->n;j=slc_man_split(s,m,j);
        if(m->n==old){smk37_slice_request=0;ui_message(old>=SLC_AUTO?"32 SLICES AT MOST":"SLICE TOO SHORT");return;}}
    else if(action==3){if(!j){smk37_slice_request=0;ui_message("FIRST SLICE");return;}j=slc_man_join(m,j);}
    /* Source/track may have changed during decoding: discard rather than
     * committing a table against another sample/selection. */
    fm1_irq_off();
    if(song.sel==tr && smk37_page==SMK37_SLICE_PAGE && ENGINES[t->eng_req]==&ENG_SLICE && slc_src_of(t->p)==src && t->p[P_E1]==div){
        slc_man_commit(k);t->p[P_E1]=SLC_DIV_MAN;smk37_slice_sel=j;slc_man_save|=1u<<k;
        ui_message(smk37_samples_owned?"SLICES / SAVE ON EXIT":"SLICES IN RAM / NOT SAVED");
    }
    smk37_slice_request=0;fm1_irq_on();
}
