# SMK sound controls (private Test08 candidate)

The application now includes a sound interface rather than only input counters.
This is implemented in `firmware/src/smk37_ui.c`, with scanner event queuing and
LCD presentation in `smk37_app.c`. The owner has tested piano, pads, wheels,
faders and page editing in Test06/07. Test08 button actions, normal status
lighting and corrected encoder polarity await hardware validation. The private
unit-specific recovery package requires explicit approval before installation.

## Provisional electrical assignments

| Electrical encoder index | Function |
| --- | --- |
| 1 | Selected track, wrapping through four tracks |
| 2 | Engine, skipping the retired DIGITAL slot |
| 3 | Factory preset, skipping sample preset aliases |
| 4 | Page, including the input diagnostic page |
| 5–8 | The four displayed parameter values |

These are scan-table indices, not confirmed left-to-right panel positions.
Four quadrature transitions produce one edit. Test08 corrects polarity to match
the stock signed event after the owner reported clockwise decrementing in Test07.
All-eight physical direction and detent resolution still need verification.
The earlier proposed panel mapping in the
preparation document is not a claim that these electrical indices match it.

The 24 sound pages cover mix, engine controls, envelope and its destinations,
LFO and its destinations, effects, delay, reverb, chorus, voice options, scale,
chords, arpeggiator, slicer, tempo, all four modulation slots, and sequencer. Parameter
descriptors, bounds, enum aliases and factory sounds reuse the shared synth.
Three diagnostic pages retain analog, button/encoder and lighting checks.
No persistent save, radio or TRS support is enabled.

## Test08 buttons and feedback

- KNOB BANK swaps navigation and parameter editing between E1-4 and E5-8.
  Diagnostics keep E4 for page navigation regardless of bank.
- FADER BANK switches the four faders between part volumes and the selected
  part's attack/decay/sustain/release. Switching banks re-anchors physical values.
- PATCH opens mix and makes < / > select factory presets. PARA, FX, GLOBE and
  SEQ open engine, effects, tempo and sequencer pages. Outside PATCH, < / >
  select sound pages; encoder navigation also reaches diagnostics.
- OCT- / OCT+ transpose new piano notes by octaves, limited to -3 through +3.
  Held notes release their original pitches even after octave/scale changes.
- ARP, NOTE REPEAT, SCALE and CHORD toggle the corresponding per-part functions.
  Chord behavior follows the selected preset's existing mono/poly voice setting.
- PLAY starts/stops the sequencer; stopping disarms recording. REC arms/disarms
  the selected part and starts playback when needed. Recorded patterns are RAM
  only and disappear on power-off in Test08.
- BT displays "BLUETOOTH NOT AVAILABLE"; no radio code is added.
- Indicator lights show selected functions, bank and transport state. Active
  pads flash blue at the existing low brightness limit. Diagnostics suppress
  normal actions/lights; leaving LIGHT CHECK restores normal status lighting.

The owner subsequently confirmed all eight knob directions, octave and function
buttons, knob-bank swapping, fader1 attack in the envelope bank, two pads playing
and lighting together, and RAM recording/disarm/stop/replay. See CONTROL_TRACE.md
for the individual confirmations. The other three envelope faders and prolonged
timing/current behavior remain unmeasured.

## Test09 candidate: velocity, editor, step editing and storage/pedal probes

Private candidate only; Test08 remains the last hardware-verified build. No
publication or device write is implied by the following host-tested additions.

- Piano velocity uses measured scanner-frame duration with normal, soft, hard
  and fixed100 curves. These are generic curves, not the recovered factory trim.
  GLOBE cycles TEMPO, CONTROLS and PROJECTS; CONTROLS shows the last velocity.
  With KNOB BANK off, E5 selects the curve and E6 selects pad bank1/2. Bank2 adds
  16 to the bank1 MIDI notes; this is a Salt assignment, not a stock mapping claim.
- SEQ alternates SEQUENCER and STEP EDIT. Pads toggle the visible group of16
  steps; < and > move between the four groups. E5 selects a step, E6 changes its
  note, E7 its velocity, E8 NOTE/TIE/REST. Lit pads identify active steps. New
  entries use the last played note. Sequence edits remain RAM-only in Test09.
- USB adds the existing editor protocol for sounds, presets, four tracks and
  steps, plus selected-track change notifications. No new command IDs or OTA.
  The local editor adapts its hardware keyboard span to C3-C6, 37 keys. Samples
  and on-device user presets are reported unavailable; project save is disabled.
- Sustain ownership/retrigger handling and outgoing CC64 are host-tested. The
  physical pedal input is NOT assigned yet. PEDAL CHECK (after LIGHT CHECK via
  E4) watches GPIO changes, ADC23/24 and matrix contact changes without driving
  new pins. Enter with the pedal plugged in and untouched, then press/release
  only the pedal. A photo before/held/after identifies the next mapping step.
