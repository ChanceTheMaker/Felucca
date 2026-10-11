# SMK-37 Test14: songs, tools, presets and panel mapping

Test14 continues the 1.5.1 integration on `integrate/smk37-upstream-20261010`,
after foundation commit `8e949c4`. Upstream remains pinned to
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`. Shared song sequencing, preset
records and pattern-tool algorithms retain their upstream authorship. SMK
adapters supply physical controls, storage and main-loop protocol dispatch.

The owner requested device preset storage and confirmed FM-1-style bank 1:
SELECT is tempo and ALGORITHM selects the track. No device was opened, written
or flashed during this work. No push/public deployment occurred. The private
localhost:8770 installer still stages the owner-tested Test12 package.

## Physical controls

| Physical knob | Bank 1 (KNOB BANK off) | Bank 2 (KNOB BANK on) |
| --- | --- | --- |
| 1 | Master volume | Track |
| 2 | SELECT: global tempo | Synth engine |
| 3 | PRESETS: factory sounds in current engine | Same |
| 4 | ALGORITHM: track 1-4 | Page |
| 5-8 | Current page K1-K4 | Same K1-K4 |

The bank no longer swaps the first and last four knobs. Diagnostic pages keep
knob 4 as their page selector, with their dedicated input/light controls.
Master volume starts at the existing 25% bench level. It can reach silence or
unity gain, and an explicit project save retains it. Older projects without
the new volume marker load at 25%. No format size or project boundary changes.

Three new pages follow AUTOMATION. Use bank 2 knob 4 to reach them:

- **SONG CHAIN:** K1 section, K2 track, K3 source project A-D or SILENT,
  K4 repeats (1-16). `>` adds a section; `<` removes the selected section.
  SEQ PLAY prepares/starts the chain; another press stops it. Save a project
  to retain the chain configuration. Up to 16 sections. Preparation reads
  saved patterns in the main loop; current sounds and editable patterns stay
  separate. Missing source projects prevent starting. Engine-specific
  automation from a different saved instrument is omitted, as upstream does.
- **SEQ TOOLS:** K1 action, K2 drum lane. `>` runs the action; `<` swaps
  undo/redo of the last tool action. Clear, reverse, shift left/right, random,
  cook, basic beat, and drum-lane clear/reverse/fill/random. Locks and motion
  follow reversed/shifted steps. Other tracks' automation is preserved.
  This is one-level **pattern-tool undo**, not a global history of every sound,
  recording, preset load or flash operation. Song playback blocks these tools.
- **USER PRESETS:** K1 slot U01-U08; `<` loads its sound; `>` explicitly
  overwrites it with the current sound. Stop/release keys and pedal first.
  The selected slot's name is displayed. Factory preset navigation remains
  separate. Browser Library can name, store, load, import/export and erase
  device slots through the existing upstream commands 16-21.

Studio Projects now shows four source selectors per section, including silent
lanes. Legacy whole-project SONG operations 0-3 still work; the SMK advertises
lane operations 4-7 with upstream's `57 01 04` capability tag. No new command
number was invented. FM6 preset patches use target 3 of commands 68/69.

## Preset storage allocation and failure behavior

The project allocation stays **0xEB000..0xF3000** (four A/B project slots).
Presets use **0xF3000..0xF4000** and **0xFE000..0xFF000**, two noncontiguous
4 KB sectors outside the stock descriptors' named regions. Both complete,
matching stock Pro_016 reads show all FF in both sectors. The build requires
`--enable-presets --enable-saves` plus both matching stock backups and the
stock package. Without that flag the low-level driver cannot write there.

A 2,564-byte bank contains a format magic, eight upstream 192-byte preset
records and eight 128-byte FM6 patches. Each write replaces the inactive sector,
verifies its payload, and commits the CRC-protected generation header last.
An entire on-device STORE, including its FM6 patch, is one commit. Browser
imports use the existing two-command record-then-FM6 protocol: interruption
between those commands may leave the record with its earlier/default patch;
repeat the import in that case. No atomic multi-command claim is made.

Runtime requires the unit's JEDEC and calibration checks plus either blank
preset sectors or a valid committed bank. Unknown bytes fail closed. An
interrupted first-ever preset save with no valid copy disables preset storage
after restart; project storage and the application remain independent. A failed
replacement falls back to the other valid copy. The device advertises zero
preset slots when that allocation cannot be authorized.

`tools/smk37_preset_recovery.py` prepares an **offline** two-sector reset plan
from verified stock and a fresh complete current read. It preserves the app,
all projects and factory regions, rejects unexpected factory changes, and
never opens or writes a device. Resetting those sectors discards user presets.
The existing Test11 recovery writer has narrower, version-pinned bounds;
**do not run that old writer against Test14 or widen it blindly**. An actual
adapter reset needs a separately reviewed writer for exactly these two sectors
and a fresh readback. No preset reset was attempted in this round.

The USB firmware package still writes only **0x4000..0x9D000**, preserving both
project and preset storage. Bootloader, calibration and factory data remain
outside all new storage ranges. Private packages contain owner-specific stock
container data and are excluded from Git/public hosting.

## Validation and next hardware check

Passed:

- Combined SMK host suite: real synthesis, 37-key ownership, both knob banks,
  all eight encoder directions, buttons/faders/wheels/sustain, project migration
  and save bounds, MIDI/parser, and actual C replies parsed by the web client.
- New completion test: every selectable engine stored/reloaded, custom FM6
  patch restored, eight-slot bounds, held-note/playback save refusal, power-cut
  simulation at each erase/program stage and every partial commit-header byte,
  protected-byte comparison, first-save failure, undo/redo with automation,
  per-track source patterns and silent lanes, missing sources and malformed rows.
- Shared upstream UI regressions (including the tool hooks); browser protocol,
  library, keyboard and updater tests. FM-1 package tests remain skipped because
  no FM-1 release package is being built.
- 71 Python SMK tests, including the new bounded offline preset reset planner.
- Headless browser with real C-generated descriptors/replies over simulated MIDI:
  INSERT cards, 37-key C3 range, MIDI Learn, nudge, eight named preset slots and
  four song-lane selectors. No physical MIDI device is opened by this test.
- Target compiler/linker, memory limits and call-free RAM flash routines.

Private candidate: `build/smk37-webupdate-test14/smk37-test14-private.smkupdate`.
Application: **353824 bytes**; BSS end `0x1c161ac`, pool end `0x1c67924`.
Application SHA-256: `29ee1a1449af7ec2fd2dddd26fc1bff0284fe10d1fb76a8af3dd9689fa9b9e5e`.
Package SHA-256: `c05bc9449e501c00cc794b03465e79f4c4871eabf88edba9684951d4e715a5a2` (627712 bytes).

Before promotion: test normal USB installation/restart, old project load,
new bank mapping, volume/save/restart, a preset on each engine and custom FM6,
song transitions/silent lanes and stop restoration, tools/undo, and sustained
playing with the new effects. Hardware validation is still pending for Test14;
website Pro & Elite headings do not change the private Pro package checks.
Bluetooth, TRS, user-sample writes, complete backup and favorites remain outside
this candidate. No splash-screen or firmware branding changes were made here.
