# SMK-37 upstream 1.5.1 integration

Latest candidate: [Test14](TEST14.md) adds songs, tools, presets and the updated
knob banks. The Test13 evidence below records the initial foundation.

## Source and release state

Branch `integrate/smk37-upstream-20261010` starts at Test12 commit
`36875e41f236d243c984fd31839b1a864b4642eb`. It imports the source delta from
upstream 1.0.1 (`20c275e39f75fa820978032efaceddfc5283c8cb`) to 1.5.1
(`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`). This is not a Git merge.
Upstream authorship, notices and font licenses are retained.

Test13 is an offline-built candidate, not a hardware-validated release.
No device was flashed during integration. The localhost:8770 installer still
stages Test12. New binaries, stock containers and calibration stay in ignored
`build/` and private backup directories, outside Git.

Website branding is **SMK-37 Pro & Elite**. The current private installer package
is matched to the owner's Pro_016 test device. Model headings do not alter its
identity, JEDEC, calibration or protected-range checks.

## Integrated behavior

- Updated shared synthesis, presets, samples, envelope/LFO/modulation behavior,
  spread, analog filter modes, reverb, dotted delay and insert effects.
- Chance, ratchet, nudge, velocity and track quantization; 128 automation/lock
  records, with physical event editing and browser event/value editing.
- MIDI Program Change and 16 learned CC assignments; no new command numbers
  collide with upstream. Browser Settings can assign/remove CC mappings.
- Metronome and recording count-in; click is added to headphone/DAC audio.
- Pad-held performance effects, including reverb/echo throws, with four macros.
- SMK display palette/bold controls: 21 retained Salt palette IDs plus upstream
  NIGHT and black/white MONO. Preferences persist in explicitly saved projects.
- Updated browser DSP and 111-parameter editor cards; engine-specific controls,
  inserts, LFO sync, envelope sync, spread and recording quantization.
- SMK editor capability tags advertise only implemented operations. Test14 advertises eight real user preset slots when their storage is authorized;
  no writable FM6 voice banks, sample uploads or complete backup capability.
- Full 37-key piano ownership and upper-key arp remapping. The independent 16
  pad owners retain fixed-note mapping. The Studio keyboard remains C3..C6 for
  SMK and F3..G5 for FM-1; octave labels follow the connected model.

## Device controls

Test14 KNOB BANK off (bank 1): knobs 1-4 are master volume, SELECT (tempo),
PRESETS and ALGORITHM (track 1-4), matching current FM-1 Felucca. Bank 2
(LED on): track, engine, presets and page. Knobs 5-8 always edit K1-K4. Existing
fader, wheel, pedal, button and light assignments stay in the SMK hardware layer.

New pages, after Projects: INSERT, INSERT MIX, LFO SYNC, DRUM LEVELS 1/2,
STEP DETAIL, MIDI, MIDI LEARN, CLICK / RECORD, DISPLAY, TIMING, PERFORMANCE FX
and AUTOMATION. Input/light/pedal diagnostics follow them.

- FX cycles Effects -> Insert -> Performance FX. Performance pads 1-14 hold
  repeat/reverse/filter/tape/freeze/octave/flanger/phaser/throw effects; pads
  15-16 hold track 1/2 mute. Releasing a pad ends its effect even after leaving
  the page. The four knobs edit filter, crush, throw and depth macros.
- SEQ cycles Sequencer -> Step Edit -> Step Detail -> Automation. In Step
  Detail, pads select one of 16 steps; < / > move through the four step banks.
  Knobs edit chance, ratchet, nudge and velocity.
- Automation knobs select step, parameter, value and lock/event type. > writes
  the event; < removes it. Save a project to retain it.
- MIDI Learn can select target track/parameter, then arm capture. Turning an
  ordinary sound parameter while learning arms that parameter. Incoming CC is
  captured unless reserved by the MIDI implementation. Save a project to keep
  the mapping. Program Change uses the selected engine's valid preset list.
- Display chooses a palette and bold text. Click / Record chooses click mode,
  level and count-in. These settings are part of a saved project.

## Project compatibility and flash boundaries

