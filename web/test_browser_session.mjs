// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFile} from 'node:fs/promises';
import vm from 'node:vm';
import {captureSession,readSession} from './audio/session.js';
const html=await readFile(new URL('./editor.html',import.meta.url),'utf8');
const make=vm.runInNewContext(html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'))+';makeMockDevice',{setTimeout,clearTimeout,setInterval,clearInterval,console});
const mock=make({auto:false,browser:true});
const saved=captureSession(mock.state);
assert.equal(JSON.stringify(readSession(JSON.parse(JSON.stringify(saved)))),JSON.stringify(saved));
for(const mutate of [s=>s.tracks.pop(),s=>s.tracks[0].engine=99,s=>s.tracks[0].p.pop(),s=>s.tracks[0].step[0].notes[0]=999,s=>s.songRows=[{slot:4,repeat:1}],s=>s.fm6bank[0]=[0]]) {
 const bad=structuredClone(saved);mutate(bad.state);assert.throws(()=>readSession(bad));
}
assert.throws(()=>readSession({format:'fm1-backup',version:1,state:saved.state}));
console.log('Browser workspace round trip and malformed/incompatible archive rejection passed.');
