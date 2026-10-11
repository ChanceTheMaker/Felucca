// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync(new URL('editor.html',import.meta.url),'utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const {parse,req,CMD}=vm.runInNewContext(proto+';({parse,req,CMD})',{console,setTimeout,clearTimeout});
const frames=readFileSync('build/smk37-tests/completion-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
const info=parse[CMD.INFO](frames[0].slice(5,-1));
assert.equal(info.songLanes,4);assert.equal(info.fm6.caps,3);
for(const f of frames.slice(1)){
 const o=parse[f[4]](f.slice(5,-1),info);
 if(f[4]===CMD.UP_LIST){assert.equal(o.total,8);assert.equal(o.slots.filter(s=>s.used).length,3);}
 if(f[4]===CMD.UP_GET){assert.equal(o.name,'TEST');assert.equal(o.p.length,111);}
 if(f[4]===CMD.SONG){assert.equal(o.rows[0].slots.join(','),'0,1,4,0');assert.equal(o.rows[0].repeat,3);}
 if(f[4]===CMD.FM6_GET){assert.equal(o.rc,0);assert.equal(o.packed.length,128);}
}
assert.equal(req.song(5,[{slots:[0,1,4,0],repeat:3}])[1].join(','),'5,1,0,1,4,0,3');
assert.throws(()=>req.song(5,[{slots:[0,1,5,0],repeat:3}]));
assert.throws(()=>parse[CMD.SONG]([4,0,1,0,0,0,0]));
console.log('SMK C-to-JS: eight presets, names/parameters, FM6 OWN patch and per-track song lanes passed');
