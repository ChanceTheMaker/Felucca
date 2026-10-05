// SPDX-License-Identifier: GPL-3.0-only
// Optional browser integration test. Requires Playwright and an installed Chrome.
// SITE_URL defaults to the isolated preview; PLAYWRIGHT_MODULE may point to a local install.
import assert from 'node:assert/strict';
import { mkdir } from 'node:fs/promises';
const { chromium } = await import(process.env.PLAYWRIGHT_MODULE || 'playwright');
const base = process.env.SITE_URL || 'http://127.0.0.1:8768';
await mkdir('build/screenshots', { recursive: true });
const browser = await chromium.launch({ channel: 'chrome', headless: true });
const context = await browser.newContext({ viewport: { width: 1440, height: 1100 } });
const page = await context.newPage();
const errors = [];
page.on('pageerror', e => errors.push(e.message));
const editor = `${base}/webapp/editor/?mock=1#sound`;
const noOverflow = async () => assert(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth + 1), 'No page-wide horizontal overflow');
const openMenu = async () => { if (!(await page.locator('.settings-menu').getAttribute('open') !== null)) await page.getByLabel('Website settings', {exact:true}).click(); };
const preference = async (label,value) => { await openMenu(); await page.getByLabel(label,{exact:true}).selectOption(value); };
try {
  await page.goto(editor);
  const consent = page.locator('.analytics-banner [data-choice=denied]');
  if (await consent.isVisible()) await consent.click();
  await page.keyboard.press('Escape');
  await page.getByRole('button', { name: 'Connect', exact: true }).click();
  await page.waitForFunction(() => document.querySelector('#status').textContent === 'Connected');
  assert.equal(await page.locator('#play-octave').inputValue(), '3', 'Launch starts on F3');
  assert.equal(await page.locator('#play-keys button[data-hardware="true"]').first().getAttribute('data-note'), '53');
  assert.equal(await page.locator('#play-keys button[data-hardware="true"]').last().getAttribute('data-note'), '79');
  await page.locator('#play-keys button[data-hardware="true"]').first().focus();
  await page.keyboard.down('a');
  assert.equal(await page.locator('#play-keys button[data-note="53"]').getAttribute('aria-pressed'), 'true', 'Typing and visual keyboard both start on F3');
  await page.keyboard.up('a');
  await page.getByRole('button',{name:'Octave up',exact:true}).click();
  assert.equal(await page.locator('#play-keys button[data-hardware="true"]').first().getAttribute('data-note'), '65');
  await page.getByRole('button',{name:'Octave down',exact:true}).click();
  assert.equal(await page.locator('#play-keys button[data-hardware="true"]').first().getAttribute('data-note'), '53');
  await page.locator('#play-octave').selectOption('-1');
  assert.equal(await page.getByRole('button',{name:'Octave down',exact:true}).isEnabled(), false);
  await page.locator('#play-octave').selectOption('7');
  assert.equal(await page.getByRole('button',{name:'Octave up',exact:true}).isEnabled(), false);
  await page.locator('#play-octave').selectOption('3');
  const atk = page.locator('#groups input[aria-label="ATK"]');
  const initial = +(await atk.inputValue());
  await atk.focus(); await atk.press('ArrowUp');
  assert.equal(+(await atk.inputValue()), initial + 1, 'Knob keyboard changes value');
  const box = await atk.boundingBox();
  await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
  await page.mouse.down(); await page.mouse.move(box.x + box.width / 2, box.y - 35, { steps: 6 }); await page.mouse.up();
  assert(+(await atk.inputValue()) > initial + 1, 'Knob vertical drag changes value');
  const edited = await atk.inputValue();
  const angle = await atk.evaluate(e => e.parentElement.style.getPropertyValue('--turn'));
  assert.notEqual(angle, '', 'Dial position is updated');
  const envStyle = page.getByLabel('ENV control style', { exact:true });
  await envStyle.selectOption('sliders');
  assert.equal(await atk.evaluate(e => getComputedStyle(e).opacity), '1', 'Card displays native slider');
  assert.equal(await page.locator('#groups input[aria-label="DEC"]').evaluate(e => e.closest('[data-controls]').dataset.controls), 'sliders');
  assert.equal(await page.locator('#groups input[aria-label="PHS"]').evaluate(e => e.closest('[data-controls]').dataset.controls), 'knobs', 'Other card stays knobs');
  await envStyle.selectOption('knobs');
  const moveEnv = page.getByRole('button', {name:'Move ENV',exact:true});
  await moveEnv.focus(); await moveEnv.press('ArrowRight');
  assert.equal(await page.locator('#groups > .group').nth(1).getAttribute('data-card-key'), 'groups:ENV:ENV');
  const dragFrom = await moveEnv.boundingBox(), dragTo = await page.getByRole('button', {name:'Move LFO',exact:true}).boundingBox();
  await page.mouse.move(dragFrom.x+10,dragFrom.y+10); await page.mouse.down();
  await page.mouse.move(dragTo.x+10,dragTo.y+10,{steps:10});
  assert.equal(await page.locator('.drag-ghost').count(), 1, 'Ghost follows held drag');
  await page.screenshot({path:'build/screenshots/ghost-drag.png'});
  assert.equal(await page.locator('#groups > .group').nth(1).getAttribute('data-card-key'), 'groups:ENV:ENV', 'Cards stay in place until drop');
  await page.mouse.up();
  assert.equal(await page.locator('.drag-ghost').count(), 0, 'Ghost removed on drop');
  const savedOrder = await page.locator('#groups > .group').evaluateAll(nodes=>nodes.map(e=>e.dataset.cardKey));
  assert.equal(savedOrder[2], 'groups:ENV:ENV', 'Drag handle reorders a card');
  await envStyle.selectOption('default');
  await page.getByLabel('LFO control style', {exact:true}).selectOption('sliders');
  for (const skin of ['stage', 'matrix', 'dx', 'modeld', 'chocolate', 'vapor', 'midnight', 'space', 'bauhaus', 'ocean', 'arcade', 'hicon']) {
    await preference('Website theme', skin);
    assert.equal(await page.locator('html').getAttribute('data-skin'), skin);
    await page.waitForFunction(() => {
      const root = document.documentElement;
      if (root.dataset.skin === 'hicon') return true;
      const expected = getComputedStyle(root).getPropertyValue('--wordmark').trim().replace(/["']/g, '');
      return expected && getComputedStyle(document.querySelector('.brand-row h1')).fontFamily.includes(expected);
    });
    await page.evaluate(async () => {
      const title = document.querySelector('.brand-row h1'), style = getComputedStyle(title);
      await document.fonts.load(`${style.fontWeight} ${style.fontSize} ${style.fontFamily}`, title.textContent);
      if (document.documentElement.dataset.skin !== 'hicon') {
        const family = style.fontFamily.split(',')[0].replace(/["']/g, '').trim();
        if (![...document.fonts].some(f => f.family.replace(/["']/g, '') === family && f.status === 'loaded')) throw new Error(`Display font not loaded for ${document.documentElement.dataset.skin}: ${family}`);
      }
    });
    assert.equal(await atk.inputValue(), edited, 'Appearance preserves patch');
    await noOverflow();
    await page.screenshot({ path: `build/screenshots/${skin}.png`, fullPage: true });
    for (const mode of ['light', 'dark']) {
      await preference('Display mode', mode);
      await noOverflow();
      assert.equal(await atk.inputValue(), edited, 'Mode preserves patch');
      await page.screenshot({ path: `build/screenshots/${skin}-${mode}.png`, fullPage: true });
    }
    await page.setViewportSize({width:390,height:844});
    await noOverflow();
    await page.screenshot({path:`build/screenshots/${skin}-title-mobile.png`});
    await page.setViewportSize({width:1440,height:1100});
  }
  await preference('Sound controls', 'sliders');
  await atk.focus(); await atk.press('ArrowDown');
  assert.equal(+(await atk.inputValue()), +edited - 1, 'Slider keyboard changes value');
  await atk.dblclick();
  assert.equal(+(await atk.inputValue()), 10, 'Double-click restores descriptor default (not preset value)');
  await page.reload();
  assert.equal(await page.getByLabel('Website theme', { exact: true }).inputValue(), 'hicon');
  assert.equal(await page.getByLabel('Sound controls', { exact: true }).inputValue(), 'sliders');
  assert.equal(await page.getByLabel('Display mode', { exact: true }).inputValue(), 'dark');
  await page.getByRole('button', { name: 'Connect', exact: true }).click();
  await page.waitForFunction(() => document.querySelector('#status').textContent === 'Connected');
  assert.deepEqual(await page.locator('#groups > .group').evaluateAll(nodes=>nodes.map(e=>e.dataset.cardKey)), savedOrder, 'Card order survives reload');
  assert.equal(await page.getByLabel('LFO control style', {exact:true}).inputValue(), 'sliders', 'Per-card style survives reload');
  await preference('Website theme', 'stage');
  await preference('Sound controls', 'knobs');
  for (const width of [1440, 390]) {
    await page.setViewportSize({ width, height: 950 });
    await page.waitForFunction(w => {
      const n = document.querySelectorAll('#play-keys .extended').length;
      return w === 390 ? n === 0 : n > 0;
    }, width);
    await page.evaluate(() => window.scrollTo(0, 0));
    const keysFit = await page.locator('#play-keys').evaluate(e => {
      const r=e.getBoundingClientRect(); return e.querySelectorAll('[data-hardware=true]').length === 27 && [...e.children].every(k => { const b=k.getBoundingClientRect(); return b.left >= r.left-1 && b.right <= r.right+1; });
    });
    assert(keysFit, 'Hardware and extension keys fit without scrolling');
    const extraCount = await page.locator('#play-keys .extended').count();
    if (width === 1440) {
      assert(extraCount > 0, 'Wide layout adds playable extension keys');
      assert(+(await page.locator('#play-keys button').first().getAttribute('data-note')) < 53);
      assert(+(await page.locator('#play-keys button').last().getAttribute('data-note')) > 79);
    } else assert.equal(extraCount, 0, 'Phone retains the 27 hardware keys');
    const dock = await page.locator('.play-keyboard').boundingBox();
    assert(Math.abs(dock.y + dock.height - 950) <= 1, 'Keyboard anchored to viewport bottom');
    for (const name of ['Sound', '6-OP FM', 'Sequencer', 'Tracks', 'Library', 'Samples', 'Projects', 'Settings']) {
      await page.getByRole('tab', { name, exact: true }).click();
      await noOverflow();
      if (name === 'Tracks') assert.equal(await page.locator('.strip input.fader').count(), 4, 'Mixer retains faders');
      await page.screenshot({ path: `build/screenshots/${name.toLowerCase()}-${width}.png`, fullPage: true });
    }
    await page.evaluate(() => window.scrollTo(0, document.body.scrollHeight));
    const endDock = await page.locator('.play-keyboard').boundingBox();
    assert.equal(endDock.y, dock.y, 'Keyboard stays fixed when page scrolls');
    assert(await page.locator('footer').evaluate(e => e.getBoundingClientRect().bottom <= document.querySelector('.play-keyboard').getBoundingClientRect().top), 'Bottom content can scroll above keyboard');
    await page.screenshot({path:`build/screenshots/docked-${width}.png`});
  }
  await page.setViewportSize({width:1920,height:1080});
  await openMenu(); await page.getByLabel('Full width',{exact:true}).check();
  assert((await page.locator('main').boundingBox()).width > 1800, 'Full width expands chassis');
  await page.reload(); await openMenu();
  assert(await page.getByLabel('Full width',{exact:true}).isChecked(), 'Full width persists');
  await page.getByLabel('Website settings',{exact:true}).press('Escape');
  assert.equal(await page.locator('.settings-menu').getAttribute('open'), null, 'Escape closes menu');
  await page.setViewportSize({width:390,height:950});
  await page.getByRole('button',{name:'Minimize keyboard',exact:true}).click();
  assert((await page.locator('.play-keyboard').boundingBox()).height < 60, 'Keyboard collapses to a small tray');
  assert.equal(await page.locator('#play-keys').isVisible(),false);
  await page.reload();
  assert(await page.getByRole('button',{name:'Show keyboard',exact:true}).isVisible(), 'Collapsed state persists');
  await page.screenshot({path:'build/screenshots/keyboard-tray.png'});
  await page.getByRole('button',{name:'Show keyboard',exact:true}).click();
  assert(await page.locator('#play-keys').isVisible());
  await page.screenshot({path:'build/screenshots/keyboard-restored-mobile.png'});
  await page.goto(`${base}/webapp/installer/`);
  await noOverflow();
  await preference('Website theme', 'chocolate');
  await preference('Display mode', 'light');
  await page.screenshot({ path: 'build/screenshots/installer-mobile.png', fullPage: true });
  await page.setViewportSize({ width: 1440, height: 1100 });
  await noOverflow();
  await page.screenshot({ path: 'build/screenshots/installer-desktop.png', fullPage: true });
  await page.goto(editor);
  assert.equal(await page.getByLabel('Website theme', { exact: true }).inputValue(), 'chocolate', 'Installer shares website preference');
  assert.equal(await page.getByLabel('Display mode', { exact: true }).inputValue(), 'light');
  assert.equal(await page.locator('html').getAttribute('data-contrast'), 'normal');
  assert.deepEqual(errors, [], 'No browser script errors');
  console.log('Website skins: twelve themes, light/dark modes, persistence, shared installer preferences, knob drag/keyboard/reset, sliders, all tabs at desktop/mobile widths passed.');
} catch(error) {
  await page.screenshot({path:'build/screenshots/skin-failure.png'});
  console.error(await page.evaluate(() => ({skin:document.documentElement.dataset.skin,
    titleFont:getComputedStyle(document.querySelector('.brand-row h1')).fontFamily,
    wordmark:getComputedStyle(document.documentElement).getPropertyValue('--wordmark')})));
  throw error;
} finally { await browser.close(); }
