# SMK Test21: user samples, physical MIDI output and settings

Private candidate on `integrate/smk37-upstream-20261010`, based on Felucca
1.5.1 `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`. Includes the Test20
master-volume odometer, saved-preset phrases, step preview and tuning.
No new upstream import, push, PR, public deployment or device flash this round.

## Local testing

- Test21 installer: `http://localhost:8771/`; Studio: `/editor.html`.
- Test19 remains served at `http://localhost:8770/`.
- Both servers bind only to loopback. Test21 uses the owner's paired stock
  backup and calibration evidence; the package must not be redistributed.
- The owner reports Test21 installed and successful physical TRS output to an
  FM-1 (see hardware follow-up below). New sample allocation still needs
  real-flash validation; host tests do not establish that result.

## User samples

Three 80 KiB slots use the upstream FSMP format and editor commands 11–15.
Physical addresses are `0x9d000`, `0xc2000`, and `0xd6000`; they are not the
FM-1's contiguous allocation. Factory sounds remain read-only.

The private build flag `--enable-samples` requires saves, presets, two matching
validated stock reads, and an erased `0xbf000` journal sector in those reads.
Boot only reads the ownership journal. It never initializes or erases samples.
Until initialization commits, the editor advertises zero sample slots and XIP
sample access uses an empty header. Studio now explains that state in all eight
languages instead of leaving the Samples area blank.

On the device, hold GLO to open NAVIGATE, choose SYSTEM, then SAMPLE STORAGE.
Press `>` to review the replacement notice and `>` again within five seconds to
initialize; `<` cancels. Stop playback and release keys/pedal first. This action
replaces stock sample data, so retain the private stock backup. It preserves the
existing eight presets, four projects, application, radio/trim and OEM identity.
Reconnect Studio after the display says READY; Samples then exposes all three
slots. Upload a short WAV, select USR1/2/3 in SAMPLE, and check playback and restart.

Initialization verifies stock CRCs before its first intent commit. A unit-bound
intent and completion marker make interrupted initialization resumable. Torn
markers must match the exact expected bit pattern; unknown data fails closed.
An already completed initialization performs no writes. Only sample header
sectors are initially erased; upload erases subsequent data sectors as needed.

Uploads require BEGIN, sequential 256-byte writes, matching readback and CRC,
and validated zone bounds. Duplicate data packets are idempotent. The header
tail is written/verified before the magic is published last. After a power
reset, an incomplete sample requires a fresh BEGIN. Sample XIP mapping starts
at the SMK application boundary `0x9d000`, with raw/XIP readback comparison.

Manual slice markers reuse upstream's sample-bound SLM1 record. Edits save after
leaving SLICES with playback stopped and keys released. Samples must leave the
last 4 KiB sector free: at most 77,312 ADPCM data bytes. Longer samples remain
playable, but manual markers stay in RAM. A torn marker save loses the markers,
not the sample. Replacing a sample invalidates markers for different material.

## Physical MIDI OUT

`--enable-trs` builds a stock-derived UART1 TX adapter. It requires the existing
2 kHz USB service timer. The device always starts with TRS OUT OFF, and does not
touch that UART/pin until the user enables it. No Bluetooth changes are included.

Open the MIDI page (NAVIGATE → SYSTEM → MIDI), turn physical K7 clockwise to
enable TRS OUT; counterclockwise disables it. USB MIDI stays available. TX shows
bytes whose UART DMA transfer completed, not confirmation that a receiver heard
them. Use a Type-A TRS MIDI cable/adapter and a MIDI input receiver to validate.

Local keys/pads, their chord note ownership, sustain, pitch bend and mod wheel
duplicate the existing USB output events. This is not USB-input MIDI Thru and
does not add sequencer/master-clock output that the existing event producer
does not generate. The queue can encode clock/transport events if a future
producer supplies them; this alone is not clock-output support.

Disabling is implemented to drain sustain-off/all-sound-off/all-notes-off on every
channel before stopping, but the physical FM-1 test failed: a held note continued
after TRS OUT was switched OFF with the cable connected. This remains an open
issue, deferred at the owner's request. Queue overflow finishes its in-flight message before the same panic.
A stalled UART times out without blocking audio; firmware-update handoff stops
UART DMA before overwriting application RAM. Physical pin polarity, signal
levels and external receiver behavior remain to be tested.

## UI and compatibility

- Speaker EQ: FLAT / LOWCUT / BASS+, backed by the existing shared DSP filter.
  The name follows FM-1; this does not imply the SMK has an internal speaker.
- Studio MENU descriptors now include Speaker EQ (upstream ID 9), MIDI IN
  (21), and LED level (4). Existing IDs and ordering are preserved.
- MIDI IN still edits the project's channel-routing value. Speaker EQ and LED
  level save with preferences. Preference marker `0x21` imports older layouts;
  Test20's reserved byte is not interpreted as EQ.
- TRS OUT is device-local and resets OFF; no new editor command or upstream
  setting ID was commandeered for it.
