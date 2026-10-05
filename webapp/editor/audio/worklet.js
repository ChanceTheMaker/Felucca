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
    this.port.onmessage = ({data}) => {
      if (data.type === 'state') {
        this.engine.synth_engine(data.engine);
        data.p.forEach((v, i) => this.engine.synth_param(i, v));
        data.g.forEach((v, i) => this.engine.synth_global(i, v));
      } else if (data.type === 'midi') this.engine.synth_midi(...data.bytes);
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
    return true;
  }
}
registerProcessor('felucca-dsp', FeluccaProcessor);
