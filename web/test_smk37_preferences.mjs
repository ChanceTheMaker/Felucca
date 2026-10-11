// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const html=readFileSync('web/editor.html','utf8');
const proto=html.slice(html.indexOf('/*PROTO-BEGIN*/'),html.indexOf('/*PROTO-END*/'));
const {parse,CMD}=vm.runInNewContext(proto+';({parse,CMD})',{console,setTimeout,clearTimeout});
const frames=readFileSync('build/smk37-tests/workflow-wire.jsonl','utf8').trim().split('\n').map(JSON.parse);
for(const f of frames){const r=parse[f[4]](f.slice(5,-1));
 assert.equal(f.at(-1),247);assert(f.slice(1,-1).every(v=>v<128));
 if(f[4]===CMD.UI_SET){assert.equal(r.rc,0);assert.equal(r.monitor,2);}
 if(f[4]===CMD.UI_STATE){assert.equal(r.monitor,2);assert.equal(r.caps,15);}
 if(f[4]===CMD.UI_PALETTES)assert.equal(r.length,23);
 if(f[4]===CMD.FAV_SET)assert.equal(r.rc,0);
 if(f[4]===CMD.FAV_GET){assert.equal(r.rc,0);assert.deepEqual(Array.from(r.values),[false,false,true,false]);}
}
console.log('SMK preferences/favorites: actual firmware frames parsed by shipping Studio protocol');
