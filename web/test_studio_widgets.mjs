// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import './studio-widgets.js';
const {scaleNotes,slicerSteps,lfoValue,arpNotes,arpSegments,envelopePoints,filterPoints,filterHandle,filterCutAt}=globalThis.FeluccaWidgets;
const source=name=>readFileSync(new URL(`../firmware/src/${name}`,import.meta.url),'utf8');
const table=(file,name)=>source(file).match(new RegExp(`\\b${name}\\[[^]*?=\\s*\\{([^]*?)\\};`))[1].replace(/\/\*[^]*?\*\//g,'');
const masks=table('seq.c','SCALE_MASK').split(',').map(s=>s.trim()).filter(Boolean).map(s=>Function(`return (${s});`)());
for(let scale=0;scale<masks.length;scale++)for(let root=0;root<12;root++) {
 const expected=Array.from({length:12},(_,i)=>i).filter(i=>masks[scale]&(1<<i)).map(i=>(i+root)%12);
 assert.deepEqual(scaleNotes(root,scale),expected);
}
const patterns=table('slicer.c','SL_PAT').match(/0x[0-9A-F]+/gi).map(v=>Number(v));
for(let p=1;p<=patterns.length;p++)assert.deepEqual(slicerSteps(p),Array.from({length:16},(_,i)=>!!(patterns[p-1]&(1<<i))));
assert.deepEqual(slicerSteps(1).slice(0,4),[true,false,true,false]);
assert.deepEqual(arpNotes(0,4),[]);
assert.deepEqual(arpNotes(6,4,1),Array(16).fill(4));
assert.deepEqual(arpNotes(1,2).slice(0,6),[0,4,7,12,16,19]);
assert.deepEqual(arpNotes(2,1).slice(0,3),[7,4,0]);
assert.deepEqual(arpNotes(3,1).slice(0,5),[0,4,7,4,0]);
assert.deepEqual(arpNotes(5,1,1).slice(0,3),[7,0,4]);
assert.equal(lfoValue(0,0),0);assert.equal(lfoValue(1,0),-1);assert.equal(lfoValue(1,.5),1);
assert.equal(lfoValue(2,0),-1);assert.equal(lfoValue(3,0),1);assert.equal(lfoValue(3,.5),-1);
assert.equal(lfoValue(4,.1),lfoValue(4,.9));assert.notEqual(lfoValue(4,.9),lfoValue(4,1.1));
for(let wave=0;wave<5;wave++)for(let p=0;p<128;p++)assert(Math.abs(lfoValue(wave,p/31))<=1);
for(const v of [0,1,64,127]) {
 const points=envelopePoints([v,v,v,v]);assert(points.every(([x,y])=>Number.isFinite(x)&&Number.isFinite(y)&&x>=0&&x<=260&&y>=0&&y<=96));
 assert(points.every(([x],i)=>!i||x>points[i-1][0]));
 for(const res of [0,127])assert(filterPoints(v,res).every(([x,y])=>Number.isFinite(x)&&Number.isFinite(y)&&y>=8&&y<=86));
}
assert(envelopePoints([0,0,127,0])[3][1]<envelopePoints([0,0,0,0])[3][1]);
// The cutoff handle must use the same logarithmic Hz axis as the response plot,
// and round-trip pointer coordinates across the full MIDI parameter range.
for(let cut=0;cut<=127;cut++) {
 const [x,y]=filterHandle(cut,64);
 assert(Math.abs(filterCutAt(x)-cut)<1e-8);
 assert(x>=8 && x<=252 && y>=8 && y<=86);
 const hz=20*Math.pow(1000,(x-8)/244);
 assert(Math.abs(hz-30*Math.pow(16000/30,cut/127))<1e-8);
}
assert(filterHandle(64,127)[1]<filterHandle(64,0)[1]);
// Arpeggiator notes remain separate even at maximum gate; no diagonal pitch
// transitions imply a glide that the synth wasn't asked to play.
assert.deepEqual(arpSegments(0,1,0,127),[]);
for(const gate of [0,64,127]) {
 const bars=arpSegments(1,2,0,gate);assert.equal(bars.length,16);
 bars.forEach(([[x,y],[end,y2]],i)=>{
  assert.equal(y,y2);assert(end>x);
  if(i<15)assert(end<bars[i+1][0][0]);
 });
}
console.log('Studio widget models: all 16 firmware scales and slicer patterns, LFO phase/shapes, arp traversal/repeat, envelope and filter bounds pass.');
