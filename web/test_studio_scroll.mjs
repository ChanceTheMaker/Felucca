// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {builtSiteFixture} from './test_site_fixture.mjs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:'chrome',headless:true});
try {
 const page=await browser.newPage({viewport:{width:1440,height:1000}});
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await builtSiteFixture(page,process.env.STUDIO_FIXTURE_DIR);
 await page.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
  localStorage.setItem('felucca.web.keyboardCollapsed','false');
 });
 await page.goto('http://127.0.0.1:8768/rc1/webapp/editor/?mock=1#sound');
 await page.locator('#connect').click();await page.locator('.widget-env').waitFor();
 const button=page.locator('#back-to-top');
 assert.equal(await button.isVisible(),false);
 for(const width of [1440,390])for(const reducedMotion of ['no-preference','reduce']) {
  await page.setViewportSize({width,height:1000});await page.emulateMedia({reducedMotion});
  for(const collapsed of [false,true]) {
   if(await page.locator('.play-keyboard').evaluate(e=>e.classList.contains('collapsed'))!==collapsed)await page.locator('.keyboard-tray').click();
   await page.evaluate(()=>window.scrollTo({top:600,behavior:'instant'}));
   await button.waitFor({state:'visible'});
   await page.waitForFunction(()=>{
    const b=document.querySelector('#back-to-top').getBoundingClientRect(),k=document.querySelector('.play-keyboard').getBoundingClientRect();
    return Math.abs(k.top-b.bottom-12)<2;
   });
   const box=await button.boundingBox();assert(box.x+box.width<=width && box.y>0);
   await button.click();
   await page.waitForFunction(()=>scrollY===0);
   await button.waitFor({state:'hidden'});
   assert.equal(await button.isVisible(),false);
   assert.equal(await page.evaluate(()=>document.activeElement.id),'studio');
   assert.equal(new URL(page.url()).hash,'#sound');
  }
 }
 await page.evaluate(()=>window.scrollTo({top:600,behavior:'instant'}));
 await button.waitFor({state:'visible'});
 await page.screenshot({path:'build/screenshots/studio-back-to-top.png',animations:'disabled'});
 assert.deepEqual(errors,[]);
 console.log('Studio back-to-top: desktop/mobile, expanded/collapsed keyboard, smooth/reduced-motion scrolling, focus, preserved tab and bounds pass.');
}finally{await browser.close();}
