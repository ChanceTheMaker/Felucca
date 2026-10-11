# SMK-37 Pro stock baseline, October 7, 2026

The owner reports successfully installing SMK-37 Pro 016, booting, and hearing
the internal synth through its output. PARA/K4 changes its volume. This verifies
the ordinary stock update path, not forced recovery from a broken application.

After supplying the internal photographs and reassembling the unit, the owner
reports that it is working fine. This is an owner-reported stock-operation check,
not a repeated capture of every control or a custom-firmware/recovery test.

The installed file supplied by the owner matches our previously inspected
705,252-byte Pro 016 package byte for byte:
`2d98ce72530e71d617384963423820536f094501299e2cfec0a8d3c3fb6bcfb0`.

Captures are local under `build/arrival/2026-10-07/`. They contain received
MIDI only; the recorder does not open output ports. Raw captures, device photos,
vendor images and analysis-only ELF containers are not release artifacts.
Timed recordings must have an `end` record before being called complete;
`*-report-snapshot.json` files may describe a still-running recording.

## USB and settings

Windows exposed three MIDI inputs and three outputs, plus audio playback and
recording endpoints named SMK-37 Pro. The initial PnP observation was
VID `4353`, PID `B64B`, MIDI interface `03`, audio interface `00`.
That observation preceded the owner's reported 016 update; do not present it
as a verified post-update descriptor or overwrite the static descriptor evidence.

### Reconnected on October 8

After the owner reconnected the unit, Windows MIDI enumeration again showed
all three input names listed below. A fresh read-only PnP snapshot at
2026-10-08T15:58:45Z reports VID `4353`, PID `B64B`, with the composite device,
audio interface `00`, and MIDI interface `03` all having status `OK`. Their
parent relationships match the same composite device. The snapshot is local at
`build/arrival/2026-10-08/usb-reconnected-01.json`.

This is a post-reassembly, post-reported-update observation; the firmware
version was not queried again. In particular, the runtime PID is **B64B**,
not the **344B** template found in the Pro016 application. The local AC79 SDK's
`apps/common/usb/device/descriptor.c:get_device_descriptor` modifies PID bytes
after copying its descriptor template; that is a possible explanation, not a
confirmed trace of this SMK's runtime path. Do not use the static template PID
as a strict device allowlist or infer the installed version from the PID.

The owner confirms that the XIAO RP2040 and USB breakout have not arrived yet.
The ordinary stock MIDI/audio connection is available; independent forced
recovery and calibration backup remain unverified. No output port was opened,
and no MIDI/SysEx, reset, update or flash command was sent during this check.

During the later captures the photographed overview showed Preset 4, Pad Bank 1,
keys on channel 1, pads on channel 10, and scale/chord/arpeggiator/note repeat off.
Do not assume these are factory defaults or that the same settings applied to
every earlier recording. Interpret the numbered ports below as the input names
observed in this session, not a guaranteed portable port order:

1. `SMK-37 Pro Midi 0`
2. `MIDIIN2 (SMK-37 Pro Midi) 1`
3. `MIDIIN3 (SMK-37 Pro Midi) 2`

## Measured controls

| Control / state | Received behavior | Evidence file stem |
| --- | --- | --- |
| 37 keys, isolated pass | Channel 1, notes 48-84, 37 presses and 37 matching releases; velocities 66-104 | `keys-isolated-01` |
| Pitch wheel | Channel 4 pitch bend, 0-16383; last released value 8256, not nominal MIDI center 8192 | `wheels-isolated-01` |
| Modulation wheel | Channel 3 CC1, 0-127, ending at 0 | `wheels-isolated-01` |
| Sustain pedal | Channel 2 CC64, three 127/0 press-release pairs on inputs 1 and 2 | `pedal-all-port*-01` |
| Faders, BANK light off | Channel 1 CC64-67 left to right, 0-127 | `fader1-bank-ab-port*-01`, `faders-bank-off-2to4-01` |
| Faders, BANK light on | Channel 1 CC68-71 left to right, 0-127 | `fader1-bank-ab-port*-01`, `faders-bank2-port*-01` |
| Knobs, CC configuration | K1-K8: channel 6 CC48-55, increasing/decreasing absolute values; inputs 1 and 2 confirmed for K1 | `k1-preset4-port*-01`, `k2-k8-preset4-01` |
| Knobs, alternate configuration | K1-K8: pitch bend on channels 1-8, 0-16256; additional paired channel-1 note events; input 3 | `knobs-bank2-retry-port*-01` |
| Pads, Bank 1 | Channel 10 velocity-sensitive notes on inputs 1 and 2 | `pads-play-port*-01` |
| Pad pressure, Bank 1 | Channel pressure on channel 10, observed 0-109, returning to 0; no poly-pressure messages observed | `pad-pressure-bank1-01` |
| Pads, owner-identified Bank 2 | Fixed velocity 127 channel-1 note events on input 3; owner reports no press lighting | `pads-bank2-port*-01` |
| SEQ / SEQ PLAY / SEQ REC test | Sequence notes on inputs 1 and 2; no clock/start/stop/continue received in this test | `transport-port*-01` |

For the alternate knob configuration, the physical bank light state was not
independently confirmed. The pitch-bend/note patterns and alternate port suggest
DAW-control assignments; their exact protocol semantics remain unverified.
Earlier silent tests do not establish failed controls: some were in PARA or SEQ,
and some listened only to the main input. The first pedal pass was silent but
the repeat succeeded on that same input; its initial silence remains unexplained.

Bank 1 pad note order, following the owner's top-left to bottom-right pass:
`40,41,42,43,48,49,50,51,36,37,38,39,44,45,46,47`.
The photos show a two-row pad layout; do not render this as a four-by-four grid.
For Bank 2, the first eight taps each sent note 72 plus a note from 0 through 7.
The remaining eight sent `94,93,95,91,92,46,47,76`. Every captured press had a
matching release. These messages should not automatically be interpreted as
musical notes in a future DAW integration.

The aftertouch capture also contains an earlier burst of channel-1 sequence
notes. Its later note-40/channel-10 pad activity is distinguishable by timestamp;
it is not a perfectly isolated two-press recording.

## Porting implications

- Key velocity, pad pressure, wheels and pedal need distinct physical input
  handling. Host MIDI channel/CC assignments are configurable output behavior,
  not a GPIO map or an internal scanner protocol.
- Preserve musical and DAW port distinctions. Merging mirrored inputs 1 and 2
  without considering duplication could double notes in a host integration.
- Investigate the pitch center before copying calibration assumptions.
- A successful stock update is valuable evidence, but calibration backup,
  forced recovery, board revision, scanner wiring, LCD and DAC routing remain open.

See [startup analysis](STARTUP_ANALYSIS.md) for the first offline binary/source
comparison. No custom firmware has been built, flashed, or published in this round.
