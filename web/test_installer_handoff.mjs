// SPDX-License-Identifier: GPL-3.0-only
// UI-only simulation: the updater and MIDI permission are replaced in this test's browser context.
import assert from 'node:assert/strict';
import {mkdir,readFile} from 'node:fs/promises';
import {resolve,sep,extname} from 'node:path';
import {fileURLToPath} from 'node:url';
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
  window.permissionCalls=0;
  navigator.requestMIDIAccess=async()=>{
   window.permissionCalls++;
   if(window.scenario==='denied') throw new Error('Permission declined');
   return {};
  };
  window.TestUpdater=class {
   async find(){
    if(window.scenario==='notfound') throw Object.assign(new Error('FM-1 not found'),{code:'notfound'});
    return window.scenario.startsWith('resume') ? {link:{close(){}}} : null;
   }
   async run(step) {
    step('start');
    for(let n=1;n<=100;n++) step('write',n);
    await new Promise(resolve=>window.finishInstall=resolve);
    if(window.scenario==='failed') throw Object.assign(new Error('Simulated disconnect'),{code:'lost'});
    if(window.scenario==='resume-stopped') return false;
    step('reboot'); step('done','FM-1_910'); return true;
   }
   async install(image,product,step){window.lastProduct=product;return this.run(step);}
   async resume(image,step,product){window.lastProduct=product;return this.run(step);}
  };
 });
 page=await context.newPage();
 page.setDefaultTimeout(10000);
 page.setDefaultNavigationTimeout(20000);
 page.on('pageerror',e=>errors.push(e.message));
 const root=fileURLToPath(new URL('../build/site/',import.meta.url));
 await page.route(base+'/**',async route=>{
  const pathname=decodeURIComponent(new URL(route.request().url()).pathname);
  const file=resolve(root,'.'+pathname+(pathname.endsWith('/')?'index.html':''));
  if(!file.startsWith(resolve(root)+sep)) return route.abort();
  let body=await readFile(file);
  if(pathname==='/webapp/installer/') {
   const source=body.toString('utf8'),marker='const $ = (id) => document.getElementById(id);';
   assert(source.includes(marker));
   let injected=source.replace(marker,'Updater = window.TestUpdater;\n'+marker);
   if(source.includes('$("stock-file").addEventListener')) {
    injected=injected.replace('$("stock-file").addEventListener',
     'window.prepareTestStock=()=>{stock={image:new Uint8Array(1),product:"STOCK-TEST"};lockInstaller(false);};\n$("stock-file").addEventListener');
   }
   body=Buffer.from(injected);
  }
  const mime={'.html':'text/html; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.jpg':'image/jpeg','.ttf':'font/ttf','.woff2':'font/woff2'};
  await route.fulfill({contentType:mime[extname(file)]||'application/octet-stream',body});
 });
 await page.goto(base+'/webapp/installer/',{waitUntil:'domcontentloaded'});
 const go=page.locator('#go'), handoff=page.locator('#studio-handoff');
 await page.waitForFunction(()=>!document.querySelector('#go').disabled);
 assert.equal(await handoff.isVisible(),false);
 assert.equal(await page.locator('#retry').isVisible(),false,'Retry starts hidden');
 assert.equal(await page.locator('#install-progress').evaluate(e=>e.open),false,'Progress starts collapsed');
 assert.equal(await page.locator('#bar').isVisible(),false);
 assert.equal(await page.locator('#status').isVisible(),false);
 assert.equal(await page.locator('#install-progress summary').textContent(),'Install Progress');
 for(const [scenario,width] of [['success',1440],['resume-success',390],['failed',390],['resume-stopped',390]]) {
  if(await page.locator('#install-progress').evaluate(e=>e.open)) {
   await page.locator('#install-progress summary').click();
   assert.equal(await page.locator('#bar').isVisible(),false,'Accordion can be collapsed');
  }
  await page.setViewportSize({width,height:844});
  await page.evaluate(s=>{window.scenario=s;window.finishInstall=null;scrollTo({top:0,behavior:'instant'});},scenario);
  await go.click();
  await page.waitForFunction(()=>!!window.finishInstall);
  assert.equal(await page.locator('#install-progress').evaluate(e=>e.open),true,'Install opens progress');
  assert.equal(await handoff.isVisible(),false,'No Studio button during a write');
  assert(await go.isDisabled(),'Install locks during the write');
  await page.waitForFunction(()=>{
   const y=document.querySelector('#install-progress').getBoundingClientRect().top;
   return y>=0 && y<=25;
  });
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
   await page.evaluate(()=>{window.scenario='denied';scrollTo({top:0,behavior:'instant'});});
   await go.click();
   await page.waitForFunction(()=>!document.querySelector('#go').disabled);
   assert.equal(await handoff.isVisible(),false);
   assert.equal(await page.locator('#log').textContent(),'');
  }
 }
 for(const scenario of ['notfound','denied','failed','resume-stopped']) {
  await page.evaluate(s=>{window.scenario=s;window.finishInstall=null;},scenario);
  await go.click();
  if(['failed','resume-stopped'].includes(scenario)) {
   await page.waitForFunction(()=>!!window.finishInstall);
   assert.equal(await page.locator('#retry').isVisible(),false,'No retry during a write');
   await page.evaluate(()=>window.finishInstall());
  }
  await page.locator('#retry').waitFor({state:'visible'});
  assert.equal(await handoff.isVisible(),false,'Failure never offers Studio');
  assert(await page.evaluate(()=>document.querySelector('#retry').getBoundingClientRect().top>=document.querySelector('#log').getBoundingClientRect().bottom),'Retry is below the output');
  for(const code of ['en','ja','es','fr','de','ru','zh-CN','pt-BR']) {
   await page.evaluate(code=>window.FeluccaI18n.setLanguage(code),code);
   assert.equal(await page.locator('#retry').textContent(),await page.evaluate(()=>window.FeluccaI18n.t('installer.retry')));
  }
  await page.evaluate(()=>window.FeluccaI18n.setLanguage('en'));
  if(scenario==='notfound') {
   await page.waitForFunction(()=>{
    const y=document.querySelector('#install-progress').getBoundingClientRect().top;
    return y>=0 && y<=25;
   });
   await page.screenshot({path:'build/screenshots/install-retry-mobile.png'});
  }
  const calls=await page.evaluate(()=>window.permissionCalls);
  await page.evaluate(()=>{window.scenario='success';window.finishInstall=null;});
  await page.locator('#retry').click();
  await page.waitForFunction(()=>!!window.finishInstall);
  assert.equal(await page.evaluate(()=>window.permissionCalls),calls+1,'Retry requests a fresh MIDI connection');
  assert.equal(await page.locator('#retry').isVisible(),false,'Retry clears immediately');
  assert(await go.isDisabled(),'Cannot start a concurrent install');
  await page.evaluate(()=>window.finishInstall());
  await handoff.waitFor({state:'visible'});
  assert.equal(await page.locator('#retry').isVisible(),false,'Successful retry replaces retry with Studio');
 }
 if(await page.locator('#stock-go').count()) {
  await page.locator('details').filter({has:page.locator('#stock-go')}).locator('summary').click();
  await page.evaluate(()=>{window.prepareTestStock();window.scenario='notfound';});
  await page.locator('#stock-go').click();
  await page.locator('#retry').waitFor({state:'visible'});
  await page.locator('#stock-recovery').check();
  await page.evaluate(()=>{window.scenario='resume-success';window.finishInstall=null;});
  page.once('dialog',dialog=>dialog.accept());
  await page.locator('#retry').click();
  await page.waitForFunction(()=>!!window.finishInstall);
  assert.equal(await page.evaluate(()=>window.lastProduct),'STOCK-TEST','Official recovery retry retains its selected package');
  assert(await page.locator('#stock-go').isDisabled());
  await page.evaluate(()=>window.finishInstall());
  await page.waitForFunction(()=>!document.querySelector('#stock-go').disabled);
  assert.equal(await page.locator('#retry').isVisible(),false);
  assert.equal(await handoff.isVisible(),false,'Official recovery never offers the Felucca editor');
 }
 assert.deepEqual(errors,[]);
 console.log('Installer handoff and retry: desktop/mobile, not found, permission denied, failed write, stopped resume, fresh connection, successful retry, progress locking, and eight-language labels passed. No hardware was accessed.');
} catch(error) {
 console.error(errors);
 if(page) console.error(await page.locator('#status').textContent().catch(()=>''));
 throw error;
} finally {await browser.close();}
