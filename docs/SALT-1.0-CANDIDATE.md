# Felucca [Salt] 1.0.1-salt1rc1

This is a software-tested release candidate based on upstream v1.0.1 (`20c275e`).
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
8. Upstream 1.0.1 DIM/INV lighting, idle glow, green PLAY and sequencer/ARP key
   feedback; combined settings preserve Salt FONT/MIDI MON/HOLD.
9. QNT SEQ: non-destructive scale mapping during melodic sequence playback, with
   duplicate-pitch and note-off handling. Studio mock descriptors and browser
   DSP are rebuilt from the same source.

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
The 1.0.1 update follows with [lighting #34](https://github.com/ChanceTheMaker/Felucca/pull/34)
and [QNT SEQ #35](https://github.com/ChanceTheMaker/Felucca/pull/35), then the
version/build/docs branch `feat/1.0.1-rc1-candidate`.

## Software validation

- Firmware package builds for `FM-1_910` with the open Felucca loader.
- Complete host regression suite passes, including 87 golden sound renders,
  target instruction budgets, MIDI/UART recovery, USB audio, UI, persistence,
  settings migration, song/motion, FM6 conversion, and simulated installation.
- UI renderer checks 115 screens across 21 palettes without layout errors.
- Editor/package/update tests and backup tests pass against simulated devices.
- WASM audio tests cover all 13 engines and 69 presets, imported FM6 patches,
  sustain, panic, filters, and 44.1/48 kHz output.
- Chrome browser audio checks cover mode changes, audible keyboard output,
  the live scope, MIDI isolation, restart, and mobile layout.
- All eight translation catalogs have matching keys and placeholders.
- Chrome checks cover twelve themes, light/dark modes, all eight editor tabs,
  custom controls, keyboard docking, and desktop/mobile layouts. The earlier
  website round also checked navigation and the guide in all eight languages
  at four viewport widths; those checks are not repeated for this firmware update.

The optional DaisySP reference comparison was skipped because its checkout is
absent. Host instruction counters are unavailable on this machine; target
disassembly budgets pass. Actual official V15 package restoration was not tested
because the vendor package is not provided. Simulated updater tests do not
replace installation on a real FM-1.

## Build and local preview

Follow [BUILDING.md](../BUILDING.md) to install the toolchain and SDK, then run:

```sh
python3 tools/build.py --release 1.0.1-salt1rc1
python3 tools/build_browser_audio.py --zig /path/to/zig
python3 web/make_site.py build/felucca-1.0.1-salt1rc1.fwsc 1.0.1-salt1rc1 build/site
# Serve build/site using your preferred local HTTP server.
```

This workspace uses one preview server on port 8768:

- Candidate: `http://localhost:8768/rc1/webapp/installer/`
- Candidate Studio: `http://localhost:8768/rc1/webapp/editor/`
- Stable: `http://localhost:8768/webapp/installer/`

Port 8871 is retired. The candidate installer visibly identifies its version.

## Device validation before release

- Back up an FM-1, install the candidate, and verify boot and all panel controls.
- Exercise rapid TRS note-on/off, chords, sustain, disconnect and reconnect,
  MIDI clock/transport, and USB audio capture.
- Verify DIM/INV lighting, green PLAY, sequencer/ARP key feedback and the scrolling menu.
- Test QNT SEQ during root/scale changes, sustained/tied notes and drum/slice exclusion.
- Confirm palette/font/monitor/LED choices persist across power cycles, including
  changes queued during playback, and test favorites from both panel and Studio.
- Connect Studio, verify FM6, SONG, motion, samples and backup/restore, then
  reconnect after a firmware change without using stale capabilities.
- Test recovery with the owner's official V15 file before describing that path
  as hardware-validated. Preserve a known-good backup and Salt14 package.

Only after these checks should this candidate replace the default installer
package or be labeled a completed hardware release.
