// SPDX-License-Identifier: GPL-3.0-only
// Website appearance only: never writes device preferences or MIDI.
(() => {
  const root = document.documentElement;
  const themes = [['stage', 'Stage Red'], ['matrix', 'Matrix'], ['dx', 'Vintage DX7'], ['modeld', 'Model D Walnut'],
    ['chocolate', 'Chocolate Factory'], ['vapor', 'Vaporwave'], ['midnight', 'Midnight Studio'],
    ['space', 'Space Mission'], ['bauhaus', 'Bauhaus'], ['ocean', 'Ocean Lab'], ['arcade', 'Arcade \u201984'], ['hicon', 'High Contrast']];
  const read = (key, fallback) => { try { return localStorage.getItem(key) || fallback; } catch { return fallback; } };
  const save = (key, value) => { try { localStorage.setItem(key, value); } catch { /* private browsing */ } };
  root.dataset.skin = themes.some(([id]) => id === read('felucca.web.skin')) ? read('felucca.web.skin') : 'stage';
  root.dataset.defaultControls = read('felucca.web.controls') === 'sliders' ? 'sliders' : 'knobs';
  root.dataset.mode = read('felucca.web.mode') === 'light' ? 'light' : 'dark';
  root.dataset.contrast = root.dataset.skin === 'hicon' ? 'high' : 'normal';
  root.dataset.fullWidth = read('felucca.web.fullWidth') === 'true' ? 'true' : 'false';
  function sync(input) {
    const min = +input.min, max = +input.max;
    const fraction = max > min ? (+input.value - min) / (max - min) : 0;
    input.parentElement?.style.setProperty('--turn', `${-135 + fraction * 270}deg`);
  }
  function enhance(input) {
    if (input.type !== 'range') return input;
    const shell = document.createElement('span');
    shell.className = 'dial-control';
    const face = document.createElement('span');
    face.className = 'dial-face'; face.setAttribute('aria-hidden', 'true');
    shell.append(face, input);
    let drag = null;
    input.addEventListener('pointerdown', e => {
      if (input.closest('[data-controls]')?.dataset.controls !== 'knobs' || input.disabled || e.button !== 0) return;
      e.preventDefault(); input.focus(); input.setPointerCapture(e.pointerId);
      drag = { id: e.pointerId, y: e.clientY, value: +input.value };
    });
    input.addEventListener('pointermove', e => {
      if (!drag || drag.id !== e.pointerId) return;
      const step = +input.step || 1, min = +input.min, max = +input.max;
      const value = drag.value + (drag.y - e.clientY) * (max - min) / (e.shiftKey ? 1200 : 160);
      input.value = String(Math.max(min, Math.min(max, min + Math.round((value - min) / step) * step)));
      input.dispatchEvent(new Event('input', { bubbles: true })); sync(input);
    });
    const finish = e => {
      if (!drag || e.pointerId !== drag.id) return;
      drag = null;
      input.dispatchEvent(new Event('change', { bubbles: true }));
    };
    input.addEventListener('pointerup', finish);
    input.addEventListener('pointercancel', finish);
    input.addEventListener('lostpointercapture', finish);
    input.addEventListener('input', () => sync(input));
    input.addEventListener('keydown', () => requestAnimationFrame(() => sync(input)));
    input.addEventListener('dblclick', () => sync(input));
    return shell;
  }
  const readJSON = (key, fallback) => { try { return JSON.parse(read(key)) ?? fallback; } catch { return fallback; } };
  const storedCards = readJSON('felucca.web.cards', {});
  const cards = storedCards && typeof storedCards === 'object' && !Array.isArray(storedCards) ? storedCards : {};
  function applyCard(card) {
    const preference = cards[card.dataset.cardKey];
    card.dataset.controls = ['knobs', 'sliders'].includes(preference) ? preference : root.dataset.defaultControls;
  }
  function decorate(container, layoutKey) {
    const sortable = container.id === 'groups';
    const orderKey = `felucca.web.order.${layoutKey}`;
    const saveOrder = () => save(orderKey, JSON.stringify([...container.children].map(c => c.dataset.cardKey)));
    let announce = document.getElementById('card-announcement');
    if (!announce) {
      announce = document.createElement('span'); announce.id = 'card-announcement'; announce.className = 'sr-only';
      announce.setAttribute('aria-live', 'polite'); document.body.append(announce);
    }
    for (const [index, card] of [...container.children].entries()) {
      card.dataset.cardKey ||= `${container.id}:fallback:${index}`;
      applyCard(card);
      const name = card.querySelector('h2')?.textContent.replace(/[\uEA00-\uEB09]/g, '').trim() || 'Section';
      const toolbar = document.createElement('div'); toolbar.className = 'card-tools';
      const select = document.createElement('select'); select.className = 'card-style';
      select.setAttribute('aria-label', `${name} control style`);
      for (const [value, label] of [['default', 'Default'], ['knobs', 'Knobs'], ['sliders', 'Sliders']]) select.add(new Option(label, value));
      select.value = ['knobs', 'sliders'].includes(cards[card.dataset.cardKey]) ? cards[card.dataset.cardKey] : 'default';
      select.addEventListener('change', () => {
        cards[card.dataset.cardKey] = select.value; save('felucca.web.cards', JSON.stringify(cards)); applyCard(card);
      });
      if (sortable) {
        const handle = document.createElement('button'); handle.type = 'button'; handle.className = 'card-grip';
        handle.textContent = '\u283f'; handle.setAttribute('aria-label', `Move ${name}`);
        handle.title = 'Drag to rearrange. Arrow keys move one position; Home/End move first/last.';
        let drag = null;
        const finish = (commit = false) => {
          if (!drag) return;
          const state = drag; drag = null;
          cancelAnimationFrame(state.frame);
          state.ghost?.remove(); card.classList.remove('reordering');
          container.querySelectorAll('.drop-target').forEach(c => c.classList.remove('drop-target'));
          if (commit && state.active && state.target && state.target !== card && state.target.isConnected) {
            const list = [...container.children];
            container.insertBefore(card, list.indexOf(card) < list.indexOf(state.target) ? state.target.nextSibling : state.target);
            saveOrder(); announce.textContent = `${name} moved to position ${[...container.children].indexOf(card) + 1}.`;
          }
          if (card.hasPointerCapture(state.id)) card.releasePointerCapture(state.id);
          document.removeEventListener('keydown', escape);
          window.removeEventListener('blur', cancel);
        };
        const cancel = () => finish(false);
        const escape = e => { if (e.key === 'Escape') { e.preventDefault(); cancel(); } };
        card.addEventListener('pointerdown', e => {
          if (drag || e.button !== 0 || (e.target.closest('input,select,a,.dial-control,button') && !e.target.closest('.card-grip'))) return;
          e.preventDefault(); handle.focus(); card.setPointerCapture(e.pointerId);
          const box = card.getBoundingClientRect();
          drag = { id:e.pointerId, startX:e.clientX, startY:e.clientY, x:e.clientX, y:e.clientY,
            offsetX:e.clientX-box.x, offsetY:e.clientY-box.y, box, active:false, target:null };
          document.addEventListener('keydown', escape); window.addEventListener('blur', cancel);
        });
        const tick = () => {
          if (!drag?.active) return;
          if (!card.isConnected) { cancel(); return; }
          drag.ghost.style.transform = `translate(${drag.x-drag.offsetX}px,${drag.y-drag.offsetY}px)`;
          const dockTop = document.querySelector('.play-keyboard')?.getBoundingClientRect().top || innerHeight;
          if (drag.y < 65) window.scrollBy(0,-12);
          else if (drag.y > dockTop-35 && drag.y < dockTop) window.scrollBy(0,12);
          const target = document.elementFromPoint(drag.x,drag.y)?.closest('#groups > .group');
          drag.target = target && target !== card ? target : null;
          container.querySelectorAll('.drop-target').forEach(c => c.classList.remove('drop-target'));
          drag.target?.classList.add('drop-target');
          drag.frame = requestAnimationFrame(tick);
        };
        card.addEventListener('pointermove', e => {
          if (!drag || drag.id !== e.pointerId) return;
          drag.x = e.clientX; drag.y = e.clientY;
          if (drag.active || Math.hypot(e.clientX-drag.startX,e.clientY-drag.startY) < 6) return;
          drag.active = true;
          const layer = document.createElement('div'); layer.className = 'groups drag-ghost';
          layer.setAttribute('aria-hidden','true'); layer.inert = true;
          layer.style.width = `${drag.box.width}px`;
          const clone = card.cloneNode(true); clone.style.height = `${drag.box.height}px`;
          const originals = card.querySelectorAll('input,select');
          clone.querySelectorAll('input,select').forEach((e,i) => { e.value = originals[i].value; });
          clone.querySelectorAll('[id]').forEach(e => e.removeAttribute('id'));
          layer.append(clone); document.body.append(layer); drag.ghost = layer;
          card.classList.add('reordering'); tick();
        });
        card.addEventListener('pointerup', e => {
          if (drag?.id !== e.pointerId) return;
          drag.target = document.elementFromPoint(e.clientX,e.clientY)?.closest('#groups > .group');
          finish(true);
        });
        card.addEventListener('pointercancel', e => { if (drag?.id === e.pointerId) cancel(); });
        card.addEventListener('lostpointercapture', e => { if (drag?.id === e.pointerId) cancel(); });
        handle.addEventListener('keydown', e => {
          const list = [...container.children], from = list.indexOf(card);
          const to = { ArrowLeft:from-1, ArrowUp:from-1, ArrowRight:from+1, ArrowDown:from+1, Home:0, End:list.length-1 }[e.key];
          if (to == null) return;
          e.preventDefault();
          if (to < 0 || to >= list.length || to === from) return;
          container.insertBefore(card, to > from ? list[to].nextSibling : list[to]); handle.focus(); saveOrder();
          announce.textContent = `${name} moved to position ${to + 1}.`;
        });
        toolbar.append(handle);
      }
      toolbar.append(select); card.prepend(toolbar);
    }
    if (sortable) {
      const stored = readJSON(orderKey, []), order = Array.isArray(stored) ? stored : [];
      const ranks = new Map(order.map((key, i) => [key, i]));
      container.append(...[...container.children].sort((a,b) => (ranks.get(a.dataset.cardKey) ?? 999) - (ranks.get(b.dataset.cardKey) ?? 999)));
    }
  }
  function setLanguageFlag(button, language) {
    const english = language === 'ja';
    button.setAttribute('aria-label', english ? 'Switch to English' : '日本語に切り替え');
    button.title = english ? 'English' : '日本語';
    let flag = '<rect width="60" height="40" fill="#fff"/><circle cx="30" cy="20" r="12" fill="#bc002d"/>';
    if (english) {
      flag = '<rect width="60" height="40" fill="#fff"/>';
      for (let row = 0; row < 7; row++) flag += `<rect y="${row * 80 / 13}" width="60" height="${40 / 13}" fill="#b22234"/>`;
      flag += '<rect width="26" height="21.54" fill="#3c3b6e"/>';
      for (let row = 0; row < 9; row++) for (let col = 0; col < (row % 2 ? 5 : 6); col++) {
        flag += `<circle cx="${2.2 + col * 4.3 + (row % 2 ? 2.15 : 0)}" cy="${1.5 + row * 2.3}" r=".7" fill="#fff"/>`;
      }
    }
    button.innerHTML = `<svg viewBox="0 0 60 40" aria-hidden="true" focusable="false">${flag}</svg>`;
  }
  window.FeluccaSkin = { enhance, sync, decorate, setLanguageFlag };
  document.addEventListener('DOMContentLoaded', () => {
    const host = document.querySelector('[data-appearance]');
    if (!host) return;
    host.innerHTML = `<label><span>Website theme</span><select id="web-skin" aria-label="Website theme">${themes.map(([id, name]) => `<option value="${id}">${name}</option>`).join('')}</select></label>` +
      (document.body.classList.contains('editor-page') ? '<label><span>Sound controls</span><select id="web-controls" aria-label="Sound controls"><option value="knobs">Knobs</option><option value="sliders">Sliders</option></select></label>' : '') +
      '<label><span>Display mode</span><select id="web-mode" aria-label="Display mode"><option value="dark">Dark</option><option value="light">Light</option></select></label>' +
      '<label class="width-toggle"><span>Full width</span><input id="web-width" type="checkbox" aria-label="Full width"></label>';
    const menu = document.createElement('details'); menu.className = 'settings-menu';
    const trigger = document.createElement('summary'); trigger.textContent = '☰'; trigger.setAttribute('aria-label','Website settings');
    menu.append(trigger); document.querySelector('.brand-links').append(menu); menu.append(host);
    document.addEventListener('pointerdown', e => { if (!menu.contains(e.target)) menu.open = false; });
    menu.addEventListener('keydown', e => { if (e.key === 'Escape') { menu.open = false; trigger.focus(); } });
    const width = document.getElementById('web-width'); width.checked = root.dataset.fullWidth === 'true';
    width.addEventListener('change', () => { root.dataset.fullWidth = String(width.checked); save('felucca.web.fullWidth', String(width.checked)); });
    const skin = document.getElementById('web-skin'); skin.value = root.dataset.skin;
    skin.addEventListener('change', () => { root.dataset.skin = skin.value; root.dataset.contrast = skin.value === 'hicon' ? 'high' : 'normal'; save('felucca.web.skin', skin.value); });
    const mode = document.getElementById('web-mode'); mode.value = root.dataset.mode;
    mode.addEventListener('change', () => { root.dataset.mode = mode.value; save('felucca.web.mode', mode.value); });
    const controls = document.getElementById('web-controls');
    if (controls) {
      controls.value = root.dataset.defaultControls;
      controls.addEventListener('change', () => { root.dataset.defaultControls = controls.value; save('felucca.web.controls', controls.value); document.querySelectorAll('[data-card-key]').forEach(applyCard); });
    }
    const keyboard = document.querySelector('.play-keyboard');
    if (keyboard) {
      const tray = document.createElement('button'); tray.type = 'button'; tray.className = 'keyboard-tray';
      tray.setAttribute('aria-controls', 'play-keys');
      const setCollapsed = collapsed => {
        // Release held notes/sustain before hiding the performance controls.
        if (collapsed) document.getElementById('play-stop')?.click();
        keyboard.classList.toggle('collapsed', collapsed);
        tray.textContent = collapsed ? '▴ Show keyboard' : '▾';
        tray.setAttribute('aria-label', collapsed ? 'Show keyboard' : 'Minimize keyboard');
        tray.title = collapsed ? 'Show keyboard' : 'Minimize keyboard';
        tray.setAttribute('aria-expanded', String(!collapsed));
        save('felucca.web.keyboardCollapsed', String(collapsed));
      };
      tray.addEventListener('click', () => setCollapsed(!keyboard.classList.contains('collapsed')));
      keyboard.querySelector('.play-controls').append(tray); setCollapsed(read('felucca.web.keyboardCollapsed') === 'true');
      const measure = () => root.style.setProperty('--keyboard-height', `${Math.ceil(keyboard.getBoundingClientRect().height)}px`);
      new ResizeObserver(measure).observe(keyboard); measure();
    }
  });
})();
