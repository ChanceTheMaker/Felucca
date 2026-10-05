# SMK37 arrival report template

Copy this file into ignored `build/arrival/REPORT.md` for the actual keyboard.
Record observations and evidence paths as they are collected. Leave unmeasured
items unknown; a matching package or USB name does not establish a board pinout.
Keep personal recordings and device identifiers local.

## Unit and baseline

| Item | Observation | Evidence file |
| --- | --- | --- |
| Model printed on unit | Unknown | |
| Stock firmware shown on screen | Unknown | |
| Board revision, if later inspected | Unknown | |
| USB connection/cable/host | Unknown | |
| USB VID/PID and interface numbers | Unknown | USB before/after diff |
| Match to stock descriptor candidates | Unknown | Compare with INTERFACES.md; ID alone does not identify model |
| MIDI input names | Unknown | `midi-inputs.json` |
| Audio input/output endpoints | Unknown | OS audio settings observation |
| Stock presets/settings exported | Unknown | Note exactly what was exportable |
| Wheel/pedal calibration baseline | Unknown | Capture filenames and settings |

## Recovery evidence

| Requirement | Observation | Evidence file |
| --- | --- | --- |
| Exact-model vendor download URL and date | Unknown | |
| Stock package SHA-256 | Unknown | Offline package report |
| Official updater version/source | Unknown | |
| Documented update entry | Unknown | Manual/source reference |
| USB identity in documented update mode | Unknown | Separate USB diff |
| Official restoration demonstrated | Not tested | Result/log |
| Data/calibration preserved by restoration | Not tested | Before/after captures |
| Recovery from interrupted custom update | Not tested | Later bring-up stage |

The presence of recovery media is separate from a demonstrated restoration.
Do not fill a success field from a simulated updater test.

## Controls and measurements

Use the [diagnostic sequence](DIAGNOSTICS.md) and one labeled capture per test.

| Test | Result | Evidence / unresolved issue |
| --- | --- | --- |
| All 37 keys, low/medium/high velocity | Not tested | |
| Chords and repeated notes, clean releases | Not tested | |
| All 16 pads, velocity and pressure type | Not tested | |
| Eight encoders, both directions and speed | Not tested | |
| Four faders, travel/endpoints | Not tested | |
| Pitch wheel center/range | Not tested | |
| Modulation wheel range | Not tested | |
| Pedal press/release and note release order | Not tested | |
| Transport and panel modes | Not tested | |
| USB disconnect/reconnect | Not tested | |
| Analog audio and USB audio baseline | Not tested | |
| Idle/power/battery observations | Not tested | |

## Board bring-up evidence

| Unknown | How to establish it | Recorded result |
| --- | --- | --- |
| CPU/package and board revision | Clear component/board photographs; compare pinned research | Unknown |
| Internal RAM and reserved boot memory | Matching silicon documentation plus startup/debug measurements | Unknown |
| Flash geometry and protected data | Stock layout, physical part identity, backup/recovery evidence | Unknown |
| LCD controller and wiring | Panel markings, board trace/continuity and measured bus activity | Unknown |
| Key/pad scanning | Connector mapping and timed observations; distinguish velocity contacts and pressure | Unknown |
| Encoder/button mapping | Wiring and observed transitions | Unknown |
| Fader/wheel/pedal ADC channels | Wiring and measured raw endpoints/center | Unknown |
| DAC pins/format and audio clocks | Component identity, wiring and measured clock/data behavior | Unknown |
| USB/TRS routing and direction | Connector wiring and known-good communication | Unknown |
| Power/reset/battery signals | Board evidence and measured behavior | Unknown |

For electrical measurements, record the instrument, test point and conditions.
Do not infer safe signal levels or probe connections from FM-1 wiring.
Record stock boot settings as leads separately from measured runtime values.

## Next focused PR

Current stage: stock baseline collection.

Next implementation: choose after the first unresolved prerequisite is measured.
Attach the supporting evidence, describe the smallest hardware change, and
record how to return to the prior working firmware before the experiment.
