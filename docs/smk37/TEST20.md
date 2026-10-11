# Test20: master odometer, saved phrases and port foundations

Based on Felucca 1.5.1 `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.
Private candidate prepared; no flash, push or publication. Localhost:8770 stays
on Test19 while the owner tests its USB recording.

## Device and Studio changes

- Master volume now rolls its percentage with upstream's digit animation:
  direction, ease-out, fast-turn snap and digit-width snap. It has its own
  animation state, uses the active palette/font, and respects ANIMATION OFF.
  Volume changes immediately; animation only affects the footer display.
- PHRASES includes saved user-preset patterns after the factory list. Empty
  patterns are omitted. A user phrase brings its first sixteen steps, length,
  division, swing and gate, including drum-grid accents; sound stays unchanged.
  Existing overwrite confirmation and pattern undo apply. Saving or erasing a
  preset invalidates any pending phrase confirmation.
- STEP PREVIEW in Settings auditions the cursor step when stopped, using the
  shared sequencer's audition path. Playing, recording, count-in and an armed
  or running song chain suppress it. Pads select/audition on Step Detail;
  the step cursor knob and page arrows audition on Step Edit.
- Studio receives STEP PREVIEW (upstream MENU ID 19) and TUNE (ID 22, -50..50 ct).
  Tuning remains a project value; setting it does not silently save a project.
  Test20's preference marker restores all 21 settings. Test19 banks restore
  their twenty fields, older Test18 banks their eleven; new preview defaults OFF
  on cold boot. Downgrading to Test19 retains the bank but that older firmware
  recognizes only its legacy eleven preference fields from the newer marker.

## Four-area progress

1. **UI:** the items above are implemented and host-tested. Remaining full
   parity includes other FM-1 hold/performance layers, applicable settings and
   header/Home behaviors. SMK has no illuminated piano keys or built-in speaker,
   so those FM-1 hardware controls cannot be copied literally.
2. **Storage:** the larger atomic object backend, eight-to-32 preset conversion
   and offline stock-region export/restore now exist and pass tests. Runtime
   migration/ownership and sample/FM6/archive adapters remain. Eight slots and
   the existing flash gate remain active. See [storage details](STORAGE_EXPANSION.md).
3. **USB audio:** the owner showed Windows' Line — Felucca input during Test19
   testing. The Windows capture tool independently enumerated it as two channels
   and recorded eight seconds each at requested 48,000 and 44,100 Hz. Both files
   contain sound on both channels, with zero PCM full-scale clipped samples.
   These WinMM recordings may be resampled by Windows: they do not establish
   native endpoint-rate switching, listening quality or long-run load stability.
4. **MIDI OUT:** identified the stock UART1 TX DMA routine and implemented the
   bounded byte queue/recovery behavior in host tests. Board/mux verification
   and physical receiving-interface tests remain. No UART is enabled by Test20.
   See [physical MIDI evidence](PHYSICAL_MIDI.md).

Bluetooth remains excluded. No public installer or published binary changes.

## Validation

Complete SMK host suite passes, including the renderer, existing save/restore
and wire integration, new saved-phrase/preview/tuning tests, multi-sector
interruption tests and MIDI transmit-queue tests. Master-volume rendering was
visually inspected and tested for immediate value change, settling, disabled
animation and a change in digit width. Target build passes with no compiler
warnings. Python backup tests cover roundtrip, wrong-unit and corrupt archives.

Private recordings: `build/audio-check/test19-48000.wav` and
`build/audio-check/test19-44100.wav`, with adjacent measurement JSON files.
Record/analyze with `tools/smk37_usb_audio_check.py`; it requires an explicitly
named Felucca input and never falls back to a microphone.

Application: 401,940 bytes; SHA-256
`2ae81f19fd4bd9dedc25c3151a2cb875f1a06eafe5eb9782f6c585552b0f421e`.
Private package: 627,712 bytes; SHA-256
`edfe58586cbb035f3abe238ff288d6cbcb22ab868c00716c03c62ae809150403`.
Identity: `SMK-37_920` / `SMK37 TEST20 USB`.
Artifacts: `build/smk37-webupdate-test20/`; logs `build/test20-host.log` and
`build/test20-build.log`. Private stock data remains under ignored `build/`.
