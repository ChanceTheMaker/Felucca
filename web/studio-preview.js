// SPDX-License-Identifier: GPL-3.0-only
// Theme-matched, non-interactive preview for Studio links on the installer.
(() => {
  const names = {stage:'Stage Red', matrix:'Matrix', dx:'Vintage DX7', modeld:'Model D Walnut',
    chocolate:'Chocolate Factory', vapor:'Vaporwave', midnight:'Midnight Studio', space:'Space Mission',
    bauhaus:'Bauhaus', ocean:'Ocean Lab', arcade:'Arcade ’84', hicon:'High Contrast'};
  const bubble = document.createElement('div');
  bubble.id = 'studio-preview';
  bubble.className = 'studio-preview';
  bubble.setAttribute('role', 'tooltip');
  bubble.hidden = true;
  const picture = document.createElement('img');
  picture.width = 720; picture.height = 550;
  const caption = document.createElement('div');
  bubble.append(picture, caption);
  document.body.append(bubble);
  let anchor = null, timer, previousDescription = null;
  function position() {
    if (!anchor) return;
    const rect = anchor.getBoundingClientRect(), box = bubble.getBoundingClientRect();
    const left = Math.max(12, Math.min(rect.left + rect.width / 2 - box.width / 2, innerWidth - box.width - 12));
    const below = rect.bottom + box.height + 12 <= innerHeight || rect.top < box.height + 12;
    const top = below ? rect.bottom + 10 : rect.top - box.height - 10;
    bubble.style.left = left + 'px';
    bubble.style.top = Math.max(8, Math.min(top, innerHeight - box.height - 8)) + 'px';
    bubble.dataset.side = below ? 'below' : 'above';
    bubble.style.setProperty('--preview-arrow', Math.max(20, Math.min(rect.left + rect.width / 2 - left, box.width - 20)) + 'px');
  }
  function refresh() {
    if (!anchor) return;
    const root = document.documentElement;
    const skin = Object.hasOwn(names, root.dataset.skin) ? root.dataset.skin : 'stage';
    const mode = root.dataset.mode === 'light' ? 'light' : 'dark';
    picture.src = `screenshots/previews/${skin}-${mode}.jpg`;
    picture.alt = `Felucca [Salt] Studio in ${names[skin]}, ${mode} mode`;
    caption.textContent = `${names[skin]} · ${mode === 'light' ? 'Light' : 'Dark'} · Studio preview`;
    position();
  }
  function hide() {
    clearTimeout(timer);
    if (anchor) {
      if (previousDescription === null) anchor.removeAttribute('aria-describedby');
      else anchor.setAttribute('aria-describedby', previousDescription);
    }
    anchor = null; bubble.hidden = true;
  }
  function show(link) {
    clearTimeout(timer);
    if (anchor !== link) {
      hide(); anchor = link;
      previousDescription = link.getAttribute('aria-describedby');
      link.setAttribute('aria-describedby', [previousDescription, bubble.id].filter(Boolean).join(' '));
    }
    bubble.hidden = false; refresh();
  }
  const later = () => { clearTimeout(timer); timer = setTimeout(hide, 160); };
  for (const link of document.querySelectorAll('a[href="../editor/"]')) {
    link.addEventListener('pointerenter', e => { if (e.pointerType !== 'touch') show(link); });
    link.addEventListener('pointerleave', later);
    link.addEventListener('focus', () => show(link));
    link.addEventListener('blur', later);
    link.addEventListener('click', hide);
  }
  bubble.addEventListener('pointerenter', () => clearTimeout(timer));
  bubble.addEventListener('pointerleave', later);
  picture.addEventListener('load', position);
  document.addEventListener('keydown', e => { if (e.key === 'Escape') hide(); });
  window.addEventListener('resize', position);
  window.addEventListener('scroll', () => {
    if (!anchor) return;
    const rect = anchor.getBoundingClientRect();
    if (rect.bottom < 0 || rect.top > innerHeight) hide();
    else position();
  }, {passive:true});
  new MutationObserver(refresh).observe(document.documentElement, {attributes:true, attributeFilter:['data-skin', 'data-mode']});
})();
