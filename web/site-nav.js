// SPDX-License-Identifier: GPL-3.0-only
// Shared destinations; navigation never initiates an installation or audio playback.
(() => {
  const nav = document.querySelector('.site-nav');
  if (!nav) return;
  const studio = document.body.classList.contains('editor-page');
  const update = () => {
    const section = studio ? 'studio' : ({'#fm1-guide':'manual','#themes':'themes','#features':'features'})[location.hash] || 'install';
    for (const link of nav.querySelectorAll('[data-nav]')) {
      if (link.dataset.nav === section) link.setAttribute('aria-current', section === 'install' || studio ? 'page' : 'location');
      else link.removeAttribute('aria-current');
    }
  };
  window.addEventListener('hashchange', update);
  update();
})();