- Four persistent project slots have A/B copies, CRCs and a commit header written
  last. A snapshot contains all four sounds/sequences plus global settings,
  octave, velocity and banks. Host tests cover restore/latest-slot selection,
  interrupted writes, partial headers and damaged-new-copy fallback.
  **SMK37_ENABLE_SAVES is zero. No runtime erase/program is permitted.**
  PROJECTS > runs the read-only check: JEDEC856014, the private trim checksum,
  and every byte blank in 0xEB000-0xF3000. Success says READ OK / SAVES DISABLED.
  Any mismatch fails closed. The gap is a candidate allocation inferred from
  the stock file map and two identical raw backups, not a vendor guarantee.

Before enabling persistent writes: verify these reads physically, define a
tested recovery/clear path for the candidate save range (the current adapter
writer permits application sectors only), handle owned records on subsequent
boots, and test a save/load/power cycle. Do not expand the recovery writer's
range as an incidental change. Test09 neither auto-loads nor probes at boot.

USB hardware connection, velocity feel, step-grid behavior and pedal mapping
are the next device checks. Bluetooth and TRS remain outside this build.

## Test09 results and Test10 SAVE candidate

The owner confirmed Test09 normal boot/piano sound; velocity49 soft through121
hard; ADC23 released871/pressed0/back871; READ OK / SAVES DISABLED; editor
connection and two-way parameter changes; lit step entry/clear/playback; and
pad bank2 changing pitch without stuck notes. See CONTROL_TRACE.md. These are
qualitative checks on one unit, not complete timing or endurance certification.

Test10 connects ADC23 to piano sustain and outgoing CC64. It requires three
fresh stable conversions, uses hysteresis, and waits for a released pedal before
arming at startup. Leave the pedal polarity switch in the position used for the
confirmed readings. A held pedal at startup must be released before use.

The private `--enable-saves` build identifies as SMK37 TEST10 SAVE. Builds without
that flag still compile out writes. The save-enabled candidate checks JEDEC,
private trim checksum and allocation ownership; it accepts blank slots or a
valid A/B committed copy, and refuses unfamiliar data without erasing it. An
interrupted replacement can use its older copy. A failed first-ever save with no
committed copy requires recovery clearing; normal sound operation still boots.

PROJECTS E5 selects1-4 (KNOB BANK off); > saves and < loads. Save is explicit,
never automatic, and requires stopped transport, no recording, no local or
USB-held notes, and pedal up. On startup, the newest valid saved project is
loaded with transport stopped. It restores four sounds/sequences, globals,
octave, velocity curve and banks. Flash work may briefly interrupt audio/input;
keep hands off controls during the save test. The web editor enables project
Save only for the SAVE variant, preserving the Test09/read-only disablement.

The separate save-only XIAO recovery build and clearer cover0xEB000..0xF3000
only. The existing application writer still rejects changes there. Clear test
saves with that separate tool before returning to a byte-identical older app or
stock restore; every byte outside the save window must match a pinned reference.
The next hardware sequence must verify the save-only adapter capability/read
path before installing Test10 SAVE with the application-writer adapter image.
Then verify sustain, save/load, reboot restore and save-area recovery. These
Test10 hardware checks remain pending; no new image is flashed by building it.

## Concurrency and sound changes

Scanner interrupts enqueue encoder transitions independently of key events.
The audio consumer takes at most one encoder transition per half-buffer, then
consumes keys. It performs all sound mutations serially with the DSP. A lost
encoder queue resets partial detents, without discarding key releases. Preset
changes use the shared panic path and preserve track mix/arp/chord settings.
Display code copies parameter values with interrupts masked, then formats and
transfers pixels with interrupts restored. Changing a page clears the content
area so diagnostic and sound layouts do not leave old labels behind.

## Offline validation

`tools/test_smk37_application.sh` tests the actual synth with AddressSanitizer:
37-key note ownership, track changes while held, encoder reversals/detents,
every selectable engine and factory preset with active voices, preset changes
while held, parameter bounds, empty controls, and page wrapping. The existing
FM-1 chord and MIDI/clock regression suites also run. Test08 adds button actions,
RAM record/replay, held-note release across octave/scale/chord changes, bank
switches, and real contact-frame-to-parameter direction tests for all eight knobs.

The processor build is produced by `tools/build_smk37.py` under
`build/smk37-integration/`. Its manifest records the current size, checksum,
disabled flash writes and outstanding physical checks. Offline success does
not establish audio deadlines, encoder mapping, calibrated velocity, startup,
display operation, safe installation or forced recovery on the SMK hardware.
