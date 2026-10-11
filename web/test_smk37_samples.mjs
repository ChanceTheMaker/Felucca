// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync('web/editor.html','utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const {parse,CMD,req}=vm.runInNewContext(proto+';({parse,CMD,req})',{console,setTimeout,clearTimeout});
const frames=readFileSync('build/smk37-tests/samples-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
assert.equal(frames.length,3);
for(const [k,f] of frames.entries()){
  assert.equal(f[4],CMD.SMP_INFO);const info=parse[CMD.SMP_INFO](f.slice(5,-1));
  assert.equal(info.nslots,3);assert.equal(info.slotKiB,80);
  assert.equal(info.slots[k].zones,1);assert.equal(info.slots[k].name,'TEST PCM');assert.equal(info.slots[k].kib,5);
  assert.deepEqual(Array.from(req.smpBegin(k)[1]),[k]);
  assert.deepEqual(Array.from(req.smpWrite(k,512,new Uint8Array([0,128,255]))[1]),[k,0,4,0,6,0,0,127]);
}
console.log('SMK samples: actual 3-slot firmware INFO replies decoded by Studio, slot capacity/names and upload packing passed');
