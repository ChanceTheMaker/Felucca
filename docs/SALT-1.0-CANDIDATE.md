# Felucca [Salt] 1.0-salt1rc1

This is a software-tested release candidate based on upstream v1.0 (`727f272`).
It does not replace the published 0.9-salt14 firmware until device validation is
complete. The experimental Bluetooth branch is excluded.

## Feature boundaries

1. Upstream 1.0 firmware, tools, tests, assets, and license foundation.
2. Content-driven TRS receive ring, retaining overflow and timestamp handling.
3. Salt display: 21 palettes, regular/bold font, Events/Notes MIDI monitor, and
   tagged settings persistence compatible with upstream HOLD preferences.
4. Studio: 91-parameter layout, FM6 patch editing, SONG, motion, recording/trim,
   full backup/restore, and session guards. SONG stays at 33; preferences and
   favorites use 34–38. The unshipped Salt command-33 layout is not supported.
5. Installer: current package validation and backup-first official V15 recovery.
6. Browser DSP: 13 selectable engines, 69 presets, imported FM6 patches, and
   the existing one-sound keyboard, sustain, panic, and scope workflow.
7. Display regression coverage and the website integration/documentation.

Each boundary is kept in a separate feature PR. Website visual changes also have
their own PRs and can ship with Salt14 while the firmware candidate is tested.

Review the stack in order: [foundation #16](https://github.com/ChanceTheMaker/Felucca/pull/16),
[TRS #17](https://github.com/ChanceTheMaker/Felucca/pull/17),
[display #18](https://github.com/ChanceTheMaker/Felucca/pull/18),
[Studio #24](https://github.com/ChanceTheMaker/Felucca/pull/24),
[installer #25](https://github.com/ChanceTheMaker/Felucca/pull/25),
[browser DSP #26](https://github.com/ChanceTheMaker/Felucca/pull/26),
[regressions #27](https://github.com/ChanceTheMaker/Felucca/pull/27), and
[website integration #28](https://github.com/ChanceTheMaker/Felucca/pull/28).

## Software validation

- Firmware package builds for `FM-1_910` with the open Felucca loader.
- Complete host regression suite passes, including 87 golden sound renders,
  target instruction budgets, MIDI/UART recovery, USB audio, UI, persistence,
  settings migration, song/motion, FM6 conversion, and simulated installation.
- UI renderer checks 111 screens across 21 palettes without layout errors.
- Editor/package/update tests and backup tests pass against simulated devices.
- WASM audio tests cover all 13 engines and 69 presets, imported FM6 patches,
  sustain, panic, filters, and 44.1/48 kHz output.
- Chrome browser audio checks cover mode changes, audible keyboard output,
  the live scope, MIDI isolation, restart, and mobile layout.
- All eight translation catalogs have matching keys and placeholders.
- Chrome checks pass for twelve themes, light/dark modes, all eight editor tabs,
  custom controls, keyboard docking, and desktop/mobile layouts. Navigation and
  the guide pass in all eight languages at four viewport widths.

The optional DaisySP reference comparison was skipped because its checkout is
absent. Host instruction counters are unavailable on this machine; target
disassembly budgets pass. Actual official V15 package restoration was not tested
because the vendor package is not provided. Simulated updater tests do not
replace installation on a real FM-1.

## Build and local preview

Follow [BUILDING.md](../BUILDING.md) to install the toolchain and SDK, then run:

```sh
python3 tools/build.py --release 1.0-salt1rc1
python3 tools/build_browser_audio.py --zig /path/to/zig
python3 web/make_site.py build/felucca-1.0-salt1rc1.fwsc 1.0-salt1rc1 build/site
python3 -m http.server 8871 --bind 127.0.0.1 --directory build/site
```

Open `http://localhost:8871/webapp/installer/` or `/webapp/editor/`. The candidate
installer visibly identifies itself. Port 8768 remains the stable website preview.

## Device validation before release

- Back up an FM-1, install the candidate, and verify boot and all panel controls.
- Exercise rapid TRS note-on/off, chords, sustain, disconnect and reconnect,
  MIDI clock/transport, and USB audio capture.
- Confirm palette/font/monitor choices persist across power cycles, including
  changes queued during playback, and test favorites from both panel and Studio.
- Connect Studio, verify FM6, SONG, motion, samples and backup/restore, then
  reconnect after a firmware change without using stale capabilities.
- Test recovery with the owner's official V15 file before describing that path
  as hardware-validated. Preserve a known-good backup and Salt14 package.

Only after these checks should this candidate replace the default installer
package or be labeled a completed hardware release.