Test10-12 sessions contain a 3584-byte FUN8 project plus 64 bytes of SMK metadata.
Test13 uses FUN10 (3840 bytes) plus the same 64-byte metadata area: 3904 total.
The loader verifies the original CRC before expanding old saves in RAM. It then
runs upstream's project migration; merely reading does not rewrite flash.

Each project retains two 4096-byte sectors, inside **0xEB000..0xF3000** only.
Old headers use payload offset 256; new large payloads use offset 128. Writes
split at physical 256-byte page boundaries, verify payload, then commit the
header last. The newest valid generation wins, including mixed old/new copies.

A save in Test13 writes the new format. **Test12 cannot read that new copy.**
The first replacement retains the old copy, but another save can replace it.
Keep the private flash backup before upgrading; do not rely on a firmware-only
downgrade to recover newer projects. Bootloader, calibration and stock storage
are outside both the save and application-update write ranges.

## Deliberate SMK differences and remaining parity work

The compact SMK control pages replace the FM-1 button-combination UI. This is
not a claim of complete FM-1 feature/UI parity. FM-1-only LED animations and
key layers are not copied onto differently wired controls. Test14 adds song-chain playback preparation, pattern tools/undo and user presets;
see [TEST14.md](TEST14.md). Favorites still need SMK-specific integration.
Song data in imported projects is retained.

Bluetooth, TRS input, USB audio, user sample writing and complete web backup
remain outside this candidate. No FM-1 flash geometry is used for SMK storage.
The adapter remains necessary after interrupted application programming; this
update does not make the recovery bootloader persistent.

## Validation

- Target compiler/linker builds Test13 and its RAM USB loader; checks flash RAM
  routines for external calls and uses SMK-specific application/package bounds.
- `tools/test_smk37_application.sh`: real sound generation across engines,
  37 keys, pads, ownership, banks, eight encoders/buttons, sustain, persistence,
  actual C-to-JS editor replies and MIDI/parser regressions.
- `tests/smk37_upstream_test.c`: a synthetic save produced by Test12 source
  (fixture `smk37_test12_save.h`, no private device data), FUN8-to-FUN10 migration,
  new settings/learn-map roundtrip, key36 arpeggiation, step ratchet/nudge,
  parameter locks, learned CC, Program Change, performance-pad release and click.
- `tests/smk37_store_test.c`: interrupted erase/program/header stages for old,
  new and mixed-format copies, corruption fallback, physical page boundaries,
  unchanged bytes outside the save range.
- `tools/test_smk37_upstream.sh`: sound14/sound15, insert, reverb, click/count-in, performance effects, drum, FM6,
  FUN10, ratchet, step15, detail, motion, MIDI control, projects, editor,
  settings, palette and shared UI regressions.
- Python SMK backup/bootstrap/package/layout/control/USB tests: 67 pass.
- Web protocol, updater, keyboard and all eight locale catalogs pass. New
  catalog strings currently use English fallback outside supplied translations.
- Browser WebAssembly engine: every selectable engine/factory preset produces
  sound; custom FM6 patches, sustain, panic and 44.1/48 kHz worklet tests pass.

FM-1 stock-package installation tests were not run: no FM-1 release package is
being produced. No new SMK hardware, audio CPU-load or long-duration checks have
been claimed. Test13 needs a normal-USB install/restart, old-project load/save,
new-effect/control and sustained-performance check on the Pro before promotion.

Browser smoke test `web/test_smk37_browser.mjs` also passes using real C firmware
descriptors over a simulated Web MIDI transport: 37-key C3 range, themed INSERT
cards, 111 MIDI-learn parameter choices, assignment UI and sequencer nudge.
The test does not open a physical MIDI port. It blocks remote font/analytics
requests so external networking cannot hold up the local functional check.

Final private Test13 package: `build/smk37-webupdate-test13/smk37-test13-private.smkupdate`.
SHA-256: `ecf20f9b8c241d74fd44508812edf4f143a42dd4c426aa3d3d380c43b03c498e`.
Application SHA-256: `e2afd63fbc3a3e225e721a2a89b08db46010884c6aa5aca676eebad1e075ea24`.
