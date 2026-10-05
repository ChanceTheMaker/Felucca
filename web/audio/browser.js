// SPDX-License-Identifier: GPL-3.0-only
export class BrowserSynth {
  async start() {
    if (this.context) { await this.context.resume(); return; }
    const Context = window.AudioContext || window.webkitAudioContext;
    if (!Context) throw new Error(window.FeluccaI18n.t('ui.noWebAudio'));
    const context = new Context({sampleRate: 44100, latencyHint: 'interactive'});
    this.context = context;
    try {
      await context.resume();
      const response = await fetch(new URL('engine.wasm', import.meta.url));
      if (!response.ok) throw new Error(window.FeluccaI18n.t('ui.engineUnavailable'));
      const module = await WebAssembly.compile(await response.arrayBuffer());
      const meta = new WebAssembly.Instance(module).exports;
      const bytes = new Uint8Array(meta.memory.buffer);
      const text = p => new TextDecoder().decode(bytes.subarray(p, bytes.indexOf(0, p)));
      this.presets = Array.from({length: meta.engine_count()}, (_, e) => Array.from({length: meta.preset_count(e)}, (_, p) => ({
        name: text(meta.preset_name(e, p)), values: Array.from({length: meta.param_count()}, (_, i) => meta.preset_value(e, p, i)),
      })));
      await context.audioWorklet.addModule(new URL('worklet.js', import.meta.url));
      this.node = new AudioWorkletNode(context, 'felucca-dsp', {numberOfInputs: 0, numberOfOutputs: 1,
        outputChannelCount: [2], processorOptions: {module}});
      this.gain = context.createGain();
      this.gain.gain.value = 0.35;
      this.node.connect(this.gain).connect(context.destination);
      this.analyser = context.createAnalyser();
      this.analyser.fftSize = 2048;
      this.gain.connect(this.analyser);
      this.node.onprocessorerror = () => {
        this.gain.gain.value = 0;
        window.FeluccaI18n.text(document.getElementById('audio-status'), 'ui.audioFailed');
      };
    } catch (error) {
      await context.close(); this.context = null; throw error;
    }
  }
  sync(state) {
    const data = {type: 'state', engine: state.engine, p: Array.from(state.p), g: Array.from(state.g),
      fm6: state.engine === 12 ? Array.from(state.tracks[state.sel].fm6) : null};
    const signature = JSON.stringify(data);
    if (signature === this.signature) return;
    this.signature = signature;
    this.node.port.postMessage(data);
  }
  midi(bytes) { this.node?.port.postMessage({type: 'midi', bytes: Array.from(bytes)}); }
  volume(value) { this.gain?.gain.setTargetAtTime(value, this.context.currentTime, 0.015); }
  showScope(canvas) {
    if (this.scopeFrame) cancelAnimationFrame(this.scopeFrame);
    const ctx = canvas.getContext('2d');
    const data = new Float32Array(2048);
    const draw = () => {
      if (!canvas.isConnected || canvas.parentElement.hidden) { this.scopeFrame = null; return; }
      if (!document.hidden) {
        const rect = canvas.getBoundingClientRect(), ratio = Math.min(devicePixelRatio || 1, 2);
        const w = Math.round(rect.width * ratio), h = Math.round(rect.height * ratio);
        if (canvas.width !== w || canvas.height !== h) { canvas.width = w; canvas.height = h; }
        const style = getComputedStyle(canvas);
        ctx.clearRect(0, 0, w, h);
        ctx.strokeStyle = style.getPropertyValue('--scope-grid'); ctx.lineWidth = ratio;
        ctx.beginPath();
        for (let i=1;i<16;i++) { const x=w*i/16; ctx.moveTo(x,0); ctx.lineTo(x,h); }
        for (let i=1;i<4;i++) { const y=h*i/4; ctx.moveTo(0,y); ctx.lineTo(w,y); }
        ctx.stroke();
        data.fill(0);
        if (this.context?.state === 'running') this.analyser?.getFloatTimeDomainData(data);
        // Rising zero-crossing trigger keeps periodic notes visually steady.
        let start=0;
        for(let i=1;i<1024;i++) if(data[i-1]<=0 && data[i]>0.002) { start=i; break; }
        ctx.strokeStyle=style.color; ctx.lineWidth=1.5*ratio;
        ctx.shadowColor=style.color; ctx.shadowBlur=4*ratio;
        ctx.beginPath();
        for(let i=0;i<1024;i++) {
          const x=i*w/1023, y=h/2-Math.max(-1,Math.min(1,data[start+i]))*h*0.46;
          if(i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
        }
        ctx.stroke(); ctx.shadowBlur=0;
      }
      this.scopeFrame=requestAnimationFrame(draw);
    };
    draw();
  }
  async stop() {
    this.midi([0xb0, 120, 0]); this.midi([0xb0, 64, 0]);
    if (this.context?.state === 'running') await this.context.suspend();
  }
}
