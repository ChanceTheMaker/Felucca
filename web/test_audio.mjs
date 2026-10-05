// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
const module = await WebAssembly.compile(await readFile(new URL('./audio/engine.wasm', import.meta.url)));
assert.deepEqual(WebAssembly.Module.imports(module), [], 'No device or host imports');
function synth(e = 0, preset = 0) {
  const x = new WebAssembly.Instance(module).exports;
  x.synth_init(); x.synth_engine(e);
  for (let i=0;i<57;i++) x.synth_param(i, x.preset_value(e,preset,i));
  // Let engine changes finish before playing.
  for(let i=0;i<100;i++) x.synth_render();
  return x;
}
function energy(x, blocks=1000) {
  let sum=0,peak=0;
  for(let b=0;b<blocks;b++) {
    const data=new Int32Array(x.memory.buffer,x.synth_render(),64);
    for(const v of data) { assert(Number.isFinite(v)); sum+=Math.abs(v); peak=Math.max(peak,Math.abs(v)); }
  }
  return {sum,peak};
}
for(let e=0;e<9;e++) {
  const x=synth(e); x.synth_midi(0x90,60,100);
  const out=energy(x); assert(out.sum>10000,`Engine ${e} produces sound`);
  for(let preset=1;preset<x.preset_count(e);preset++) {
    const voice=synth(e,preset); voice.synth_midi(0x90,e===4 && preset===4 ? 36 : 60,100); // PERC uses GM drum mappings.
    assert(energy(voice,2000).sum>1000,`Engine ${e}, preset ${preset} produces sound`);
  }
  console.log(`Engine ${e}: peak ${out.peak}, ${x.preset_count(e)} presets`);
}
const x=synth();
const before=synth(), after=synth();
after.synth_param(53,10);
before.synth_midi(0x90,60,100); after.synth_midi(0x90,60,100);
assert.notEqual(energy(before).sum,energy(after).sum,'Filter control changes the sound');
// Sustained organ-like envelope without FX or arp for release testing.
for(const [i,v] of [[1,0],[2,0],[3,127],[4,0],[17,0],[33,0],[34,0],[35,0],[36,0],[37,0]]) x.synth_param(i,v);
x.synth_midi(0x90,60,100); energy(x,100);
x.synth_midi(0xb0,64,127); x.synth_midi(0x80,60,0);
assert(energy(x).sum>10000,'Sustain keeps released notes sounding');
x.synth_midi(0xb0,64,0); energy(x,2000);
assert.equal(energy(x,100).sum,0,'Pedal release ends the note');
x.synth_midi(0x90,60,100); assert(energy(x).sum>10000);
x.synth_midi(0xb0,120,0); energy(x,2000);
assert.equal(energy(x,100).sum,0,'Panic silences notes');
console.log('WASM audio checks passed');
const source=await readFile(new URL('./audio/worklet.js',import.meta.url),'utf8');
for(const rate of [44100,48000]) {
  let Processor;
  vm.runInNewContext(source,{WebAssembly,Int32Array,Math,sampleRate:rate,
    AudioWorkletProcessor:class { constructor(){this.port={};} },
    registerProcessor(name,p){Processor=p;}});
  const p=new Processor({processorOptions:{module}});
  const meta=synth();
  p.port.onmessage({data:{type:'state',engine:0,p:Array.from({length:57},(_,i)=>meta.preset_value(0,0,i)),g:[]}});
  p.port.onmessage({data:{type:'midi',bytes:[0x90,60,100]}});
  let sum=0;
  for(let n=0;n<200;n++) {
    const out=[new Float32Array(n%2?128:256),new Float32Array(n%2?128:256)];
    p.process([], [out]);
    for(const ch of out) for(const v of ch) {assert(Number.isFinite(v)&&Math.abs(v)<=1);sum+=Math.abs(v);}
  }
  assert(sum>1); console.log(`AudioWorklet ${rate} Hz / variable block sizes passed`);
}
