// SPDX-License-Identifier: GPL-3.0-only
// Optional website analytics. Never awaited by audio or firmware installation.
(() => {
  'use strict';
  const id = 'G-JVF09MZEGD', key = 'felucca.web.analyticsConsent';
  const production = location.hostname === 'chancethemaker.github.io' && location.pathname.startsWith('/Felucca/');
  const allowed = new Set(['install_attempt', 'install_write_started', 'install_success', 'install_failed', 'install_resume_complete', 'browser_audio_started', 'download_click']);
  let consent = '', loaded = false;
  try { consent = localStorage.getItem(key) || ''; } catch (_) {}
  window.dataLayer = window.dataLayer || [];
  function tag() { window.dataLayer.push(arguments); }
  const denied = {analytics_storage: 'denied', ad_storage: 'denied', ad_user_data: 'denied', ad_personalization: 'denied'};
  tag('consent', 'default', denied);
  function enable() {
    if (!production || consent === 'denied') return;
    window['ga-disable-' + id] = false;
    tag('consent', 'update', {...denied, analytics_storage: consent === 'granted' ? 'granted' : 'denied'});
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
    ? '<div class="analytics-heading"><h2 id="analytics-title">アクセス解析の設定</h2><button type="button" data-close aria-label="閉じる" autofocus>×</button></div><p>標準では Cookie を使わずに訪問数、ダウンロードのクリック、インストール結果を Google Analytics に送信します。アクセス解析用 Cookie を許可することも、解析を停止することもできます。MIDI ノート、音声、プリセット名は送信しません。どの設定でもすべての機能を使えます。</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google のプライバシーポリシー</a><div class="analytics-actions"><button type="button" data-choice="basic">Cookie なし</button><button type="button" data-choice="granted">Cookie を許可</button><button type="button" data-choice="denied">解析を停止</button></div>'
    : '<div class="analytics-heading"><h2 id="analytics-title">Analytics preferences</h2><button type="button" data-close aria-label="Close analytics preferences" autofocus>×</button></div><p>By default, we send cookieless visit, download-click, and installation measurements to Google Analytics. You can allow analytics cookies for fuller measurement or turn analytics off. We do not send MIDI notes, audio, or preset names. Every feature works with any choice.</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google privacy policy</a><div class="analytics-actions"><button type="button" data-choice="basic">Without cookies</button><button type="button" data-choice="granted">Allow analytics cookies</button><button type="button" data-choice="denied">Turn analytics off</button></div>';
  function reflectChoice() {
    for (const button of panel.querySelectorAll('[data-choice]')) button.setAttribute('aria-pressed', String(button.dataset.choice === (consent || 'basic')));
  }
  function choose(value) {
    consent = value;
    try { localStorage.setItem(key, consent); } catch (_) {}
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
  window.addEventListener('storage', event => {
    if (event.key === key) choose(['granted', 'denied'].includes(event.newValue) ? event.newValue : 'basic');
  });
  function mountPreferences() {
    document.body.append(panel);
    const host = document.querySelector('.settings-menu [data-appearance]');
    if (!host) return;
    const open = document.createElement('button');
    open.type = 'button';
    open.className = 'analytics-menu-button';
    open.textContent = ja ? 'アクセス解析の設定' : 'Analytics preferences';
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
