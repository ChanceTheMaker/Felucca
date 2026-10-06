// SPDX-License-Identifier: GPL-3.0-only
// Load each product view only as needed; never rotate offscreen or in reduced motion.
(() => {
  const gallery = document.querySelector('[data-accessory-slides]');
  if (!gallery) return;
  const frames = [...gallery.querySelectorAll('img')];
  const sources = ['fm1-case-red.png', 'fm1-case-white-device.png', 'fm1-case-blue.png',
    'fm1-case-glow.png', 'fm1-case-silver-device.png', 'fm1-case-knob-extenders.png'];
  const reduced = matchMedia('(prefers-reduced-motion: reduce)');
  let index = 0, front = 0, visible = false, timer = 0, loading = false;
  const canRotate = () => visible && !document.hidden && !reduced.matches;
  function schedule() {
    clearTimeout(timer);
    if (canRotate() && !loading) timer = setTimeout(advance, 4000);
  }
  async function advance() {
    if (!canRotate() || loading) return;
    loading = true;
    const next = (index + 1) % sources.length, back = 1 - front;
    frames[back].src = new URL(sources[next], new URL('accessories/', document.baseURI)).href;
    try {
      await frames[back].decode();
      if (canRotate()) {
        gallery.closest('.accessory-art').classList.toggle('is-glow', sources[next] === 'fm1-case-glow.png');
        frames[back].classList.add('is-current');
        frames[front].classList.remove('is-current');
        front = back; index = next;
      }
    } catch { /* Keep the last valid illustration if an asset fails to load. */ }
    finally { loading = false; schedule(); }
  }
  if ('IntersectionObserver' in window) {
    new IntersectionObserver(entries => {
      visible = entries[0].isIntersecting; schedule();
    }, {threshold: 0.15}).observe(gallery);
  } else { visible = true; schedule(); }
  reduced.addEventListener('change', schedule);
  document.addEventListener('visibilitychange', schedule);
})();
