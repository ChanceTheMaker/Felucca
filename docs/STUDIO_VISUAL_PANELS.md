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
  Double-click an envelope or cutoff handle to restore the parameter default.
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
  editing and reset, simulated device pushes, all thirteen engines, eight languages,
  card ordering, 192 light/dark/theme/control/viewport combinations (320-1440 pixels), generic
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

## Review follow-up

`fix/studio-widget-polish` follows the separate typing-keyboard correction on
`fix/studio-piano-shortcuts`. Its review fixes include:

- The cutoff handle uses the same logarithmic frequency axis and response curve
  as the plot. Pointer-to-cutoff conversion is verified at all 128 values.
- Arpeggiator pitches are separate horizontal gate-length bars, avoiding diagonal
  lines that looked like pitch glide.
- Pressed graph handles keep their position; the generic button active style
  previously removed their centering transform. The decorative SVG cannot
  intercept pointer input. Browser tests verify no value jump on press, real
  dragging, and double-click reset.
- Larger captions, 28-pixel handle targets, 32-pixel shortcut buttons and more
  legible out-of-scale notes improve use on small displays.
- The modulation matrix has translated Source, Destination and Amount headings.

## Open-select wheel navigation

`feat/studio-select-wheel` adds live option changes when the mouse wheel is over
an open Studio select or its picker. Closed controls and wheel events outside
the menu do not change values. Small trackpad deltas accumulate; direction
changes, reopening and idle gaps reset the gesture. Disabled/hidden options are
skipped, the ends do not wrap, and Escape closes the picker keeping the value
already applied. Existing input/change handlers update the synth and graphs.

The implementation uses [native customizable selects](https://developer.mozilla.org/en-US/docs/Learn_web_development/Extensions/Forms/Customizable_select),
including native keyboard, focus, dismissal and touch behavior. Browsers without
`base-select` and `:open` support retain their normal native menus. Chrome's
top-layer options do not set `:hover` on their parent select, so the actual wheel
event target identifies the hovered menu.

Validation includes real browser wheel events, outside/closed controls, limits,
hidden/disabled options, Escape, waveform updates, and keyboard navigation via
`web/test_select_wheel.mjs`, plus the Studio layout/audio regression suite.
Set `STUDIO_FIXTURE_DIR` to a built `webapp/editor` directory to serve its static
assets directly to an isolated test browser. This avoids intermittent shared
preview connection failures; it does not mock the UI, select behavior or DSP.

## Decorative hardware panels

`feat/studio-hardware-fillers` fills only unused space inside the unified Sound
cards with understated brushed-aluminum panels and small corner screws. Most
panels are plain. Perforated grilles are limited to the spare VOICE level cells
and the FX, Delay and ARP spare sections, giving four grille accents in the layout.
The neutral metal finish follows the theme's panel tone. Jack sockets and
circular speaker drivers remain removed. All artwork is CSS, with no external
image assets.

In knob mode, panels occupy spare cells in the existing four-control rows.
Other panels fit blank space beneath shorter sections of a shared card. They
hide when that space is too small, including when responsive columns stack.
They never create extra rows, enlarge cards or occupy gaps between cards.
Decoration is inert, hidden from assistive technology and ignores pointer input;
it has no audio or routing behavior.

`web/test_studio_hardware.mjs` passes 48 theme, light/dark, control-mode and
viewport combinations. It checks bounds, control overlap, noninteraction and
rebuilds, verifies that plain panels outnumber grilles and no sockets or drivers remain, and compares
card/control geometry with decoration removed. Desktop and mobile screenshots
were inspected. The earlier complete Studio regression passed thirteen engines,
eight languages, 192 layout/theme combinations and actual browser DSP output.
These browser checks used built-site fixtures because the shared preview server
intermittently resets asset requests.
