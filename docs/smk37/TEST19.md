# Test19: settings, phrases and USB recording candidate

Local development only; no device flash or public release in this round.
Upstream remains Felucca 1.5.1, `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.
Hardware comparison: [CPU clocks, RAM and flash](HARDWARE_COMPARISON.md).

## Implemented

- PHRASES in the sequencer navigation group: K5 chooses the upstream factory
  phrase, `>` loads into the selected track, `<` returns. Existing edited steps
  require a second `>` within three seconds. The confirmation is tied to the
  phrase, track and step signature. Loading preserves the sound and clears
  automation as upstream does. Pattern undo now includes length/division/swing/
  gate as well as notes and automation.
- Settings add LINE/FLAT, LARGE (a two-by-two parameter layout), FX LATCH,
  SCOPE OUT/MIX, USB LEVEL MASTER/FIXED, CLICK, CLICK LEVEL, COUNT-IN and
  RESTORE LAST. List/action pages retain their normal layout. This is further
  FM-1 parity, not a claim that every FM-1 hold layer/setting has been ported.
- Existing preferences migrate without changing bank size or flash addresses.
  Test18's preferences restore their original eleven fields; a new marker at
  preference byte 30 identifies Test19's extra fields. Old data defaults to
  restore-last enabled. Saving still uses the verified A/B commit mechanism.
- MENU_DESC/SET commands 72/73 use upstream command numbers, setting IDs,
  descriptor encoding and result codes. Only implemented settings are advertised.
  HOME and ANIM adapt SMK's internal booleans to upstream wire meanings.
  Studio detects the capability, reads firmware descriptors, groups controls and
  writes through the same settings functions as the panel. Older firmware and
  Browser mode are not sent these requests.
- User slice move/split/join now has a main-loop adapter to upstream manual
  tables. The control interrupt queues the operation; decoder work runs with
  audio interrupts enabled. Selection/source changes cancel the commit. Built-in
  material remains read-only. Host tests supply synthetic user material.
  **The device still has no user-sample upload allocation, so manual editing is
  not yet reachable with real user samples. Slices are RAM-only at this stage.**
- Optional `--enable-usb-audio` build: shared upstream UAC1 16-bit stereo
  recording at 44.1/48 kHz. Synth output enters USB before the local metronome
  and before DAC conversion. FIXED affects USB independently of headphone level.
  TIMER5 services USB at 2 kHz, priority 4, between audio priority 3 and scanner
  priority 5. Main does not poll SIE concurrently. USB reset/retry stays in main
  while `usb.up` is false, avoiding a 25 ms reset delay inside the audio-preempting
  interrupt. Update/reset/fault paths stop TIMER5 before detaching/handoff.
  The default build still has USB audio off. The private Test19 candidate uses it.

## Work still required from the six-part request

1. Navigation/settings: remaining FM-1 layers and settings, full home/animation
   variants, and device validation of the new layouts. The browser phrase list
   currently contains factory phrases, not patterns from saved user presets.
2. Slice editing: connect actual uploaded samples and persistent manual markers
   after storage migration. The tested edit algorithm alone does not complete it.
3. Factory phrase browser: implemented; physical navigation/playback check remains.
4. Storage expansion: [allocation/migration proposal](STORAGE_EXPANSION.md), not
   enabled. Still eight device presets; no sample upload, writable FM6 voice bank
   or complete archive support is advertised.
5. USB audio: compiled and host-tested; host enumeration, recording, dropouts under
   maximum load, concurrent MIDI/editor activity and restart/update need hardware.
6. Physical MIDI: [stock UART evidence](PHYSICAL_MIDI.md); no guessed GPIO enabled.
   The SMK's documented jack is MIDI OUT, unlike FM-1's MIDI IN.

Bluetooth remains excluded. Existing boot, calibration, project and preset
regions are retained. No expanded-storage erase/program range is enabled.

## Validation and artifacts

`tools/test_smk37_application.sh` includes Test19 confirmation/undo, preference
migration, menu protocol and manual slice tests alongside existing MIDI, DSP,
storage, editor and renderer tests. The renderer exercises all pages/palettes/
fonts and both style/large choices. `web/test_smk37_menu.mjs` parses actual C
replies using the shipping Studio parser. `tests/uac_test.c` uses 128-frame halves
with CDC off, matching SMK: descriptors, ring flow, resampling and rate switches.

Private target and hashes: `build/smk37-webupdate-test19/manifest.json`.
Build logs: `build/test19-build.log`, `build/test19-host.log`,
`build/test19-uac.log`. No host result substitutes for an SMK recording test.

The target build, complete SMK host suite, C-to-JS menu tests and UAC tests pass.
Rendered PHRASES, SETTINGS, LARGE and LINE screens were visually inspected.
Headless Chrome also renders actual Test19 menu replies as nineteen controls
in DISPLAY/CONTROL/AUDIO/SYSTEM groups without script errors, using an injected
device model. This checks rendering, not a live MIDI connection or device writes.
The updater's package-integrity, model-isolation, bootstrap, disconnect and
cleanup tests also pass. No hardware was written during these checks.

- Application: 400,784 bytes; SHA-256
  `685c7a9ee440d61b21059bb6831de516272f79c62e24463c05642e129fd83490`.
- Private package: 627,712 bytes; SHA-256
  `42e67163027142e35df8b6a7bbe0bfcbf4447a07ca64b27e3dde5eab7b705cfe`.
- Product identity `SMK-37_919`, version `SMK37 TEST19 USB`.
- BSS end `0x1c1790c`, pool end `0x1c685ac`.

Package: `build/smk37-webupdate-test19/smk37-test19-private.smkupdate`.
It is tied to the owner's backed-up Pro and must remain private. At the owner's
request, localhost:8770 now offers Test19; the served package hash matches the
artifact above. Test18 remains available on disk. This staging did not flash
the device. First checks: normal USB enumeration/MIDI, existing keys/audio and
sustain, settings/save/reboot, phrases/undo, then USB recording and update/restart.

Subsequent device evidence: Windows enumerated `Line (Felucca)` with two input
channels. Two eight-second recordings at requested 48,000/44,100 Hz captured
signal in both channels with no full-scale clipping. Windows may resample;
native USB rate switching and long-run stability are not established. See
[Test20's validation record](TEST20.md) for the saved recordings and tool.
