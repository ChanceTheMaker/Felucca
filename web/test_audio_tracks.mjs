// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
const module=await WebAssembly.compile(await readFile(new URL('./audio/engine.wasm',import.meta.url)));
const x=new WebAssembly.Instance(module).exports;x.synth_init();
const energy=(blocks=400)=>{let sum=0;for(let i=0;i<blocks;i++)for(const v of new Int32Array(x.memory.buffer,x.synth_render(),64))sum+=Math.abs(v);return sum;};
for(let k=0;k<4;k++) {
 x.synth_target(k);x.synth_engine(0);
 for(let p=0;p<x.param_count();p++)x.synth_param(p,x.preset_value(0,0,p));
 for(const [p,v] of [[4,0],[33,0],[34,0],[35,0],[36,0]])x.synth_param(p,v);
}
energy();
for(let k=0;k<4;k++) {
 x.synth_midi(0x90|k,60+3*k,110);assert(energy()>10000,`track ${k+1} audible`);
 x.synth_midi(0xb0|k,120,0);energy();
 x.synth_target(k);x.synth_param(40,1);x.synth_midi(0x90|k,60+3*k,110);energy();assert.equal(energy(),0,`track ${k+1} mute blocks notes`);
 x.synth_midi(0xb0|k,120,0);x.synth_param(40,0);energy();
}
for(let k=0;k<4;k++) {
 x.synth_target(k);x.synth_param(29,4);x.synth_param(30,2);
 x.synth_step(k,0,1,0,0,100,0,0,100,60+k*3,0,0,0);
}
x.synth_transport(3);assert(energy(100)>10000,'sequencer produces actual audio');
assert.equal(x.synth_playing(),1);
energy(800);assert(x.synth_position(0)>0,'audio clock advances pattern');
x.synth_transport(2);energy(1000);assert.equal(x.synth_playing(),0);assert.equal(energy(),0,'stop releases sequence notes');
console.log('Four-track audio, per-track mute, sequence playback/clock/stop passed.');
