// SPDX-License-Identifier: GPL-3.0-only
// Run against a built preview. PLAYWRIGHT_MODULE may point at an existing install.
// STUDIO_URL defaults to the shared RC1 preview; no physical MIDI device is used.
import assert from 'node:assert/strict';
import {mkdir} from 'node:fs/promises';
import {builtSiteFixture} from './test_site_fixture.mjs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const url=process.env.STUDIO_URL || 'http://127.0.0.1:8768/rc1/webapp/editor/';
const browser=await chromium.launch({channel:process.env.BROWSER_CHANNEL || 'chrome',headless:true});
const errors=[];
try {
 const page=await browser.newPage({viewport:{width:1440,height:1100}});
 page.setDefaultTimeout(30000);
 page.on('pageerror',e=>{errors.push(e.message);console.error('PAGE ERROR:',e.message);});
 page.on('requestfailed',request=>console.error('REQUEST FAILED:',request.url(),request.failure()?.errorText));
 const fixtureHtml=await builtSiteFixture(page,process.env.STUDIO_FIXTURE_DIR);
 // Expose read-only handles inside this test response, never in shipped assets.
 let testHtml;
 await page.route('**/webapp/editor/?*',async route=>{
  if(testHtml) {await route.fulfill({status:200,contentType:'text/html; charset=utf-8',body:testHtml});return;}
  const response=fixtureHtml?null:await route.fetch();let html=fixtureHtml?await fixtureHtml():await response.text();
  const marker='if (!MOCK && !navigator.requestMIDIAccess) sayK("nomidi");';
  assert(html.includes(marker));
  html=html.replace(marker,marker+`
   Object.defineProperties(window,{
    dev:{configurable:true,get:()=>dev},busy:{configurable:true,get:()=>busy},
    connecting:{configurable:true,get:()=>connecting},mock:{configurable:true,get:()=>mock},
    ctl:{configurable:true,get:()=>ctl},browserSynth:{configurable:true,get:()=>browserSynth}
   });window.buildGroups=buildGroups;window.engName=engName;
  `);
  testHtml=html;
  await route.fulfill({...(response?{response}:{status:200,contentType:'text/html; charset=utf-8'}),body:html});
 });
 await page.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.controls','sliders');
  localStorage.setItem('felucca.web.keyboardCollapsed','true');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
 });
 await page.goto(url+'?mock=1&theme=midnight');
 await page.locator('#connect').click();
 console.log('Connecting to simulated FM-1…');
 await page.waitForFunction(()=>dev?.dump && busy===0 && !connecting);
 console.log('Connected; exercising widgets.');
 await page.evaluate(()=>mock.stop()); // deterministic tests, no simulated knob timer
 assert.equal(await page.locator('#groups > .unified-card').count(),8);
 assert.equal(await page.locator('#groups .studio-widget').count(),6);
 const integrity=()=>page.evaluate(()=>{
  const problems=[];
  for(const [key,c] of ctl) if(!c.input.isConnected || !c.row.isConnected) problems.push(key);
  const params=[...document.querySelectorAll('[data-param]')].map(e=>e.dataset.param);
  if(new Set(params).size!==params.length)problems.push('duplicate parameter');
  return problems;
 });
 assert.deepEqual(await integrity(),[]);
 const value=id=>page.evaluate(id=>dev.dump.p[id],id);
 const waitValue=(id,v)=>page.waitForFunction(([id,v])=>dev.dump.p[id]===v,[id,v]);
 const attack=page.locator('.widget-env [role=slider]').nth(0);
 await attack.press('Home');await waitValue(1,0);
 await attack.press('Shift+ArrowRight');await waitValue(1,10);
 await attack.dblclick();
 await page.waitForFunction(()=>dev.dump.p[1]===ctl.get('0:1').desc.def);
 const sustain=page.locator('.widget-env [role=slider]').nth(2);
 await sustain.scrollIntoViewIfNeeded();
 const box=await sustain.boundingBox();
 const beforeSustain=await value(3);
 await page.mouse.move(box.x+box.width/2,box.y+box.height/2);await page.mouse.down();
 assert.equal(await value(3),beforeSustain,'handle press must not jump the value');
 await page.mouse.move(box.x+box.width/2,box.y-30,{steps:5});await page.mouse.up();
 assert((await value(3))>beforeSustain);
 assert.equal(await page.locator('.drag-ghost').count(),0);
 await page.evaluate(()=>mock.sim.param(mock.state.sel,4,127));
 await page.waitForFunction(()=>document.querySelectorAll('.widget-env [role=slider]')[3].getAttribute('aria-valuenow')==='127');
 await page.locator('.widget-filter [role=slider]').press('End');
 const filterId=await page.evaluate(()=>dev.pdesc.findIndex(d=>d?.label==='CUT'));
 await waitValue(filterId,127);
 await page.locator('.widget-filter [role=slider]').dblclick();
 await page.waitForFunction(id=>dev.dump.p[id]===ctl.get(`0:${id}`).desc.def,filterId);
 await page.locator('.widget-lfo button').filter({hasText:'SQR'}).click();await waitValue(10,3);
 await page.locator('.widget-scale button').nth(7).click();await waitValue(25,7);
 await page.locator('.widget-arp button').nth(2).click();await waitValue(19,3);
 const hold=await value(23);await page.locator('.widget-arp button').last().click();await waitValue(23,hold?0:1);
 await page.locator('[data-param="0:17"] select').selectOption('1');
 await page.waitForFunction(()=>document.querySelector('.widget-arp .widget-line').getAttribute('d').split('M').length===17);
 // Ordinary controls update the diagrams too, including wrap at the last pattern.
 await page.locator('[data-param="0:46"] input').fill('16');
 await page.locator('[data-param="0:46"] input').dispatchEvent('change');
 await page.locator('.widget-slicer button').last().click();await waitValue(46,1);
 await page.waitForFunction(()=>document.querySelector('.widget-slicer .widget-caption').textContent.includes('1/16'));
 await page.evaluate(()=>FeluccaI18n.setLanguage('fr'));
 assert.equal(await attack.getAttribute('aria-label'),'Attaque');
 assert.match(await page.locator('.mod-head').innerText(),/Intensité/);
 await page.evaluate(()=>FeluccaI18n.setLanguage('en'));
 const env=page.locator('#groups > [data-section=ENV]');
 await env.locator('.card-grip').press('Home');
 assert.equal(await page.locator('#groups > .group').first().getAttribute('data-section'),'ENV');
 // All 13 engines retain their visible parameter controls; filter guide is ANALOG-only.
 const engines=await page.locator('#engine option').evaluateAll(es=>es.filter(e=>!e.disabled&&!e.hidden).map(e=>e.value));
 for(const engine of engines) {
  console.log('Checking engine',engine);
  await page.locator('#engine').selectOption(engine);
  await page.waitForFunction(e=>busy===0 && dev.dump.engine===+e,engine);
  assert.deepEqual(await integrity(),[],`engine ${engine}`);
  assert.equal(await page.locator('.widget-filter').count(),await page.evaluate(()=>engName()==='ANALOG')?1:0);
 }
 await page.locator('#engine').selectOption('0');
 await page.waitForFunction(()=>busy===0 && dev.dump.engine===0);
 // Theme/control-mode and viewport matrix checks document and panel overflow.
 for(const controls of ['sliders','knobs'])for(const width of [320,390,768,1440]) {
  await page.setViewportSize({width,height:1100});
  for(const mode of ['light','dark'])for(const theme of ['stage','matrix','dx','modeld','chocolate','vapor','midnight','space','bauhaus','ocean','arcade','hicon']) {
   await page.evaluate(({theme,controls,mode})=>{
    document.documentElement.dataset.skin=theme;
    document.documentElement.dataset.mode=mode;
    document.documentElement.dataset.contrast=theme==='hicon'?'high':'normal';
    document.querySelectorAll('[data-card-key]').forEach(c=>c.dataset.controls=controls);
   },{theme,controls,mode});
   const overflow=await page.evaluate(()=>[...document.querySelectorAll('#groups > .group')].filter(e=>e.scrollWidth>e.clientWidth+2).map(e=>e.dataset.section));
   assert.deepEqual(overflow,[],`${controls} ${width} ${theme} panel overflow`);
   assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth+2),`${controls} ${width} ${theme} page overflow`);
  }
 }
 await page.setViewportSize({width:1440,height:1100});
 await page.evaluate(()=>{document.documentElement.dataset.skin='midnight';document.documentElement.dataset.contrast='normal';document.querySelectorAll('[data-card-key]').forEach(c=>c.dataset.controls='sliders');});
 await mkdir('build/screenshots',{recursive:true});
 await page.screenshot({path:'build/screenshots/studio-panels-desktop.png',fullPage:true,animations:'disabled'});
 await page.setViewportSize({width:390,height:844});
 await page.screenshot({path:'build/screenshots/studio-panels-mobile.png',fullPage:true,animations:'disabled'});
 for(const language of ['en','es','fr','de','ru','zh-CN','pt-BR','ja']) {
  await page.evaluate(language=>FeluccaI18n.setLanguage(language),language);
  assert.equal(await page.locator('.mod-head [data-i18n]').count(),3);
  assert(await page.evaluate(()=>[...document.querySelectorAll('.studio-widget')].every(e=>e.scrollWidth<=e.clientWidth+2)),`${language} widget text overflow`);
 }
 await page.evaluate(()=>{FeluccaI18n.setLanguage('en');document.documentElement.dataset.skin='space';document.documentElement.dataset.mode='light';document.documentElement.dataset.contrast='normal';});
 await page.screenshot({path:'build/screenshots/studio-panels-light-mobile.png',fullPage:true,animations:'disabled'});
 await page.setViewportSize({width:1440,height:1100});
 await page.evaluate(()=>{document.documentElement.dataset.skin='midnight';document.documentElement.dataset.mode='dark';document.querySelectorAll('[data-card-key]').forEach(c=>c.dataset.controls='knobs');});
 await page.screenshot({path:'build/screenshots/studio-panels-knobs.png',fullPage:true,animations:'disabled'});
 // The unknown-layout fallback must stay editable without fixed-ID diagrams.
 await page.evaluate(()=>{dev.info={...dev.info,pcount:92};buildGroups();});
 assert.equal(await page.locator('.studio-widget').count(),0);
 assert((await page.locator('#groups input').count())>0);
 // Actual browser WASM/audio startup, separate from the simulated MIDI connection.
 await page.goto(url+'?audio=1&theme=midnight');
 console.log('Browser audio page loaded.');
 await page.waitForFunction(()=>typeof window.dev!=='undefined');
 await page.locator('#connect').click();
 await page.waitForFunction(()=>dev?.dump && busy===0 && !connecting,{},{timeout:30000});
 assert.equal(await page.locator('.widget-env').count(),1);
 await page.locator('.widget-lfo button').filter({hasText:'TRI'}).click();await waitValue(10,1);
 await page.locator('.keyboard-tray').click();
 const note=page.locator('#play-keys button[data-note="60"]');
 await note.focus();await page.keyboard.down('Space');
 await page.waitForFunction(()=>{
  const samples=new Float32Array(2048);browserSynth.analyser.getFloatTimeDomainData(samples);
  return samples.some(v=>Math.abs(v)>.001);
 });
 await page.keyboard.up('Space');
 assert.deepEqual(errors,[]);
 console.log(`Studio browser checks passed: widget editing/reset and device pushes, ${engines.length} engines, card ordering, eight languages, 192 theme/layout combinations, unknown-layout fallback, audible browser DSP output.`);
} finally {await browser.close();}
