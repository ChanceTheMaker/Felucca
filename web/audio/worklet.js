// SPDX-License-Identifier: GPL-3.0-only
class FeluccaProcessor extends AudioWorkletProcessor {
  constructor(options) {
    super();
    this.engine = new WebAssembly.Instance(options.processorOptions.module).exports;
    this.engine.synth_init();
    this.samples = new Int32Array(this.engine.memory.buffer);
    this.index = 32;
    this.position = 0;
    this.a = [0, 0]; this.b = [0, 0];
    this.state={tracks:[],g:[]};this.statusFrames=0;
    this.port.onmessage = ({data}) => {
      if (data.type === 'state') {
        const tracks=data.tracks || [data];
        tracks.forEach((t,k)=>{
          const old=this.state.tracks[k];this.engine.synth_target(k);
          if(old?.engine!==t.engine)this.engine.synth_engine(t.engine);
          t.p.forEach((v,i)=>{if(old?.engine!==t.engine || old?.p[i]!==v)this.engine.synth_param(i,v);});
          if(t.engine===12 && t.fm6?.length===128 && JSON.stringify(t.fm6)!==JSON.stringify(old?.fm6)) {
            new Uint8Array(this.engine.memory.buffer,this.engine.synth_fm6_buffer(),128).set(t.fm6);this.engine.synth_fm6_apply();
          }
          t.step?.forEach((s,i)=>{if(JSON.stringify(s)!==JSON.stringify(old?.step?.[i]))this.engine.synth_step(k,i,s.n,s.time,s.flags,s.vel,s.hit,s.acc,s.chance,...s.notes);});
          if(t.motion && JSON.stringify(t.motion)!==JSON.stringify(old?.motion)) {
            this.engine.synth_motion_clear(k);
            t.motion.events.forEach(e=>this.engine.synth_motion_event(k,e.step,e.param,e.value));
            this.engine.synth_motion_on(k,+t.motion.on);
          }
        });
        data.g.forEach((v,i)=>{if(this.state.g[i]!==v)this.engine.synth_global(i,v);});
        this.engine.synth_select(data.sel || 0);this.state={tracks,g:data.g};
      } else if (data.type === 'midi') this.engine.synth_midi(...data.bytes);
      else if(data.type==='transport') this.engine.synth_transport(data.op);
      else if(data.type==='chain') {
        this.engine.synth_chain_begin();
        data.slots.forEach((p,slot)=>{
          if(!p)return;
          const buffer=new Uint8Array(this.engine.memory.buffer,this.engine.synth_chain_steps(slot),4*64*11);
          p.tracks.forEach((t,k)=>{
            t.step.forEach((s,i)=>buffer.set([...s.notes,s.n,s.time,s.flags,s.vel,s.hit,s.acc,s.chance>=100?0:s.chance||101],(k*64+i)*11));
            this.engine.synth_chain_timing(slot,k,...t.p.slice(29,33));
            this.engine.synth_chain_motion(slot,k,+t.motion.on,255,0,0);
            t.motion.events.forEach(e=>this.engine.synth_chain_motion(slot,k,0,e.step,e.param,e.value));
          });
        });
        data.rows.forEach((r,i)=>this.engine.synth_chain_row(i,r.slot,r.repeat));
        this.engine.synth_chain_play();
      }
    };
  }
  next() {
    if (this.index === 32) {
      this.offset = this.engine.synth_render() / 4;
      this.index = 0;
    }
    const i = this.offset + this.index++ * 2;
    this.b[0] = Math.max(-1, Math.min(1, this.samples[i] / 32768));
    this.b[1] = Math.max(-1, Math.min(1, this.samples[i + 1] / 32768));
  }
  process(inputs, outputs) {
    const channels = outputs[0];
    if (!channels.length) return true;
    for (let i = 0; i < channels[0].length; i++) {
      while (this.position >= 1) {
        this.a[0] = this.b[0]; this.a[1] = this.b[1];
        this.next(); this.position--;
      }
      for (let c = 0; c < channels.length; c++) channels[c][i] = this.a[c % 2] + (this.b[c % 2] - this.a[c % 2]) * this.position;
      this.position += 44100 / sampleRate;
    }
    this.statusFrames+=channels[0].length;
    if(this.statusFrames>=sampleRate/20) {
      this.statusFrames=0;
      const chain=this.engine.synth_chain_status();
      this.port.postMessage?.({type:'transport',playing:!!this.engine.synth_playing(),positions:[0,1,2,3].map(k=>this.engine.synth_position(k)),chainPlaying:!!(chain&1),chainRow:(chain>>8)&255,chainRemaining:(chain>>16)&255});
    }
    return true;
  }
}
registerProcessor('felucca-dsp', FeluccaProcessor);