- Eight device presets remain active. The prepared 32-preset object backend,
  standalone FM6 user bank and complete archives are not runtime-integrated yet.
  Chord-entry preference parity also remains; FM-1 keyboard LED behavior has no
  equivalent LED hardware under the SMK piano keys.

## Validation

The full SMK host suite passed, including the shared expressive MIDI/clock and
UART parser regressions, ASan integration, editor wire parsing and all existing
display tests. New tests cover ownership/stock mismatch, every initialization
write failure, every partial ownership marker, protected ranges, all three
sample slots, duplicate/out-of-order packets, CRC failures, interrupted sample
header publication, reload/erase, marker persistence/corruption, and confirmation.
UART register/queue tests cover OFF startup, mux masks, no enabled UART IRQ,
DMA buffer ownership, completion accounting, disable panic and timeout.

The real C firmware's 22 MENU descriptors and sample replies decode in the
shipping Studio parser. Headless Chrome rendered 23 selects plus numeric TUNE
in five groups, with sample setup guidance and no page errors. Device screens
were rendered and visually checked. All eight locale catalogs pass. Updater
tests pass; target compilation has no warnings and validates call-free RAM
flash routines. The exact private package served on port 8771 matches its SHA.

Longer audio testing used the already installed Test19: 90 seconds at each
requested Windows capture rate, recording only `Line (Felucca)`:

| Requested rate | Stereo frames | Peak L/R | RMS L/R | Clipped samples |
| --- | ---: | --- | --- | ---: |
| 48,000 Hz | 4,320,000 | 1979 / 1977 | −43.09 / −43.09 dBFS | 0 |
| 44,100 Hz | 3,969,000 | 1772 / 1772 | −43.26 / −43.26 dBFS | 0 |

Every one-second window contained signal on both channels; the longest exact
zero run was 0.0002 seconds. Private WAVs and JSON reports are under
`build/audio-check/test19-long-*`. This is three minutes of continuous-signal
capture, not a claim of an hours-long soak, native-rate switching, bit-perfect
transport, or artifact-free listening. WinMM may resample. Repeat after Test21
installation, including UI activity and physical MIDI output.

## Hardware follow-up

On 2026-10-10, the owner identified the SMK-37 build as Test21 and reported
"Totally works" when testing its TRS MIDI OUT into an FM-1. This confirms basic
end-to-end note reception and audible playback for that setup. Receiver firmware
and cable details were not recorded. Follow-up results:

- Chords, sustain and pitch bend: owner reports working.
- Mod wheel: inconclusive; audible effect was hard to distinguish.
- TRS OUT returns to OFF after a power cycle: confirmed by the owner. This is
  intentional in Test21; reviewing persistence is deferred in WORK_QUEUE.md.
- TRS OUT switched OFF while holding a note, cable connected: FAILED. The FM-1
  continued sounding. This was explicitly clarified as the software toggle,
  not cable removal. Root cause is not established; no firmware fix was made.
  The owner requested documenting and deferring this issue to continue testing.
  Until fixed, release keys and sustain and let the receiver's notes stop before
  disabling TRS. This is a workaround, not a verified shutdown fix.

The owner subsequently initialized sample storage after the paired stock backups
were revalidated (identical reads, exact Pro016 firmware match and 37 valid key
calibration curves). The device displayed three user slots and Studio reported
three slots after reconnecting. The owner then reported uploading the generated
one-second SMKTEST_C4.wav to USR1 and confirmed playback when prompted to select
SAMPLE / USR1. The post-restart check was inconclusive: TRS needed re-enabling,
and the listening path was not confirmed as the SMK's own audio output. This is
not evidence that the sample was erased. Direct SMK headphone playback and
Studio slot inspection after restart remain pending, deferred at the owner's
request. TRS MIDI does not transmit the uploaded sample's audio to the FM-1.

## Artifact identity

Commit-checkpoint validation: updater and all eight locale checks passed again,
as did the offline storage-backup regression and staged whitespace check. A
browser-audio regression retry against port 8771 did not complete: its error-path
screenshot timed out in Playwright, masking the original failure. This retry is
not a passing browser-audio result and needs investigation in a later round.
The checkpoint does not publish or flash a firmware release.

- Product `SMK-37_921`, version `SMK37 TEST21 USB`.
- Application: 413,220 bytes; SHA-256
  `dbdea3cfca1473dba6cb93bba604419df20b5ef35a3f77beb7fd88304b2c03b3`.
- Private update: 627,712 bytes; SHA-256
  `b16bd1326fd0bf4162115c616ad298264e5928d248f3199ff738370771eba886`.
- `_bss_end = 0x1c17f90`, `_pool_end = 0x1c685ac`, within the existing linker
  limits (`0x1c1a000` and `0x1c74000`).
- Local output: `build/smk37-webupdate-test21/`; logs:
  `build/test21-build.log`, `build/test21-host.log`.
