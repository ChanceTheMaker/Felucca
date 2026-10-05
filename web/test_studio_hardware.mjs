// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {mkdir} from 'node:fs/promises';
import {builtSiteFixture} from './test_site_fixture.mjs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:process.env.BROWSER_CHANNEL || 'chrome',headless:true});
try {
 const page=await browser.newPage({viewport:{width:1440,height:1100}});
 page.setDefaultTimeout(20000);
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await builtSiteFixture(page,process.env.STUDIO_FIXTURE_DIR);
 await page.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
  localStorage.setItem('felucca.web.keyboardCollapsed','true');
 });
 await page.goto((process.env.STUDIO_URL || 'http://127.0.0.1:8768/rc1/webapp/editor/')+'?audio=1&theme=midnight');
 await page.locator('#connect').click();await page.locator('.widget-env').waitFor();
 await page.waitForFunction(()=>!document.querySelector('#connect').disabled);
 const settle=()=>page.evaluate(()=>new Promise(resolve=>requestAnimationFrame(()=>requestAnimationFrame(()=>requestAnimationFrame(resolve)))));
 const check=()=>page.evaluate(()=>{
  const problems=[],rect=e=>e.getBoundingClientRect();
  for(const filler of document.querySelectorAll('.bank-hardware,.bank-hardware-gap')) {
   if(!filler.checkVisibility())continue;
   const f=rect(filler),card=filler.closest('.unified-card'),c=rect(card);
   if(f.left<c.left || f.right>c.right+1 || f.top<c.top || f.bottom>c.bottom+1)problems.push('outside card');
   if(filler.getAttribute('aria-hidden')!=='true'||!filler.inert||getComputedStyle(filler).pointerEvents!=='none')problems.push('interactive decoration');
   for(const control of card.querySelectorAll('.row,.studio-widget,h2,h3,.card-tools')) {
    const r=rect(control);
    if(Math.min(f.right,r.right)-Math.max(f.left,r.left)>1 && Math.min(f.bottom,r.bottom)-Math.max(f.top,r.top)>1)problems.push(`covers ${control.className}`);
   }
  }
  return problems;
 });
 for(const controls of ['knobs','sliders'])for(const width of [390,768,1440])for(const mode of ['light','dark'])for(const theme of ['stage','space','midnight','hicon']) {
  await page.setViewportSize({width,height:1100});
  await page.evaluate(({controls,mode,theme})=>{
   document.documentElement.dataset.skin=theme;document.documentElement.dataset.mode=mode;
   document.documentElement.dataset.contrast=theme==='hicon'?'high':'normal';
   document.querySelectorAll('[data-card-key]').forEach(e=>e.dataset.controls=controls);
  },{controls,mode,theme});
  await settle();assert.deepEqual(await check(),[],`${controls} ${width} ${mode} ${theme}`);
 }
 await page.setViewportSize({width:1440,height:1100});
 await page.evaluate(()=>{
  document.documentElement.dataset.skin='midnight';document.documentElement.dataset.mode='dark';document.documentElement.dataset.contrast='normal';
  document.querySelectorAll('[data-card-key]').forEach(e=>e.dataset.controls='knobs');
 });
 await settle();
 assert((await page.locator('.bank-hardware').count())>0);
 assert((await page.locator('.hardware-grille').count())>0);
 assert((await page.locator('.hardware-panel:not(.hardware-grille)').count())>(await page.locator('.hardware-grille').count()));
 assert((await page.locator('.hardware-grille').count())<=2);
 assert.equal(await page.locator('.hardware-plate,.hardware-cone,.hardware-jack-rim,.bank-hardware svg,.bank-hardware-gap svg').count(),0);
 await mkdir('build/screenshots',{recursive:true});
 await page.screenshot({path:'build/screenshots/studio-hardware-desktop.png',fullPage:true,animations:'disabled'});
 // Removing decorative nodes must leave every card and real control in place.
 const geometry=()=>page.locator('#groups > .group,#groups .row').evaluateAll(es=>es.map(e=>{const r=e.getBoundingClientRect();return [r.x,r.y,r.width,r.height];}));
 const before=await geometry();
 await page.evaluate(()=>document.querySelectorAll('.bank-hardware,.bank-hardware-gap').forEach(e=>e.remove()));await settle();
 const after=await geometry();assert.equal(before.length,after.length);
 before.forEach((rect,i)=>rect.forEach((v,j)=>assert(Math.abs(v-after[i][j])<1,'decoration changed control/card geometry')));
 await page.evaluate(()=>FeluccaHardware.decorate(document.getElementById('groups')));await settle();
 assert.deepEqual(await check(),[]);
 await page.setViewportSize({width:390,height:844});await settle();
 await page.locator('[data-section=VOICE]').screenshot({path:'build/screenshots/studio-hardware-mobile.png',animations:'disabled'});
 assert.deepEqual(errors,[]);
 console.log('Hardware fillers: 48 layout/theme combinations; mostly plain panels, at most two grilles; no control overlap, interaction or card resizing; rebuild passes.');
}finally{await browser.close();}
