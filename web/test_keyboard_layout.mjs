// SPDX-License-Identifier: GPL-3.0-only
// STUDIO_URL and PLAYWRIGHT_MODULE work as in test_studio_panels.mjs.
import assert from 'node:assert/strict';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:process.env.BROWSER_CHANNEL || 'chrome',headless:true});
try {
 const page=await browser.newPage({viewport:{width:1440,height:1000}});
 page.setDefaultTimeout(30000);
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
 });
 await page.goto((process.env.STUDIO_URL || 'http://127.0.0.1:8768/rc1/webapp/editor/')+'?mock=1');
 await page.locator('#connect').click();
 await page.waitForFunction(()=>document.querySelector('#play-keys button')?.disabled===false);
 const mapping=await page.locator('#play-keys button').evaluateAll(es=>es.map(e=>({
  key:e.querySelector('small').textContent.trim(),note:+e.dataset.note,black:e.classList.contains('black')
 })).filter(e=>e.key));
 const home=new Set(['A','S','D','F','G','H','J','K','L',';']);
 assert.equal(mapping.length,17);
 for(const {key,note,black} of mapping) {
  assert.equal(black,!home.has(key),`${key} must match its piano row`);
  await page.locator('#play-keys button').first().focus();
  const code=key===';'?'Semicolon':`Key${key}`;
  await page.keyboard.down(code);
  assert.equal(await page.locator(`#play-keys button[data-note="${note}"]`).getAttribute('aria-pressed'),'true',`${key} note on`);
  await page.keyboard.up(code);
  assert.equal(await page.locator('#play-keys button[aria-pressed=true]').count(),0,`${key} note off`);
 }
 await page.keyboard.down('KeyT');
 assert.equal(await page.locator('#play-keys button[aria-pressed=true]').count(),0);
 await page.keyboard.up('KeyT');
 assert.deepEqual(errors,[]);
 console.log('Typing keyboard: all 17 displayed shortcuts match white/black rows and produce/release the matching note; T stays unused.');
} finally {await browser.close();}
