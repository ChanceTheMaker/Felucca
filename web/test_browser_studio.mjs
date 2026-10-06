// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {builtSiteFixture} from './test_site_fixture.mjs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:'chrome',headless:true});
try {
 const page=await browser.newPage({viewport:{width:1440,height:1100}});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 const html=await builtSiteFixture(page,process.env.STUDIO_FIXTURE_DIR);
 await page.route('**/webapp/editor/?*',async route=>{
  const source=html?await html():await (await route.fetch()).text();
  await route.fulfill({contentType:'text/html',body:source.replace('if (!MOCK && !navigator.requestMIDIAccess) sayK("nomidi");',`
   Object.defineProperties(window,{dev:{get:()=>dev},busy:{get:()=>busy},mock:{get:()=>mock},browserSynth:{get:()=>browserSynth}});
  `)});
 });
 await page.addInitScript(()=>{
  navigator.requestMIDIAccess=()=>{throw Error('Browser mode requested MIDI access');};
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
 });
 await page.goto('http://127.0.0.1:8768/rc1/webapp/editor/?audio=1');
 await page.locator('#connect').click();await page.waitForFunction(()=>window.dev?.dump && !window.busy);
 await page.locator('#tabs [data-tab=fm6]').click();await page.locator('#fm6ops input').first().waitFor();
 await page.locator('#fm6init').click();await page.locator('#fm6send').click();
 await page.waitForFunction(()=>window.dev.dump.engine===12 && !window.busy);
 await page.locator('#fm6read').click();await page.waitForFunction(()=>!window.busy);
 assert.equal(await page.locator('#fm6trk option').count(),4);
 // The live editor sends real patch data, and the real analyser sees sound.
 await page.locator('#fm6live').check();
 const input=page.locator('#fm6voice input[title=FB]');await input.fill('5');await input.dispatchEvent('change');
 await page.waitForFunction(()=>FM6.unpack(window.mock.state.tracks[0].fm6)[FM6.VI.FB]===5 && !window.busy);
 await page.evaluate(()=>window.browserSynth.midi([0x90,60,100]));
 await page.waitForFunction(()=>{
  const a=new Float32Array(window.browserSynth.analyser.fftSize);window.browserSynth.analyser.getFloatTimeDomainData(a);
  return a.some(v=>Math.abs(v)>.001);
 });
 await page.evaluate(()=>window.browserSynth.midi([0x80,60,0]));
 await page.locator('#tabs [data-tab=tracks]').click();await page.locator('#mixer').waitFor();
 await page.locator('#tabs [data-tab=sequencer]').click();
 await page.locator('#browser-seq-play').click();
 await page.waitForFunction(()=>window.browserSynth.playing && window.browserSynth.positions.some(p=>p>0 && p<64));
 await page.locator('#browser-seq-stop').click();await page.waitForFunction(()=>!window.browserSynth.playing);
 await page.locator('#trackbtns [data-i="2"]').click();await page.waitForFunction(()=>window.dev.sel===2 && !window.busy);
 await page.locator('#tabs [data-tab=settings]').click();await page.locator('#p-settings').waitFor();
 assert.equal(await page.locator('#setgroups [data-param="1:2"]').count(),0);
 assert.deepEqual(errors,[]);
 console.log('Browser Studio: FM6 tab, send/read, automatic engine selection, live editing and real audio pass without MIDI access.');
}finally{await browser.close();}
