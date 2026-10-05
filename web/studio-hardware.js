// SPDX-License-Identifier: GPL-3.0-only
// Decorative rack hardware occupies existing blank space; never sound controls.
(() => {
  const states=new WeakMap();
  function panel(vented=false) {
    const item=document.createElement('span');
    item.className=`hardware-panel${vented?' hardware-grille':''}`;
    for(let i=0;i<4;i++) {
      const screw=document.createElement('span');
      screw.className='hardware-fastener';item.append(screw);
    }
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
      extra.append(panel(count===1 && bank.closest('.unified-card').dataset.section==='VOICE'));
      bank.append(extra);
    }
    for(const section of container.querySelectorAll('.unified-card .bank-section')) {
      const last=section.lastElementChild;
      if(!last)continue;
      const extra=filler('bank-hardware-gap');extra.hidden=true;
      // One vent in the effects strip; other spare areas are quiet blank plates.
      extra.append(panel(section.closest('.unified-card').dataset.section==='FX' && section===section.parentElement.firstElementChild));
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
