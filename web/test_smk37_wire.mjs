// SPDX-License-Identifier: GPL-3.0-only
// Consume real host-compiled SMK firmware replies with the shipping JS parser.
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import vm from 'node:vm';
const html = readFileSync(new URL('editor.html', import.meta.url), 'utf8');
const proto = html.slice(html.indexOf('/*PROTO-BEGIN*/'), html.indexOf('/*PROTO-END*/'));
const { parse, CMD } = vm.runInNewContext(proto + ';({parse,CMD})', {console,setTimeout,clearTimeout});
const frames = readFileSync(process.argv[2], 'utf8').trim().split('\n').map(JSON.parse);
let info, names = 0, desc = 0;
for (const frame of frames) {
  assert.deepEqual(frame.slice(0,4), [240,125,70,76]);
  assert.equal(frame.at(-1),247);
  assert(frame.slice(1,-1).every(b => b >= 0 && b < 128));
  const cmd = frame[4], result = parse[cmd](frame.slice(5,-1),info);
  if (cmd === CMD.INFO) {
    info=result; assert.match(info.version,/SMK37 TEST10/);
    assert.equal(info.ntrk,4); assert.equal(info.nstep,64); assert.equal(info.uiCaps,0);
  }
  if (cmd === CMD.DUMP || cmd === CMD.TRACK_DUMP) assert.equal(result.p.length,info.pcount);
  if (cmd === CMD.NAMES) { names++; assert.equal(result.names.length,frame[6]); assert.equal(result.titles.length,2); }
  if (cmd === CMD.DESC) { desc++; assert(result.min<=result.max); }
  if (cmd === CMD.SMP_INFO) assert.equal(result.nslots,0);
  if (cmd === CMD.UP_LIST) assert.equal(result.total,0);
}
assert.equal(names,info.nengines);
assert.equal(desc,info.nengines*info.pcount+info.gcount);
console.log(`SMK wire integration: ${frames.length} actual C replies parsed, all engines/descriptors, tracks and steps passed`);
