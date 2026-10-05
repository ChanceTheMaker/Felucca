// SPDX-License-Identifier: GPL-3.0-only
// Decorative rack hardware occupies existing blank space; never sound controls.
(() => {
  const states=new WeakMap();
  const screw=(x,y)=>`<circle cx="${x}" cy="${y}" r="3" class="hardware-screw"/><path d="M${x-1.5} ${y+1.5}l3-3" class="hardware-slot"/>`;
  const jack=(x,y)=>`<circle cx="${x}" cy="${y}" r="10" class="hardware-jack-rim"/><circle cx="${x}" cy="${y}" r="6.5" class="hardware-jack-ring"/><circle cx="${x}" cy="${y}" r="3.5" class="hardware-jack-hole"/>`;
  function plate(fraction,kind='patch') {
    const item=document.createElement('span');
    item.className=`hardware-plate hardware-${kind}`;item.dataset.fraction=fraction;
    item.style.setProperty('--hardware-span',fraction==='1/2'?4:fraction==='1/4'?2:1);
    const width=fraction==='1/2'?160:fraction==='1/4'?80:40;
    let art;
    if(kind==='speaker') art='<circle cx="80" cy="84" r="70" class="hardware-cone"/><circle cx="80" cy="84" r="57" class="hardware-cone-ring"/><circle cx="80" cy="84" r="44" class="hardware-cone-ring"/><circle cx="80" cy="84" r="22" class="hardware-dustcap"/>';
    else art=(fraction==='1/2'?[32,80,128]:fraction==='1/4'?[24,56]:[20]).map(x=>jack(x,31)+jack(x,61)).join('');
    item.innerHTML=`<svg viewBox="0 0 ${width} 84" aria-hidden="true" focusable="false">${art}${screw(7,7)}${screw(width-7,7)}${screw(7,77)}${screw(width-7,77)}</svg>`;
    return item;
  }
  function filler(className) {
    const div=document.createElement('div');div.className=className;
    div.setAttribute('aria-hidden','true');div.inert=true;return div;
  }
  function decorate(container) {
    const previous=states.get(container);
    if(previous){previous.observer.disconnect();cancelAnimationFrame(previous.frame);}
    container.querySelectorAll('.bank-hardware,.bank-hardware-gap').forEach(e=>e.remove());
    const sections=[];
    for(const bank of container.querySelectorAll('.unified-card .bank-rows')) {
      const count=bank.querySelectorAll(':scope > .row').length, spare=(4-count%4)%4;
      if(!spare)continue;
      const extra=filler('bank-hardware');extra.style.setProperty('--spare-columns',spare);
      // These use only the unused cells in the existing four-control grid row.
      if(spare>=2)extra.append(plate('1/2',count===1?'speaker':'patch'));
      if(spare%2) {
        if(count===3)extra.append(plate('1/8'),plate('1/8'));
        else extra.append(plate('1/4'));
      }
      bank.append(extra);
    }
    for(const section of container.querySelectorAll('.unified-card .bank-section')) {
      const last=section.lastElementChild;
      if(!last)continue;
      const extra=filler('bank-hardware-gap');extra.hidden=true;
      extra.append(plate('1/2','speaker'),plate('1/4'),plate('1/8'),plate('1/8'));
      section.append(extra);sections.push({section,last,extra});
    }
    const state={frame:0};
    const measure=()=>{
      cancelAnimationFrame(state.frame);
      state.frame=requestAnimationFrame(()=>{
        const sizes=sections.map(({section,last,extra})=>{
          const box=section.getBoundingClientRect(),end=last.getBoundingClientRect();
          const free=box.bottom-end.bottom-10;
          return {extra,visible:section.isConnected&&box.width>120&&free>=42,height:Math.min(110,Math.floor(free))};
        });
        for(const {extra,visible,height} of sizes){extra.hidden=!visible;if(visible)extra.style.height=`${height}px`;}
      });
    };
    state.observer=new ResizeObserver(measure);
    for(const {section,last} of sections){state.observer.observe(section);state.observer.observe(last);}
    states.set(container,state);measure();
  }
  window.FeluccaHardware={decorate};
})();
