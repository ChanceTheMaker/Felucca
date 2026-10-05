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
  const tag = (...args) => window.dataLayer.push(args);
  const denied = {analytics_storage: 'denied', ad_storage: 'denied', ad_user_data: 'denied', ad_personalization: 'denied'};
  tag('consent', 'default', denied);
  function enable() {
    if (!production || consent !== 'granted') return;
    window['ga-disable-' + id] = false;
    tag('consent', 'update', {...denied, analytics_storage: 'granted'});
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
        if (!production || consent !== 'granted' || !allowed.has(name)) return;
        const safe = {};
        for (const field of ['firmware_version', 'stage', 'error_code', 'file_type']) {
          if (typeof props[field] === 'string' && /^[a-zA-Z0-9_.-]{1,64}$/.test(props[field])) safe[field] = props[field];
        }
        tag('event', name, safe);
      } catch (_) { /* Tracking must never interrupt the product. */ }
    }
  });
  const panel = document.createElement('details');
  panel.className = 'analytics-choice';
  panel.open = !consent;
  const ja = (navigator.language || 'en').toLowerCase().startsWith('ja');
  panel.innerHTML = ja
    ? '<summary>アクセス解析の設定</summary><p>Google Analytics の Cookie を使って訪問数、ダウンロードのクリック、インストール結果を計測してもよろしいですか？ MIDI ノート、音声、プリセット名は送信しません。拒否してもすべての機能を使えます。</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google のプライバシーポリシー</a> <button type="button" data-choice="granted">許可</button> <button type="button" data-choice="denied">拒否</button>'
    : '<summary>Analytics preferences</summary><p>Allow Google Analytics cookies to measure visits, download clicks, and installation outcomes? We do not send MIDI notes, audio, or preset names. All features work if you decline. You can change this choice here anytime.</p><a href="https://policies.google.com/privacy" target="_blank" rel="noopener">Google privacy policy</a> <button type="button" data-choice="granted">Allow analytics</button> <button type="button" data-choice="denied">Decline</button>';
  function choose(value) {
    consent = value;
    try { localStorage.setItem(key, consent); } catch (_) {}
    if (consent === 'granted') enable();
    else { window['ga-disable-' + id] = true; tag('consent', 'update', denied); }
    panel.open = false;
    for (const button of panel.querySelectorAll('button')) button.setAttribute('aria-pressed', String(button.dataset.choice === consent));
  }
  panel.addEventListener('click', event => {
    const choice = event.target.closest('[data-choice]')?.dataset.choice;
    if (choice) choose(choice);
  });
  window.addEventListener('storage', event => {
    if (event.key === key) choose(event.newValue === 'granted' ? 'granted' : 'denied');
  });
  (document.querySelector('main') || document.body).append(panel);
  document.addEventListener('click', event => {
    const link = event.target.closest('a[href]');
    if (!link) return;
    const path = new URL(link.href, location.href).pathname;
    const match = path.match(/\.(zip|fwsc)$/i);
    if (match) window.FeluccaAnalytics.track('download_click', {file_type: match[1].toLowerCase()});
  });
  try { enable(); } catch (_) {}
})();
