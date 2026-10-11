# SMK Test15: FM-1-style device display

Local candidate, 2026-10-10, on `integrate/smk37-upstream-20261010`.
Based on Test14, with upstream pinned at
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03` (Felucca 1.5.1).

## What changes

Test14 still used the SMK bring-up text layout. Test15 replaces the sound screens
with the FM-1's 240px layout: 24px header, four 57x44 parameter cards at y=28,
122px graphical panel at y=76, and footer at y=202. It uses the existing FM-1
fonts, palette tokens, rounded surfaces, values and gauges.

- Header: selected track, tempo, play/record status.
- MIX: four track rows with engine, level, mute and pattern activity.
- Envelope, LFO and scope: the actual upstream drawing functions, extracted
  unchanged to `ui_shapes.c` and used by both boards. Scope samples are captured
  before the click, after the master mix, as on FM-1.
- Sequencing pages: a 16-step SMK pad grid with active steps, edit selection and playhead.
- Settings/storage/tools: readable full-label rows beneath the parameter cards.
- Footer: action hints, knob bank, octave/fader bank where space permits,
  engine/preset name, transient notices and hardware fault messages.
- Existing physical mappings, storage layouts, update ranges and diagnostic
  pages are preserved. A page change clears the full screen; palette/font changes
  redraw it. Rendering keeps the existing 250ms cadence and enables IRQs during
  LCD transfers. The graph page identity is frozen before transfers so a physical
  page change cannot redirect envelope parameter indexing mid-frame.

This is a visual adapter, **not full FM-1 UI parity**. SMK retains its existing
43-page navigation and shortcuts. FM-1 layered menus, rolling digit animation,
hot-knob highlights, specialized engine/FX diagrams, piano roll, graphical song
and preset browsers remain separate work. Pages without a specialized graph
show the live output scope. No FM-1 battery calibration is applied to the SMK.

## Validation

- `tools/test_smk37_application.sh`: synth, controls, editor wire protocol,
  migration, interrupted storage commits, protected sectors, song/preset/tool
  behavior and the new renderer pass.
- `tools/test_smk37_upstream.sh ui`: original FM-1 UI regression suite passes
  after shared curve extraction.
- `tests/smk37_draw_test.c`: ASAN render of every SMK sound page in all 23
  palettes and both font weights, engine parameter extremes, LCD bounds and
  IRQ-enabled transfers. Fault notices also exercised.
- Actual C-rendered screenshots reviewed in `build/smk37-tests/test15-screens.png`.
- Target build passes entry/image/memory bounds and call-free flash RAM checks.
  Application 358024 bytes; BSS end `0x1c165bc`;
  pool end `0x1c67d24`.
- Device installation, display transfer/audio timing and long-running playback
  with this exact artifact still need the owner's hardware test.

## Artifact / local installation

Private package: `smk37-test15-private.smkupdate` (627712 bytes).

Application SHA-256: `d52a1b6da62a9f3b4e45bb09334d5a878154a365dd487d078669fd108180a287`

Package SHA-256: `7e9d1612ef4e7dc20d06be446826503ce564285e6a66486e0d7a560dd864125c`

Full private image SHA-256: `165c6a7f3d0ffdfc3c05bad26f990f6e2ebff7b2c71820c90a817f86ead0eb7f`

Staged and served-package SHA-256 verified on the existing loopback installer at `http://localhost:8770/#install`.
Confirm **SMK37 TEST15 USB** after refreshing. The package is matched to the
owner's backed-up Pro; it stays outside Git. Test14 artifacts remain available.
No automatic device flash, Git push or public publication is part of this work.

## Local website refresh

The local landing page now describes Test15, song sections, one-level pattern-tool
undo/redo, eight device presets, and both physical knob banks. It separates the
existing direct-USB update path from the adapter bootstrap needed for stock or
Test10, and distinguishes confirmed Test12 hardware results from candidate tests.
New Studio imagery is captured in Browser audio mode; device imagery comes from
the real C renderer and is labeled accordingly. Browser mode hides the mock
firmware identity and keeps the SMK hardware toggle label. No installer protocol,
staged firmware bytes, public deployment or hardware was changed for this refresh.
