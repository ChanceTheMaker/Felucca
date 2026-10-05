// SPDX-License-Identifier: GPL-3.0-only
// UI-only simulation: the updater and MIDI permission are replaced in this test's browser context.
import assert from 'node:assert/strict';
import {mkdir,readFile} from 'node:fs/promises';
const {chromium}=await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser=await chromium.launch({channel:'chrome',headless:true});
const base=process.env.SITE_URL || 'http://127.0.0.1:8871';
let page;
const errors=[];
try {
 const context=await browser.newContext({viewport:{width:1440,height:1100}});
 await context.addInitScript(()=>{
  localStorage.setItem('felucca.web.analyticsConsent','denied');
  window.scenario='success';
  navigator.requestMIDIAccess=async()=>{
   if(window.scenario==='denied') throw new Error('Permission declined');
   return {};
  };
  window.TestUpdater=class {
   async find(){return window.scenario.startsWith('resume') ? {link:{close(){}}} : null;}
   async run(step) {
    step('start');
    for(let n=1;n<=100;n++) step('write',n);
    await new Promise(resolve=>window.finishInstall=resolve);
    if(window.scenario==='failed') throw Object.assign(new Error('Simulated disconnect'),{code:'lost'});
    if(window.scenario==='resume-stopped') return false;
    step('reboot'); step('done','FM-1_910'); return true;
   }
   async install(image,product,step){return this.run(step);}
   async resume(image,step,product){return this.run(step);}
  };
 });
 page=await context.newPage();
 page.setDefaultTimeout(10000);
 page.setDefaultNavigationTimeout(20000);
 page.on('pageerror',e=>errors.push(e.message));
 await page.route('**/webapp/installer/',async route=>{
  const source=await readFile(new URL('../build/site/webapp/installer/index.html',import.meta.url),'utf8');
  const marker='const $ = (id) => document.getElementById(id);';
  assert(source.includes(marker));
  await route.fulfill({contentType:'text/html; charset=utf-8',body:source.replace(marker,'Updater = window.TestUpdater;\n'+marker)});
 });
 await page.goto(base+'/webapp/installer/',{waitUntil:'domcontentloaded'});
 const go=page.locator('#go'), handoff=page.locator('#studio-handoff');
 await page.waitForFunction(()=>!document.querySelector('#go').disabled);
 assert.equal(await handoff.isVisible(),false);
 for(const [scenario,width] of [['success',1440],['resume-success',390],['failed',390],['resume-stopped',390]]) {
  await page.setViewportSize({width,height:844});
  await page.evaluate(s=>{window.scenario=s;window.finishInstall=null;scrollTo(0,0);},scenario);
  await go.click();
  await page.waitForFunction(()=>!!window.finishInstall);
  assert.equal(await handoff.isVisible(),false,'No Studio button during a write');
  assert(await go.isDisabled(),'Install locks during the write');
  const progress=await page.locator('#install-progress').boundingBox();
  assert(progress.y>=0 && progress.y<=25,`Install scrolls to progress at ${width}px: ${progress.y}`);
  assert.equal(await page.evaluate(()=>document.activeElement.id),'install-progress');
  assert((await page.locator('#log').boundingBox()).height<=325,'Output remains bounded');
  await page.evaluate(()=>window.finishInstall());
  await page.waitForFunction(()=>!document.querySelector('#go').disabled);
  const success=scenario.endsWith('success');
  assert.equal(await handoff.isVisible(),success,scenario);
  if(success) {
   assert.equal(await handoff.locator('a').textContent(),'Edit in Studio');
   assert.equal(await handoff.locator('a').getAttribute('href'),'../editor/#sound');
   const output=await page.locator('#log').boundingBox(),button=await handoff.boundingBox();
   assert(button.y>=output.y+output.height,'Handoff is below output');
   assert.equal(await page.locator('#bar').getAttribute('value'),'100');
   for(const code of ['en','ja','es','fr','de','ru','zh-CN','pt-BR']) {
    await page.evaluate(code=>window.FeluccaI18n.setLanguage(code),code);
    assert.equal(await handoff.locator('a').textContent(),await page.evaluate(()=>window.FeluccaI18n.t('installer.editStudio')));
   }
   await page.evaluate(()=>window.FeluccaI18n.setLanguage('en'));
   await mkdir('build/screenshots',{recursive:true});
   await page.screenshot({path:`build/screenshots/install-success-${width}.png`});
   // A new attempt must immediately remove the previous success action, even if permission fails.
   await page.evaluate(()=>{window.scenario='denied';scrollTo(0,0);});
   await go.click();
   await page.waitForFunction(()=>!document.querySelector('#go').disabled);
   assert.equal(await handoff.isVisible(),false);
   assert.equal(await page.locator('#log').textContent(),'');
  }
 }
 assert.deepEqual(errors,[]);
 console.log('Installer handoff: desktop/mobile scroll, normal/resumed success, write failure, stopped resume, denied retry, output bounds, and eight-language labels passed. No hardware was accessed.');
} catch(error) {
 console.error(errors);
 if(page) console.error(await page.locator('#status').textContent().catch(()=>''));
 throw error;
} finally {await browser.close();}
