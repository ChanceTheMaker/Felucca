// SPDX-License-Identifier: GPL-3.0-only
// Shared destinations; navigation never initiates an installation or audio playback.
(() => {
  if (location.pathname === '/editor.html') {
    for (const a of document.querySelectorAll('a[href^="../installer/"]')) {
      const hash = a.getAttribute('href').split('#')[1];
      a.setAttribute('href', '/'+(hash ? '#'+(hash === 'fm1-guide' ? 'guide' : hash === 'themes' ? 'features' : hash) : ''));
    }
  }
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
  const topButton = document.getElementById('back-to-top');
  if (!studio && topButton) {
    const showTop = () => { topButton.hidden = window.scrollY < 300; };
    window.addEventListener('scroll', showTop, {passive:true});
    window.addEventListener('pageshow', showTop);
    topButton.addEventListener('click', () => {
      history.replaceState(null, '', location.pathname + location.search);
      update();
      document.getElementById('page-top')?.focus({preventScroll:true});
      window.scrollTo({top:0, behavior:matchMedia('(prefers-reduced-motion: reduce)').matches ? 'instant' : 'smooth'});
    });
    showTop();
  }
})();
