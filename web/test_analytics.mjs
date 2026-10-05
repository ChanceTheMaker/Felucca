// SPDX-License-Identifier: GPL-3.0-only
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import vm from 'node:vm';
const source = readFileSync(new URL('./analytics.js', import.meta.url), 'utf8');
function setup(host = 'chancethemaker.github.io', saved = null, blocked = false) {
  const scripts = [], handlers = {}, listeners = {}, storage = new Map();
  if (saved) storage.set('felucca.web.analyticsConsent', saved);
  const panel = {querySelectorAll: () => [], addEventListener: (k, fn) => {handlers[k] = fn;}};
  const context = {
    navigator: {language: 'en'},
    location: {hostname: host, pathname: '/Felucca/webapp/installer/', origin: 'https://' + host, href: 'https://' + host + '/Felucca/webapp/installer/?secret=private'},
    localStorage: {getItem: k => {if (blocked) throw Error('blocked'); return storage.get(k);}, setItem: (k, v) => {if (blocked) throw Error('blocked'); storage.set(k, v);}},
    document: {referrer: 'https://example.org/private?q=secret', documentElement: {lang: 'en'},
      createElement: name => name === 'details' ? panel : {}, head: {append: s => scripts.push(s)},
      querySelector: () => ({append() {}}), addEventListener: (k, fn) => {listeners[k] = fn;}},
    URL, Date, Set, addEventListener() {}
  };
  context.window = context;
  vm.runInNewContext(source, context);
  return {context, scripts, listeners, choose: choice => handlers.click({target: {closest: () => ({dataset: {choice}})}})};
}
const fresh = setup();
assert.equal(fresh.scripts.length, 0, 'No Google script before opt-in');
fresh.context.FeluccaAnalytics.track('install_attempt');
assert.equal(fresh.context.dataLayer.filter(x => x[0] === 'event').length, 0);
fresh.choose('granted');
assert.equal(fresh.scripts.length, 1);
const config = fresh.context.dataLayer.find(x => x[0] === 'config')[2];
assert.ok(!config.page_location.includes('?'));
assert.equal(config.page_referrer, 'https://example.org');
fresh.context.FeluccaAnalytics.track('install_failed', {stage: 'write', error_code: 'lost', midi: 'private'});
assert.equal(fresh.context.dataLayer.at(-1)[2].midi, undefined);
fresh.context.FeluccaAnalytics.track('unknown_event');
assert.equal(fresh.context.dataLayer.at(-1)[1], 'install_failed');
fresh.choose('denied');
const count = fresh.context.dataLayer.length;
fresh.context.FeluccaAnalytics.track('install_attempt');
assert.equal(fresh.context.dataLayer.length, count);
assert.equal(fresh.context['ga-disable-G-JVF09MZEGD'], true);
fresh.choose('granted');
assert.equal(fresh.scripts.length, 1, 'No duplicate tag');
assert.equal(setup('localhost', 'granted').scripts.length, 0);
assert.equal(setup('hugelton.github.io', 'granted').scripts.length, 0);
assert.equal(setup('chancethemaker.github.io', 'denied').scripts.length, 0);
assert.equal(setup('chancethemaker.github.io', 'granted').scripts.length, 1);
assert.doesNotThrow(() => setup('chancethemaker.github.io', null, true).choose('granted'));
console.log('Analytics consent, host isolation, event filtering, and storage-failure checks passed.');
