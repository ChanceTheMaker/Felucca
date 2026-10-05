// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import './studio-widgets.js';
const {scaleNotes,slicerSteps,lfoValue,arpNotes,envelopePoints,filterPoints}=globalThis.FeluccaWidgets;
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
console.log('Studio widget models: all 16 firmware scales and slicer patterns, LFO phase/shapes, arp traversal/repeat, envelope and filter bounds pass.');
