# SMK Test17: page content and physical shortcuts

Based on Test16; upstream remains pinned to Felucca 1.5.1
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.

## Device display

Test15/16 ported the cards and a few graphs but left most centers as a generic
output scope. Test17 shares the actual FM-1 Scale, Chord, FX sends, insert
transfer curves, Slicer, modulation routing, organ drawbars, sample waveform
and FM6 algorithm diagrams. Explicit graph arguments replace FM-1-only UI
state in the shared drawing modules; original FM-1 behavior is preserved.

The piano roll and drum grid retain upstream geometry, ties, ratchets, scale
lanes and automation marks, adapted to SMK navigation and all 53 local note
owners (37 piano keys plus 16 pads). SMK step-detail graphs show chance,
ratchets, nudge and velocity. Other SMK views show song-section/track assignments,
saved/empty project slots, named user presets, automation rows, drum levels,
theme swatches and the actual 16-pad performance FX map. Remaining parameter
pages show their live values and units. Engines without an upstream-specific
instrument diagram retain the live audio scope, as on FM-1.

Page and track selection are frozen for each frame. LCD transfers and read-only
project enumeration run with interrupts enabled. Sample waveform decoding is
bounded to 4096 samples per display pass. No flash writes originate in rendering.

This completes the missing content pass for the current 43 SMK pages. It does
not import the complete FM-1 navigation/menu stack, hold layers, animated digits,
or absent dedicated operator/slice editing pages. Those remain separate UI work;
this candidate must not be described as full FM-1 UI parity.

## Buttons

- PATCH: Home/Mix and factory-preset browsing.
- PARA: Projects/Save menu; knob 5 selects A-D, > explicitly saves, < loads.
  Opening this menu, including repeated presses, never initiates a save.
- BT: parameter editing. Its steady indicator follows the editing pages;
  Bluetooth itself remains disabled. Radio blinking is deferred to BT support.

Installer instructions describe these mappings. The automatic hero finish order
is red, white, Elite slate/orange, black, pink, charcoal, then repeats. Finishes
crossfade over 1.5 seconds every five seconds; no selector buttons. Reduced-motion
and background-tab handling are retained.

## Verification

- Complete SMK host application suite passes: synth, keys, buttons, storage,
  project migration, presets, song chains, MIDI and C-to-JavaScript protocol.
- ASAN renderer covers every page in all 23 palettes and both fonts, engine
  extremes, every FM6 algorithm, LCD bounds and IRQ-enabled display transfers.
- Center-panel hashes change with Scale, FX, Chord, Insert, Slicer, modulation,
  FM6 algorithm and sequence edits; card-digit changes alone cannot pass.
- Piano-roll held-note tests include piano key 37 and pad 16.
- Original FM-1 UI behavioral regression passes after shared graph extraction.
- FM-1 visual regression: 215 screens x 23 palettes plus page/value sweeps;
  zero layout findings, invisible ink or GREY/MONO color violations, and
  3,377,391 alignment measurements with none off by one pixel or more.
- Chrome hero test passes exact finish order, wraparound, captions, genuine
  crossfade and absence of selector buttons.
- USB updater integrity, wrong-model rejection and disconnect cleanup pass.
- Target build has no compiler warnings; memory/image bounds and call-free
  RAM flash checks pass.
- Actual C-rendered panel sheet: `build/smk37-tests/test17-panels.png`.
- This exact artifact still needs device installation and audio/display timing
  checks. No device flash, public publication, commit or push in this round.

## Private local candidate

`build/smk37-webupdate-test17/`; unit-specific and excluded from Git/public assets.
Identity: `SMK37 TEST17 USB` / `SMK-37_917`.

Application: 367648 bytes, SHA-256 `6c8ecfa932a549268604d836dc0862ff6a81fe6b588d0107278524c4cab7dc46`.
BSS end `0x1c1675c`; pool end `0x1c67d24`.

Package: `smk37-test17-private.smkupdate` (627712 bytes),
SHA-256 `a43d02e59aa2633ceab5d428de2429df33f95feb2febdf234478e878a05206e6`.
Full-image SHA-256 `3762aeab6fd3c53f1a2858fc7767e99455305cff2058ee923bd305cf2993a98d`.

Application packaging preserves bootloader, calibration and save sectors outside
0x4000..0x9D000. Test16 and its recovery artifacts remain available.

Local installer at http://localhost:8770/ serves Test17; the downloaded package
length and SHA-256 were verified against the candidate metadata.

## Device follow-up

On October 10 the owner reported: "Test17 looks good to me." This confirms
the owner's display review on the device; it is not evidence for Test18.
