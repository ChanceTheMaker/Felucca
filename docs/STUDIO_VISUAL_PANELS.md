# Studio visual panels

This is an original Salt implementation using the collaborator's screenshot as
a layout reference. No collaborator source or assets were imported. It builds
on `feat/rc1-salt-header-logo` (`23e4963`), above the RC1 candidate `7b1164a`.
The integrated upstream baseline remains v1.0.1, commit
`20c275e39f75fa820978032efaceddfc5283c8cb`.

## Behavior

- Engine pages share one card; the ANALOG filter includes a low-pass response
  guide and an editable cutoff handle.
- Envelope and envelope destinations share a card. Attack, decay, sustain and
  release handles support pointer dragging, arrow keys, Shift+arrows, Home and End.
- Voice settings share a card; LFO and destinations share another. The waveform
  preview follows waveform and phase, with quick waveform selection.
- The effects strip contains FX levels, slicer, delay, reverb and chorus. The
  slicer preview follows the firmware's sixteen patterns, mode and depth, and
  provides previous/next pattern buttons.
- ARP includes an example G-C-E phrase, octave selection and Hold. Scale/root
  selection highlights notes in the current firmware scale; chord settings sit
  alongside it.
- Existing knobs/sliders, modulation slots, card reordering and all thirteen
  engines remain available. The layout folds to two columns and then one column
  on smaller screens. New labels are in all eight existing language catalogs.

Widgets use the editor's existing parameter writes and device-change updates.
They do not implement another synth, MIDI transport or polling loop. Unknown
firmware layouts retain the generic descriptor-based controls without widgets.
Combined cards use new preference/order keys; the global knob/slider preference
is retained, while earlier per-page card overrides/order do not map automatically.

## What the plots mean

These are parameter guides, not audio measurements or live sequencer playheads.
Envelope stages have separate horizontal scales to keep short stages editable.
The filter response is illustrative and only shown for the known ANALOG filter.
The arpeggiator uses an example chord; random and sample-and-hold displays use
representative sequences, not the DSP's random state. Firmware scale masks and
slicer pattern bits are verified against their C tables.

## Validation

- `node web/test_web.mjs`: existing editor, protocol, samples and updater suite.
- `node web/test_studio_widgets.mjs`: all firmware scale masks and slicer patterns,
  LFO shapes/phase, arpeggiator traversal/repeat, envelope and filter bounds.
- `web/test_studio_panels.mjs`: local Chrome interactions, pointer/keyboard
  editing, simulated device pushes, all thirteen engines, translated labels,
  card ordering, 96 theme/control/viewport combinations (320-1440 pixels), generic
  firmware fallback and actual WASM/audio-worklet output.
- Built RC1 preview and inspected desktop/mobile screenshots in slider and knob
  modes. The logo after `[Salt]` is a separate prerequisite change.

Run the browser test against an already-built preview with Playwright installed:

```sh
STUDIO_URL=http://localhost:8768/rc1/webapp/editor/ node web/test_studio_panels.mjs
```

`PLAYWRIGHT_MODULE` can point to an existing Playwright module. `BROWSER_CHANNEL`
defaults to Chrome. The test exposes internal state only in its intercepted HTML
response; production assets contain no test hook. It uses a simulated FM-1 and
browser audio, never a physical MIDI device.

No firmware, DSP, protocol or release-package change is included. Physical FM-1
validation remains pending. This is a fork review branch; no upstream submission,
merge, firmware promotion or website publication is part of this round.
