# Saxophone synthesis plan

Improve Felucca's saxophone sound in stages: evaluate the existing sampled
tenor sax first, improve samples and expression, then prototype a monophonic
reed model in browser Studio. Consider a hybrid only after listening and
profiling show a benefit. This is a recorded proposal for later feature
selection; no new sax engine is implemented by this document.

## Existing foundation

The RC1 source already includes a `SAX` set in the SAMPLE engine.
`tools/gen_samples.py` defines it as a looped sustain instrument. The sample
attribution lists three non-vibrato tenor recordings from Versilian Studios'
VCSL library, with original filenames rooted at A#1, A#2 and A#3. Their recorded
license is CC0; preserve source attribution when replacing or extending them.

The sample engine stores IMA ADPCM data and selects zones by note range.
It does not currently choose alternate recordings by velocity or round robin.
The user uploader converts to mono 22050 Hz, with up to 16 zones per slot;
each of three FM-1 user slots is 80 KiB including metadata, roughly seven
seconds of combined sample data. This is a total across zones, not seven
seconds per note. SMK storage capacity and layout remain a separate porting
question, so do not promise a larger sample budget there.

PHYS currently offers MODAL, STRNG, MEMB and SYMP models. A dedicated saxophone
reed/air-column model would be new DSP, rather than a preset of those models.

Code references: [sample sets](../tools/gen_samples.py),
[sample playback](../firmware/src/eng_sample.c),
[sample uploader](../tools/fm1_sample_upload.py),
[sample attribution](../assets/samples-cc0/ATTRIBUTION.txt),
[physical models](../firmware/src/eng_phys.c).

## Options

| Approach | Benefit | Main work and limits |
| --- | --- | --- |
| Better samples | Recognizable acoustic tone quickly | Better attacks, tuning, loops and note coverage; storage limits; velocity layers and articulation switching require engine/format work |
| Physical modeling | Continuous breath, reed response, pitch transitions and expressive timbre | New nonlinear reed/waveguide DSP; tuning, stability and CPU validation; realism requires careful listening and parameter design |
| Hybrid | Recorded attacks or noise with a responsive synthesized sustain | Phase/pitch matching, envelope transitions and extra memory/CPU; improvement is not guaranteed |

Sax-like digital waveguide synthesis is established. STK's [Saxofony][stk]
is a useful reference for a simplified reed instrument with sax-like behavior,
not proof that an unchanged model will sound convincingly acoustic or fit our
hardware budget. Review any code's license and attribution before reuse.

## Proposed feature sequence

1. **Baseline and sampled tenor quality.** Render the current SAX across its
   register and dynamics, document obvious loop/tuning/attack problems, and
   compare compact replacement samples. Use licensed source recordings or the
   owner's recordings. Keep this as a sample-content change where possible.
2. **Expressive playback.** Add controlled legato and smooth pressure-driven
   changes to level and brightness. Use a wheel/fader initially; support
   standard breath/expression input when routed by the MIDI layer. Confirm
   pressure availability separately for each hardware controller.
3. **Browser reed prototype.** Start monophonic with reed response, a bore
   model, breath noise, vibrato and pitch control. Keep its sound and CPU tests
   separate from the SMK board port. Compare it with the sampled baseline.
4. **Optional hybrid.** Evaluate short recorded attacks or breath/key noise
   blended with modeled sustain. Retain this only if it improves the sound
   without unacceptable discontinuities or cost.
5. **Embedded integration.** Port the chosen design after measuring cost;
   determine fixed-point requirements and memory use. Preserve existing engine
   IDs, presets, saved projects and protocol compatibility. Rebuild browser
   audio from the integrated source and validate on each actual device.

Split engine/format changes, sample assets and controller-expression work into
focused PRs. No SMK firmware promotion depends on this optional instrument.

## Expression design

Velocity can control the initial attack, but sustained expression should use
a continuous control to shape timbre and amplitude together. Proposed controls
are breath/pressure, reed or tone, growl/noise, vibrato amount/rate and bend.
Smooth parameter changes and handle note transitions intentionally so expression
does not introduce zipper noise, clicks or stuck notes.

Breath-controller input is optional; a wheel or fader must make the instrument
playable from the SMK keyboard. Do not assume the keybed has aftertouch simply
because the pads report pressure. Choose tenor first to compare with the
existing material; alto or other voices can follow a successful baseline.

## Acceptance and open choices

- Listen to clean sustained notes, soft/hard attacks, tongued repeats, legato,
  bends, vibrato and register changes. Compare at matched loudness.
- Test pitch accuracy and stability across the intended register; pressure
  extremes must remain bounded and silent after release/panic/reset.
- For samples, inspect loop discontinuities, tuning and pitch-shift artifacts.
  For hybrids, inspect the attack-to-sustain transition and phase behavior.
- Measure peak CPU, memory and flash use at 44.1/48 kHz in the browser and at
  the supported hardware rate. Profile worst-case controls with other tracks
  and effects active before selecting polyphony.
- Re-run existing sound regression tests without changing unrelated golden
  baselines. Use listening review alongside automated clipping/DC/stability
  tests; passing numerical tests does not establish acoustic realism.

Before implementation, choose the first target sound (clean tenor, breathy
jazz, bright pop, or another reference), desired expressive controls and sample
source. A compact expressive instrument is the starting target; large desktop
sample-library realism is not a promised embedded outcome.

[stk]: https://github.com/thestk/stk/blob/master/include/Saxofony.h
