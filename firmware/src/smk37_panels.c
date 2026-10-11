/* SPDX-License-Identifier: GPL-3.0-only
 * Copyright (C) 2026 Chance Roth (@ChanceTheMaker).
 * SMK-specific views use the FM-1 palette, list geometry and sequencer concepts.
 * They describe the actual SMK state and available actions, not FM-1-only menus. */
static void smk37_graph_values(const char *title,char labels[4][24],char values[4][24]){
    cv_text_fit(10,2,&AF_M,title,T_TEXT,T_SURF,220);
    for(unsigned i=0;i<4;++i){
        int y=28+i*22;
        cv_text_fit(10,y,&AF_S,labels[i],T_MID,T_SURF,108);
        cv_text_fit(122,y,&AF_S,values[i],T_THEME,T_SURF,108);
        if(i<3)cv_rect(10,y+19,220,1,T_RAISE);
    }
}
static void smk37_graph_row(int y,int selected,const char *tag,const char *name,const char *tail){
    uint16_t bg=selected?T_THEME:T_SURF,fg=selected?T_INK:T_TEXT;
    if(selected)cv_rrect(6,y,228,23,4,bg,T_SURF);
    cv_text_on(12,y+3,&AF_S,tag,fg,bg);
    int end=tail&&tail[0]?186:228;
    cv_text_fit(45,y+3,&AF_S,name,fg,bg,end-45);
    if(tail)cv_text_r(226,y+3,&AF_S,tail,selected?T_INK:T_MID,bg);
}
static void smk37_graph_projects(void){
    /* Reads happen in the main loop, with IRQs enabled; never issue a write here. */
    for(unsigned i=0;i<4;++i){
        char tag[2]={(char)('A'+i),0};
        int used=smk37_session_used(i);
        smk37_graph_row(5+i*28,i==smk37_project_slot,tag,used?"SAVED PROJECT":"EMPTY SLOT",0);
    }
}
static void smk37_graph_presets(void){
    unsigned first=smk37_user_slot<2?0:smk37_user_slot>5?4:smk37_user_slot-1;
    for(unsigned r=0;r<4;++r){
        unsigned i=first+r;char tag[5]={'U','0',(char)('1'+i),0},name[13]={0};
        int used=smk37_up_used(i);
        if(used)memcpy(name,smk37_up.record[i].name,12);else str_cpy(name,"EMPTY SLOT",sizeof name);
        smk37_graph_row(5+r*28,i==smk37_user_slot,tag,name,0);
    }
}
static void smk37_graph_song(void){
    if(!chain_config.count){panel_note("NO SECTIONS","> ADD SECTION","SAVE PROJECT TO KEEP");return;}
    unsigned first=(smk37_song_row/4)*4;
    for(unsigned row=0;row<4;++row){
        char label[3]={(char)('1'+row),0};cv_text_on(8,29+row*22,&AF_S,label,T_MID,T_SURF);
    }
    for(unsigned col=0;col<4 && first+col<chain_config.count;++col){
        unsigned n=first+col;const chain_row_t *r=&chain_config.row[n];char title[12],v[3];
        fmt_int(title,n+1);str_cpy(title+str_len(title)," x",4);fmt_int(title+str_len(title),r->repeat);
        int x=30+col*50;
        cv_text_in(x,3,46,&AF_S,title,n==smk37_song_row?T_THEME:T_MID,T_SURF);
        for(unsigned tr=0;tr<4;++tr){
            int selected=n==smk37_song_row && tr==smk37_song_track;
            uint16_t bg=selected?T_THEME:T_RAISE;
            cv_rrect(x,28+tr*22,46,19,3,bg,T_SURF);
            v[0]=r->slot[tr]<4?'A'+r->slot[tr]:'-';v[1]=0;
            cv_text_in(x,28+tr*22,46,&AF_S,v,selected?T_INK:r->slot[tr]<4?T_TEXT:T_DIM,bg);
        }
    }
}
static void smk37_graph_drums(const track_t *t,unsigned bank){
    for(unsigned i=0;i<8;++i){
        int x=12+i*28,h=clamp(t->p[P_LN0+i],0,127)*73/127;
        uint16_t c=i/4==bank?T_THEME:T_MID;
        cv_rrect(x+6,9,5,77,2,T_RAISE,T_SURF);
        if(h)cv_rect(x+4,86-h,9,h,c);
        cv_rrect(x+1,83-h,15,6,2,c,T_SURF);
        cv_text_in(x-3,93,25,&AF_S,drum_lane_abbr(t,i),c,T_SURF);
    }
}
static void smk37_graph_detail(const track_t *t){
    static const char *const names[]={"CHN","RAT","NDG","VEL"};
    unsigned base=smk37_draw_step&~15u;
    for(unsigned row=0;row<4;++row){
        int y=6+row*28;cv_text_on(7,y+6,&AF_S,names[row],T_MID,T_SURF);
        for(unsigned i=0;i<16 && base+i<(unsigned)t->p[P_SLEN];++i){
            const step_t *s=&seq_steps(t)[base+i];int x=37+i*12,h;
            uint16_t c=base+i==smk37_draw_step?T_ACCENT:step_on(s)?T_THEME:T_DIM;
            cv_rect(x,y+24,10,1,T_RAISE);
            if(row==0){h=step_chance(s)*22/100;cv_rect(x+1,y+24-h,8,h,c);}
            else if(row==1)pr_split(x+1,y+10,9,10,c,step_ratchet(s));
            else if(row==2){int n=step_nudge(s);cv_rect(x+5,y+3,1,20,T_RAISE);cv_line(x+5,y+12,x+5+n/2,y+12,c);}
            else{h=s->vel*22/127;cv_rect(x+1,y+24-h,8,h,c);}
            if(base+i==smk37_draw_step)cv_frame(x,y+1,12,26,T_TEXT);
        }
    }
}
static void smk37_graph_automation(const track_t *t){
    unsigned shown=0;char tag[8],value[24];
    for(unsigned i=0;i<motion.count && shown<4;++i){
        const motion_event_t e=motion.event[i];
        if((e.place>>6)!=trk_index(t) || (e.place&63)<(smk37_draw_step&~15u))continue;
        unsigned id=MOTION_ID(&e);const param_desc_t *d=track_desc(t,id);
        const char *unit;fmt_int(tag,(e.place&63)+1);param_format(d,e.value,value,&unit);
        smk37_graph_row(5+shown*28,(e.place&63)==smk37_draw_step,tag,d->label,value);++shown;
    }
    if(!shown)panel_note("NO EVENTS IN THIS RANGE","> SET EVENT / LOCK","< DELETE SELECTED EVENT");
}
static void smk37_graph_display(void){
    cv_text_in(10,3,220,&AF_M,UI_PALETTES[smk37_palette%NPALETTES].name,T_TEXT,T_SURF);
    const uint16_t colors[]={T_THEME,T_ACCENT,T_TEXT,T_MID,T_RAISE};
    for(unsigned i=0;i<5;++i)cv_rrect(14+i*44,34,36,30,5,colors[i],T_SURF);
    cv_text_in(10,77,220,&AF_M,"Felucca SMK",T_THEME,T_SURF);
    cv_text_in(10,101,220,&AF_S,smk37_bold?"BOLD FONT":"REGULAR FONT",T_MID,T_SURF);
}
static void smk37_graph_performance(void){
    static const char *const name[16]={"1/8","1/16","1/32","REV","LPF","HPF","STOP","FRZ","OCT+","OCT-","FLNG","PHSR","RVRB","ECHO","MUTE1","MUTE2"};
    for(unsigned i=0;i<16;++i){
        int x=9+(i%4)*57,y=5+(i/4)*29;
        int active=(smk37_perf_pads&(1u<<i))!=0;
        uint16_t bg=active?T_THEME:T_RAISE;
        cv_rrect(x,y,51,24,4,bg,T_SURF);
        cv_text_in(x,y+5,51,&AF_S,name[i],active?T_INK:T_TEXT,bg);
    }
}
