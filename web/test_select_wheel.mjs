// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {builtSiteFixture} from './test_site_fixture.mjs';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:process.env.BROWSER_CHANNEL || 'chrome',headless:true});
try {
 const page=await browser.newPage({viewport:{width:1440,height:1100}});
 page.setDefaultTimeout(15000);
 const errors=[];page.on('pageerror',e=>errors.push(e.message));
 // Optional deterministic built-site fixture when a shared preview connection
 // is unreliable. Use STUDIO_FIXTURE_DIR only for isolated browser tests.
 await builtSiteFixture(page,process.env.STUDIO_FIXTURE_DIR);
 await page.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  localStorage.setItem('felucca.web.soundSourceTipDismissed','1');
  localStorage.setItem('felucca.web.keyboardCollapsed','true');
 });
 await page.goto((process.env.STUDIO_URL || 'http://127.0.0.1:8768/rc1/webapp/editor/')+'?mock=1',{waitUntil:'domcontentloaded'});
 await page.locator('#connect').click();
 await page.locator('.widget-env').waitFor();
 await page.waitForFunction(()=>!document.querySelector('#connect').disabled);
 const select=page.locator('[data-param="0:17"] select');
 const open=()=>select.evaluate(e=>e.matches(':open'));
 await select.scrollIntoViewIfNeeded();
 await select.hover();await page.mouse.wheel(0,100);
 assert.equal(await select.inputValue(),'0','closed hovered select stays unchanged');
 await select.click();assert(await open());
 const option=select.locator('option').nth(1);
 await option.hover();await page.mouse.wheel(0,100);
 await page.waitForFunction(()=>document.querySelector('[data-param="0:17"] select').value==='1');
 assert(await open(),'wheel keeps menu open');
 assert.match(await page.locator('[data-param="0:17"] .v').innerText(),/UP/,'normal parameter change handler ran');
 await option.hover();await page.mouse.wheel(0,-100);
 await page.waitForFunction(()=>document.querySelector('[data-param="0:17"] select').value==='0');
 // End stop, rather than wrapping, and pointer outside the open menu.
 await page.mouse.wheel(0,-100);
 assert.equal(await select.inputValue(),'0');
 await page.mouse.move(3,3);await page.mouse.wheel(0,100);
 assert.equal(await select.inputValue(),'0');
 await page.keyboard.press('Escape');assert.equal(await open(),false);
 await select.click();await option.hover();await page.mouse.wheel(0,100);
 await page.waitForFunction(()=>document.querySelector('[data-param="0:17"] select').value==='1');
 await page.keyboard.press('Escape');
 assert.equal(await select.inputValue(),'1','closing retains the applied wheel value');
 // Eligibility: hidden aliases and disabled choices must never be applied.
 await select.evaluate(e=>{e.options[2].hidden=true;e.options[3].disabled=true;});
 await select.click();await option.hover();await page.mouse.wheel(0,100);
 await page.waitForFunction(()=>document.querySelector('[data-param="0:17"] select').value==='4');
 await page.keyboard.press('Escape');
 await select.evaluate(e=>{e.options[2].hidden=false;e.options[3].disabled=false;});
 // Waveform selects keep rich icons and their existing change handlers.
 const wave=page.locator('[data-param="0:10"] select');
 await wave.click();await wave.locator('option').nth(1).hover();await page.mouse.wheel(0,100);
 await page.waitForFunction(()=>document.querySelector('[data-param="0:10"] select').value==='1');
 assert.equal(await page.locator('.widget-lfo button[aria-pressed=true]').innerText(),'TRI');
 await page.keyboard.press('Escape');
 // Native keyboard operation remains available when the menu is closed.
 await select.focus();await page.keyboard.press('Space');await page.keyboard.press('ArrowDown');await page.keyboard.press('Enter');
 assert.equal(await select.inputValue(),'5');
 assert.deepEqual(errors,[]);
 console.log('Open-select wheel: live values, open/hover gating, boundaries, hidden/disabled skip, Escape, waveform graphs and keyboard input pass.');
}finally{await browser.close();}
