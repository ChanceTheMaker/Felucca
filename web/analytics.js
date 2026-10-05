// SPDX-License-Identifier: GPL-3.0-only
// Optional website analytics. Never awaited by audio or firmware installation.
(() => {
  'use strict';
  const id = 'G-JVF09MZEGD', key = 'felucca.web.analyticsConsent';
  const production = location.hostname === 'chancethemaker.github.io' && location.pathname.startsWith('/Felucca/');
  const allowed = new Set(['install_attempt', 'install_write_started', 'install_success', 'install_failed', 'install_resume_complete', 'browser_audio_started', 'download_click']);
  let consent = '', loaded = false;
  try { consent = localStorage.getItem(key) || ''; } catch (_) {}
  if (!['granted', 'denied', 'basic'].includes(consent)) consent = '';
  window.dataLayer = window.dataLayer || [];
  function tag() { window.dataLayer.push(arguments); }
  const denied = {analytics_storage: 'denied', ad_storage: 'denied', ad_user_data: 'denied', ad_personalization: 'denied'};
  tag('consent', 'default', {...denied, analytics_storage: production && (consent === '' || consent === 'granted') ? 'granted' : 'denied'});
  function enable() {
    if (!production || consent === 'denied') return;
    window['ga-disable-' + id] = false;
    tag('consent', 'update', {...denied, analytics_storage: consent === 'basic' ? 'denied' : 'granted'});
    if (loaded) return;
    loaded = true;
    tag('js', new Date());
    tag('config', id, {
      allow_google_signals: false, allow_ad_personalization_signals: false,
      page_location: location.origin + location.pathname,
      page_referrer: (() => { try { return new URL(document.referrer).origin; } catch (_) { return ''; } })()
    });
    const script = document.createElement('script');
    script.async = true;
    script.src = 'https://www.googletagmanager.com/gtag/js?id=' + id;
    document.head.append(script);
  }
  window.FeluccaAnalytics = Object.freeze({
    track(name, props = {}) {
      try {
        if (!production || consent === 'denied' || !allowed.has(name)) return;
        const safe = {};
        for (const field of ['firmware_version', 'stage', 'error_code', 'file_type']) {
          if (typeof props[field] === 'string' && /^[a-zA-Z0-9_.-]{1,64}$/.test(props[field])) safe[field] = props[field];
        }
        tag('event', name, safe);
      } catch (_) { /* Tracking must never interrupt the product. */ }
    }
  });
  const panel = document.createElement('dialog');
  panel.id = 'analytics-preferences';
  panel.className = 'analytics-choice';
  panel.setAttribute('aria-labelledby', 'analytics-title');
  const ja = (navigator.language || 'en').toLowerCase().startsWith('ja');
  panel.innerHTML = ja
    ? '<div class="analytics-heading"><h2 id="analytics-title">Cookie の設定</h2><button type="button" data-close aria-label="閉じる" autofocus>×</button></div><p>サイトの機能と設定の保存にはブラウザーのストレージを使用します。任意のアクセス解析 Cookie を許可すると、訪問数、ダウンロードのクリック、インストール結果を Google Analytics に送信します。MIDI ノート、音声、プリセット名は送信しません。許可しなくてもすべての機能を使えます。このメニューからいつでも設定を変更できます。</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google のプライバシーポリシー</a><div class="analytics-actions"><button type="button" data-choice="granted">アクセス解析を許可</button><button type="button" data-choice="denied">必須のみ</button></div>'
    : '<div class="analytics-heading"><h2 id="analytics-title">Cookie settings</h2><button type="button" data-close aria-label="Close cookie settings" autofocus>×</button></div><p>We use browser storage for site features and to remember your preferences. Optional Google Analytics cookies are on by default to measure visits, download clicks, and installation results. Choose Essential only to turn analytics off. We do not send MIDI notes, audio, or preset names. Every feature works with essential storage only. You can change your choice here anytime.</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google privacy policy</a><div class="analytics-actions"><button type="button" data-choice="granted">Keep analytics on</button><button type="button" data-choice="denied">Essential only</button></div>';
  const banner = document.createElement('section');
  banner.className = 'analytics-banner';
  banner.setAttribute('aria-labelledby', 'cookie-banner-title');
  banner.innerHTML = ja
    ? '<div><h2 id="cookie-banner-title">Cookie の設定</h2><p>サイトの機能と設定の保存にはブラウザーのストレージを使用します。任意の Google Analytics Cookie を許可すると、サイトの利用状況を把握できます。</p></div><div class="analytics-actions"><button type="button" data-choice="granted">アクセス解析を許可</button><button type="button" data-choice="denied">必須のみ</button><button type="button" data-settings aria-haspopup="dialog" aria-controls="analytics-preferences">Cookie の設定</button></div>'
    : '<div><h2 id="cookie-banner-title">Your cookie choices</h2><p>We use browser storage for site features and your preferences. Optional Google Analytics cookies are on by default to help us understand how the site is used. Choose Essential only to turn analytics off.</p></div><div class="analytics-actions"><button type="button" data-choice="granted">Keep analytics on</button><button type="button" data-choice="denied">Essential only</button><button type="button" data-settings aria-haspopup="dialog" aria-controls="analytics-preferences">Cookie settings</button></div>';
  banner.hidden = ['granted', 'denied'].includes(consent);
  function reflectChoice() {
    banner.hidden = ['granted', 'denied'].includes(consent);
    for (const container of [panel, banner]) {
      for (const button of container.querySelectorAll('[data-choice]')) button.setAttribute('aria-pressed', String(button.dataset.choice === (consent || 'granted')));
    }
  }
  function choose(value, persist = true) {
    consent = value;
    if (persist) { try { localStorage.setItem(key, consent); } catch (_) {} }
    if (consent !== 'denied') enable();
    else { window['ga-disable-' + id] = true; tag('consent', 'update', denied); }
    if (panel.open) panel.close();
    reflectChoice();
  }
  panel.addEventListener('click', event => {
    const choice = event.target.closest('[data-choice]')?.dataset.choice;
    if (choice) choose(choice);
    if (event.target.closest('[data-close]')) panel.close();
    if (event.target === panel) {
      const rect = panel.getBoundingClientRect();
      if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) panel.close();
    }
  });
  banner.addEventListener('click', event => {
    const choice = event.target.closest('[data-choice]')?.dataset.choice;
    if (choice) choose(choice);
    if (event.target.closest('[data-settings]')) { reflectChoice(); panel.showModal(); }
  });
  window.addEventListener('storage', event => {
    if (event.key === key || event.key === null) choose(['granted', 'denied', 'basic'].includes(event.newValue) ? event.newValue : '', false);
  });
  function mountPreferences() {
    reflectChoice();
    document.body.append(panel, banner);
    const host = document.querySelector('.settings-menu [data-appearance]');
    if (!host) return;
    const open = document.createElement('button');
    open.type = 'button';
    open.className = 'analytics-menu-button';
    open.textContent = ja ? 'Cookie の設定' : 'Cookie settings';
    open.setAttribute('aria-haspopup', 'dialog');
    open.setAttribute('aria-controls', panel.id);
    open.addEventListener('click', () => {
      host.closest('.settings-menu').open = false;
      reflectChoice();
      panel.showModal();
    });
    panel.addEventListener('close', () => host.closest('.settings-menu').querySelector('summary').focus());
    host.append(open);
  }
  if (document.readyState === 'complete') mountPreferences();
  else document.addEventListener('DOMContentLoaded', mountPreferences, {once:true});
  document.addEventListener('click', event => {
    const link = event.target.closest('a[href]');
    if (!link) return;
    const path = new URL(link.href, location.href).pathname;
    const match = path.match(/\.(zip|fwsc)$/i);
    if (match) window.FeluccaAnalytics.track('download_click', {file_type: match[1].toLowerCase()});
  });
  try { enable(); } catch (_) {}
})();
