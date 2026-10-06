// SPDX-License-Identifier: GPL-3.0-only
// Browser workspace archives are distinct from physical-device backup files.
const fail=()=>{throw Error('Invalid browser workspace');};
const integer=(v,min,max)=>Number.isInteger(v)&&v>=min&&v<=max?v:fail();
const array=(v,n,fn)=>Array.isArray(v)&&v.length===n?v.map(fn):fail();
const nums=(v,n,min=0,max=127)=>array(v,n,x=>integer(x,min,max));
const name=(v,n)=>typeof v==='string'&&v.length<=n?v:fail();
const engine=v=>v!==1?integer(v,0,13):fail();
function step(s) {return {n:integer(s.n,0,4),notes:nums(s.notes,4),time:integer(s.time,0,2),flags:integer(s.flags,0,3),vel:integer(s.vel,0,127),hit:integer(s.hit,0,255),acc:integer(s.acc,0,255),chance:integer(s.chance??100,0,100)};}
function track(t) {
 const events=t.motion?.events;if(!Array.isArray(events)||events.length>64)fail();
 return {engine:engine(t.engine),preset:integer(t.preset,0,127),p:nums(t.p,91,-8192,8191),fm6:nums(Array.from(t.fm6),128),step:array(t.step,64,step),
  motion:{on:!!t.motion.on,events:events.map(e=>({step:integer(e.step,0,63),param:integer(e.param,0,90),value:integer(e.value,-64,127)}))}};
}
function project(p) {
 if(!Array.isArray(p.songRows)||p.songRows.length>16)fail();
 const result={tracks:array(p.tracks,4,track),sel:integer(p.sel,0,3),g:nums(p.g,27,-8192,8191),songRows:p.songRows.map(r=>({slot:integer(r.slot,0,3),repeat:integer(r.repeat,1,16)}))};
 if(result.tracks.reduce((n,t)=>n+t.motion.events.length,0)>64)fail();
 // Only the internal audio clock and direct four-part routing are meaningful here.
 result.g[2]=0;result.g[14]=0;return result;
}
export function captureSession(st) {
 return readSession({format:'felucca-browser-workspace',version:1,state:{...project(st),slots:st.slots,bank:st.bank,fm6bank:st.fm6bank,favorites:st.favorites,filter:st.filter}});
}
export function readSession(data) {
 if(data?.format!=='felucca-browser-workspace'||data.version!==1||!data.state)fail();
 const s=data.state;
 const state={...project(s),slots:array(s.slots,4,p=>p===null?null:project(p)),
  bank:array(s.bank,32,p=>p===null?null:{engine:engine(p.engine),name:name(p.name,12),p:nums(p.p,91,-8192,8191),
    pattern:p.pattern==null?null:array(p.pattern,16,x=>nums(x,2)),grid:p.grid==null?null:array(p.grid,16,x=>nums(x,2,0,255))}),
  fm6bank:array(s.fm6bank,27,p=>p===null?null:nums(Array.from(p),128)),
  favorites:array(s.favorites,15,a=>{if(!Array.isArray(a)||a.length>128)fail();return Array.from(a,Boolean);}),filter:integer(s.filter,0,1)};
 return {format:'felucca-browser-workspace',version:1,state};
}
