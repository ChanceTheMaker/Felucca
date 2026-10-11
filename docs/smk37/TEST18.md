# SMK Test18: navigation and FM-1 workflow

Baseline: Felucca 1.5.1 `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.
The owner reviewed Test17 on the device and said it looks good, and subsequently
reported "18 was good" on October 10, 2026. This records an overall positive
Test18 device result, not individual sign-off on every check below. Test18
remains a private local candidate, not a public release.

## Controls

- Tap PATCH: Home/Mix and preset stepping. Hold PATCH: preset browser.
- Tap PARA: Projects/Save, without writing. Hold PARA: Undo/Redo.
- Tap BT: cycle engine, FM6 operator (FM6 only), slice (SLICE only), envelope,
  voice and modulation editing pages. Hold BT: Settings. Radio remains disabled.
- Hold GLO: grouped navigation. K5 selects family, K6 selects page, > opens,
  < returns Home. Diagnostic pages remain available and K4 exits them in bank 2.
- Hold threshold is configurable (0.3-0.6 seconds). A hold suppresses its tap;
  taps of those four navigation buttons execute on release.
- Bank 1 retains volume / tempo / preset / track and K5-K8 page controls.
  Bank 2 retains track / engine / preset / page.

## Settings and presets

Settings: palette, bold font, MIDI monitor OFF/EVENTS/NOTES, knob acceleration,
BPM lock, hold time, Home TRACKS/SCOPE, screen timeout, help hints, pad LED level
(and indicator off at level zero), animation ON/OFF. K5 selects, K6 changes,
> saves. Existing Controls, MIDI, Click/Record and Display pages remain.
Settings are separate from project slots; old project color/font is a fallback
until independent preferences have been stored. Screen timeout blanks the LCD
without powering down audio; local playing or controls wake it.

Browser: K5 engine or USER, K6 preset, K7 ALL/FAV, K8 favorite OFF/ON; > loads,
< Home. Empty favorites do not load another sound; one favorite remains one row.
Favorite changes queue a save until playback/notes/pedal are idle. A failed
write reports unsaved. Settings and stars sync with Studio using the existing
FM-1 Salt commands 34-38, including palette names and favorite/bank signatures.
No new command IDs. Read-only queries never write flash.

User Presets remains eight slots. K8 opens naming for the selected slot; K5
selects one of twelve characters, K6 changes it, > renames, < cancels. This
changes the stored name without replacing the sound. Existing Studio rename,
current-patch FM6 transfer and preset/project wire formats remain supported.

## Editing and display

FM6 OPERATOR / OP ENV / OPERATOR 2 retain upstream field ranges, reversed
operator storage order, OWN adoption and current-patch persistence. Operator
selection is shared across the pages. The algorithm and envelope drawings use
upstream graphics. K5 selects operator; remaining knobs edit coarse/fine/level,
stage/rate/level, or mode/detune/velocity respectively.

Sound undo captures one gesture (800 ms grouping), including engine, parameter
values and FM6 patch. > on Undo/Redo swaps it with the current sound (redo).
It covers panel parameters, faders, mode toggles, preset loads and editor sound
writes. Pattern undo also covers step edits, pad step entry and automation edits,
besides existing sequence tools. < swaps pattern/automation state. It is not an
unlimited history, project-load undo, global-mixer undo or storage-erase recovery.

The existing Salt monitor tracks sounding notes through actual voice ownership,
including local and USB sustain. NOTES shows up to four chromatic notes plus a
remaining count, not a named-chord classifier. EVENTS shows incoming USB MIDI.
It overlays the lower-right area of Home/engine panels. TRS is still disabled.

The odometer-style digits are adapted from upstream ui_draw.c: the same easing,
direction, stagger, clipping, retarget and snap rules. Numbers and sound change
immediately; animation affects rendering only. Rapid changes, label/page/engine
changes and number-width changes snap. ANIMATION OFF disables it. Local activity
uses a 30 ms display cadence; idle remains 250 ms. Device audio/display timing
with this higher activity refresh rate still needs a physical check.

SLICES shows the real source waveform and starts/end, with bounded incremental
ADPCM decoding and marker selection. Built-in BREAK/PIANO are read-only, as in
upstream. Manual marker changes/split/join remain blocked until the separate
user-sample storage work; this candidate does not claim functioning manual
slice edits. No user sample uploads, USB audio, TRS or Bluetooth are added.

Full FM-1 menu/hold-layer parity is not claimed: for example, performance key
layers, large-label/line styles, animated header details and factory phrase
browsing remain separate work. This is a board-adapted navigation pass using
upstream musical semantics, not a replacement FM-1 UI architecture.

## Storage compatibility

Projects keep the existing 3904-byte session and existing migration paths.
The preset bank grows from 2564 to 3116 bytes, inside the same two existing
4 KB sectors (0xF3000 and 0xFE000). Its tail stores upstream favorite bitsets
and versioned preferences. No additional sectors are authorized.
Both A/B candidates are CRC-checked and compared by generation even when one
uses the old length. Old banks are extended in RAM; the first replacement is
written to the other copy. Failed erases/programs/headers preserve the prior
committed copy. Boot, calibration and project sectors remain excluded.

Forward migration preserves Test17 presets. Test17 does not understand the
new bank length; after both copies have rotated to the new layout, a firmware
downgrade alone cannot read that bank. Keep a preset export/backup for downgrade.
This private SMK flash layout is not a binary FM-1 flash-image format.

## Verification and candidate

- Complete SMK host suite: keys/audio, controls, expression, persistence,
  Test12 migration, MIDI/clock, sequencer, songs, presets and editor wire formats.
- New workflow suite: old-bank migration and interrupted upgrade commits,
  protected ranges, empty/single favorites, hold/tap distinction, engine/preset
  undo, FM6 bounds/OWN and sustained-note monitor ownership.
- ASAN display suite: all 52 pages, 23 palettes, both fonts, engine bounds,
  operator stages, rolling-digit expiration/snap and IRQ-enabled LCD transfers.
- Actual C preference/favorite responses parsed by the shipping Studio parser.
- Target build: no compiler warnings, RAM/image bounds and RAM flash-call checks.
- Web updater: integrity, model rejection, bootstrap, success/disconnect cleanup.
- Actual rendered preview: `build/smk37-tests/test18-panels.png`.

Private candidate `build/smk37-webupdate-test18/`, identity `SMK37 TEST18 USB`
/ `SMK-37_918`, served only by the loopback preview at http://localhost:8770/.
Application 380888 bytes, SHA-256
`810d3dc5bc11a7ed1fcda3d9d30fdbd648f9a0e4edc9353a8e2ad96c2904626c`.
BSS end `0x1c16ebc`; pool end `0x1c685a4`.
Package 627712 bytes, SHA-256
`9519ded933d4e498572c8064847709d1fda026cab1fa12e6e8b655fc1bab30e8`.
Image SHA-256 `13e54e73ee6385f264d88f44b4f7a67a92e33ccfec1fa4521193666c3c7a6bff`.

On-device check: tap/hold navigation, preset load/star/rename, reboot persistence,
FM6 editing/undo, sustained-note display, and audio while values roll. No device
flash, commit, push or public deployment was performed by this work.

Hardware reference: [FM-1 / SMK processing and memory comparison](HARDWARE_COMPARISON.md).
