import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const codes=['en','ja','es','fr','de','ru','zh-CN','pt-BR'];
const catalogs=Object.fromEntries(codes.map(c=>[c,JSON.parse(readFileSync(new URL(`locales/${c}.json`,import.meta.url),'utf8'))]));
const placeholders=s=>[...s.matchAll(/\{(\w+)\}/g)].map(m=>m[1]).sort();
for(const code of codes) {
 assert.deepEqual(Object.keys(catalogs[code]).sort(),Object.keys(catalogs.en).sort(),`${code}: full catalog coverage`);
 for(const [key,source] of Object.entries(catalogs.en)) {
  const target=catalogs[code][key];
  assert.equal(typeof target,'string',`${code}.${key}`);
  assert.ok(target.trim(),`${code}.${key} is not blank`);
  assert.deepEqual(placeholders(target),placeholders(source),`${code}.${key} placeholders`);
 }
}
for(const file of ['editor.html','index_pkg.html','analytics.js','skin.js']) {
 const source=readFileSync(new URL(file,import.meta.url),'utf8');
 for(const match of source.matchAll(/data-i18n(?:-(?:aria-label|alt|title|placeholder))?="([\w.]+)"/g)) assert.ok(match[1] in catalogs.en,`${file}: ${match[1]}`);
}
console.log(`All ${codes.length} languages cover ${Object.keys(catalogs.en).length} keys with matching placeholders.`);
const runtime=readFileSync(new URL('i18n.js',import.meta.url),'utf8');
function setup(saved, browserLanguages, blocked=false) {
 const handlers={}, storage=new Map(saved ? [['felucca.web.language',saved]] : []);
 const document={documentElement:{},body:{classList:{contains:()=>true}},getElementById:()=>null,querySelectorAll:()=>[],addEventListener:()=>{}};
 const context={document,navigator:{languages:browserLanguages},FeluccaLocales:catalogs,
  localStorage:{getItem:k=>{if(blocked) throw Error('blocked');return storage.get(k)},setItem:(k,v)=>{if(blocked) throw Error('blocked');storage.set(k,v)}},
  addEventListener:(name,fn)=>{handlers[name]=fn}};
 context.window=context;vm.runInNewContext(runtime,context);
 return {i:context.FeluccaI18n,document,storage,handlers};
}
const saved=setup('fr',['de-DE']);assert.equal(saved.i.language,'fr');
assert.equal(setup(null,['zh-SG']).i.language,'zh-CN');
assert.equal(setup(null,['pt-PT']).i.language,'pt-BR');
assert.equal(setup(null,['xx','es-MX']).i.language,'es');
assert.equal(setup(null,['xx']).i.language,'en');
saved.i.setLanguage('ru');assert.equal(saved.document.documentElement.lang,'ru');
assert.equal(saved.storage.get('felucca.web.language'),'ru');
assert.equal(saved.i.t('ui.version',{version:'1.2'}),catalogs.ru['ui.version'].replace('{version}','1.2'));
assert.equal(saved.i.namespace('editor','de').connect,catalogs.de['editor.connect']);
saved.handlers.storage({key:'felucca.web.language',newValue:'ja'});assert.equal(saved.i.language,'ja');
assert.doesNotThrow(()=>setup(null,['de'],true).i.setLanguage('es'));
console.log('Language detection, persistence, tab sync, interpolation, namespaces and blocked storage passed.');
