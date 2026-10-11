# SMK37 preparation

The SMK-37 Pro port has reached a hardware-tested private beta on the owner's
unit. Test12 plays Felucca's engines with the piano keys, pads and panel controls,
supports sustain and saved projects, and updates through normal USB after a
one-time recovery-adapter bootstrap. See [USB installation](smk37/USB_INSTALLER.md)
for test evidence and remaining limitations. Packages contain unit-specific stock
data and remain local; there is no public, redistributable SMK firmware release.
Elite hardware has not been tested.
Base: Salt RC1 `7b1164abd2322c279faf9bb57ae22283466b9f53`, incorporating upstream
v1.0.1 `20c275e39f75fa820978032efaceddfc5283c8cb`. No firmware or protocol
changes have been imported from another project in this round.

## Pro and Elite

The manufacturer's January 26, 2026 [model difference declaration][declaration]
identifies Pro and Elite as sharing circuitry and software. The associated
[test report][report], page 5, lists identical PCB layouts and hardware V02 for
the tested family. This supports one shared implementation. Mechanical feel
differences and the revision of the arriving unit still need inspection.

Model-specific package identities remain separate until update acceptance is
verified. Shared electronics do not establish that the stock updater accepts
another model's file. Do not rename an FM-1 image to make an SMK image.

## Evidence and unknowns

Research checked October 5, 2026. The community [SMK documentation][research]
is pinned at `ab85d7340163ba9e171dc9396e31c034829bbc02`. It reports an AC7911B8
SoC and CS4344 DAC. Treat these as leads to verify against this unit; a matching
AC791N container alone does not establish RAM size, board wiring or a safe
flash map. Research downloads stay under ignored `build/research/`; vendor
firmware is not redistributed in our source or releases.

| Subsystem | Felucca boundary to inspect | Evidence required from SMK |
| --- | --- | --- |
| Boot and clocks | `firmware/hal/fm1_sys.h`, vector assembly, `firmware/app.ld` | Boot entry, CPU/clock setup, RAM availability, mailbox reservations |
| Keys and buttons | `fm1_input.h`, `firmware/src/ui_input.c` | Matrix/secondary MCU protocol, key velocity timing, debounce, encoder wiring |
| Pads and LEDs | `fm1_input.h` and UI events | Velocity/pressure encoding, RGB driver, multiplexing/current limits |
| Wheels and faders | `fm1_adc.h`, modulation input | ADC channel mapping, endpoints, centers and calibration storage |
| Display | `fm1_lcd_hw.h`, `lcd.c` | Controller, bus/pins, reset, resolution and orientation |
| Audio | `fm1_audio.h`, `main.c` | DAC clocks/data format, sample rate, mute/amplifier pins |
| Storage and update | `fm1_flash.h`, `storage.c`, `ota.c`, `firmware/loader/`, `tools/fm1pkg_make.py` | Erase geometry, protected regions, application budget, verified restoration route |
| USB and TRS | `fm1_usb.h`, `fm1_uart.h`, `usb.c` | Descriptors/port roles, endpoint allocation, UART routing and electrical direction |
| Power | Board startup and idle logic | Battery sensing, shutdown, charge indicators and wake behavior |

Felucca's engines, effects and sequence data provide the reusable foundation.
Keep FM-1 GPIO, flash offsets and scan timing in its existing HAL while a new
SMK HAL is established. Avoid a wholesale HAL rename before the real boundary
is known. Experimental Bluetooth work remains excluded.

## Focused feature branches

1. `feat/smk37-preparation`: evidence, arrival/bring-up plan and explicit board
   selection. Default and `--board fm1` retain the FM-1 build. `--board smk37`
   stops before SDK access; it is a reserved target, not a stub firmware image.
2. `feat/smk37-package-inspection`: offline container inventory and Pro/Elite
   comparison, with corruption tests. No update transport or device writes.
3. `feat/smk37-diagnostics`: input-only stock MIDI capture and offline reports
   for controls, velocities and unbalanced note events. No output ports.
4. After arrival, separate PRs for verified board startup/recovery, display,
   inputs, audio, storage/update, then Studio/installer device support. Each
   depends on measured evidence and the preceding bring-up stage.

Fork PRs stay draft until reviewed. Keep this stack separate from FM-1 RC1
promotion and website publication. No upstream submission is authorized.

## Arrival session

1. Record model, firmware screen, USB device IDs, enumerated MIDI port names,
   boot behavior and external connectors before changing settings. Keep serial
   numbers and personal captures local. Photograph labels and, if opened later,
   board revision/component markings.
2. Save available presets/projects/settings through the stock software and
   document what it cannot export. A vendor update file is not a full backup
   of calibration or user data. Do not factory-reset to obtain a baseline.
3. Obtain the exact-model official package and M-Upgrade through the vendor
   download page. Record URL, date and SHA-256. The mirrored packages used for
   comparison do not replace verified official recovery media.
4. Establish the documented update/recovery entry and verify that the device
   enumerates. Record a successful official restore before a custom flash.
   The forced recovery procedure is still unknown; do not invent a key chord
   or assume the FM-1 loader preserves SMK calibration.
5. Capture each stock MIDI port separately. Test every key at soft/medium/hard
   velocities, repeats/chords, every pad plus held pressure, each encoder in
   both directions, fader endpoints, wheel center/limits, pedal and transport.
   Annotate mode/channel/settings and test one physical control at a time.
6. Record analog/USB audio, idle behavior and USB reconnection. MIDI host
   timestamps show host arrival timing, not key scanning or audio latency.
   Use simultaneous contact/audio measurements for real latency comparisons.

## Proposed Felucca controls

| Control | Initial mapping to evaluate |
| --- | --- |
| 37 keys | Selected track notes, velocity and octave/transpose |
| Pitch and modulation wheels | Track pitch bend and modulation |
| Four faders | T1-T4 levels, with pickup to avoid jumps after track/project changes |
| Encoders 1-4 | Current page's four parameters |
| Encoders 5-8 | Track/preset/navigation/macros, after checking physical push support |
| Sixteen pads | Sequencer steps; separate drum and performance modes |
| Transport | Play/stop/record; all held notes released on stop/disconnect/mode changes |
| Pedal | Sustain first; expression only if the socket and ADC support it |

Preserve a way to reach every Felucca page without requiring a connected browser.
Mapping is a proposal, not a claim about the stock MIDI CC numbers. Pad pressure
must be measured before choosing polyphonic versus channel aftertouch behavior.

## Bring-up acceptance

### Current evidence audit (October 8)

The current artifact is an experimental sound application, **not the completed SMK
Felucca port**. Sixteen driver host suites and a processor link do not satisfy
the acceptance stages below.

| Requirement | Current evidence | Still required |
| --- | --- | --- |
| Separate SMK application | Own startup, scanner, ADC, display/audio drivers; shared DSP; linked image (current size/hash in build manifest) | Physical startup, RAM/clock validation |
| Hardware mapping | Stock-derived contact tables, ADC channel/mux schedule, owner MIDI captures and photos | Verify electrical indices against every physical control on custom firmware |
| Playing behavior | 37-key fixed-velocity adapter, provisional encoder assignments and real DSP host tests | Calibrated velocity, pad pressure, wheels/faders, physical encoder validation, pedal and transport |
| Device interface | Track/engine/preset navigation, 23 sound pages and diagnostic page | Full sequencer, storage menus and physical standalone validation |
| Calibration preservation | Read-only decoder tested on synthetic records; no flash writer linked | Actual unit backup and verified physical storage mapping; vendor update file is insufficient |
| Recovery | Owner successfully used ordinary vendor update; traced software mailbox | Independently verified recovery from failed application startup; owner reports parts arrived October 10, adapter identity/wiring still unconfirmed |
| Persistence | Disabled to avoid using FM-1 storage geometry | SMK storage implementation and restore/power-interruption validation |
| Installation artifact | Analysis-only ELF/bin and manifest, `installable:false` | Model-specific loader/package with verified writes and protected-region preservation |
| Final validation | Host tests, source/disassembly checks | On-device display, audio, timing, full controls, boot/recovery and Studio reconnect |

Immediate hardware prerequisite: obtain a recovery path independent of this
application and a restorable backup of the unit's calibration before any
installation recommendation. The owner has explicitly requested no flashing.
Until that changes, the diagnostic image can be improved offline but must not
be promoted to an installer or represented as tested on the device. Do not use
the FM-1 updater or fabricate calibration defaults to bypass these requirements.

| Stage | Exit evidence before proceeding |
| --- | --- |
| Recovery | Exact board identity, restorable stock image, protected regions documented |
| Startup | Repeatable boot/return to stock, USB enumeration, stable clocks/watchdog |
| Display and inputs | Correct panel geometry, every control mapped, velocity/pressure and debounce checked |
| Audio | Quiet startup, correct rate/channels, stable pitch, no underruns under load |
| Synth | Engine/FX host regressions, physical polyphony/load and stuck-note tests |
| Persistence/update | Calibration preserved; saves, power cycles, interruption recovery tested |
| Product integration | Capability detection, exact model packages, backup and Studio reconnect tested |

Offline integration binaries may be built for analysis. No SMK installation
package is published or recommended before these prerequisites are resolved.
FM-1 host tests do not substitute for SMK hardware validation.

## Prepared tools and validation

- [Arrival results](smk37/ARRIVAL_RESULTS.md): observed stock controls, ports,
  bank behavior and limitations from the October 7 physical-device session.
- [Startup analysis](smk37/STARTUP_ANALYSIS.md): decoded stock entry-path RAM
  operations and concrete FM-1 hardware-layer assumptions that need replacement.
- [Board photographs](smk37/BOARD_PHOTOS.md): this unit's component markings,
  keybed construction, scanner hypotheses and next milestones.
- [Control tracing](smk37/CONTROL_TRACE.md): stock interrupt routines, candidate
  scan inputs/selection table and display-transfer differences from FM-1.

- [Package inventory and comparison](smk37/PACKAGES.md): 13 pinned research
  packages inspected offline; Pro/Elite wrapper and payload differences recorded.
- [Arrival diagnostics](smk37/DIAGNOSTICS.md): receive-only MIDI recording,
  offline control/event reports, and the stock baseline test sequence.
- [Flash and boot evidence](smk37/FLASH_LAYOUT.md): decoded directory/boot
  configuration and the concrete conflict with FM-1 storage addresses.
- [Stock interface evidence](smk37/INTERFACES.md): USB descriptor candidates
  across model/version combinations and diagnostic-string leads. IDs vary by
  version and overlap across models; runtime detection needs corroboration.
- [Arrival report template](smk37/ARRIVAL_REPORT.md): evidence fields for recovery,
  physical measurements and the first board bring-up PR. The diagnostic guide
  includes read-only Windows USB snapshots and offline comparisons.
- `python -m unittest discover -s tests -p "smk37_*_test.py"` runs the preparation
  tests without a device or SDK. Full firmware rebuilds and hardware tests are
  separate from this preparation round; no firmware source has changed here.

[declaration]: https://manuals.plus/m/fcbba55f34b76e45d57847128f5b1bff1ca6bffc0dca03cc5b4fdcabe0f1be5d.pdf
[report]: https://device.report/m/e1477f22153841fb96635ae650f53e8ba8cc6abb6095f8c36b40d7b4067e5b4d.pdf
[research]: https://github.com/jonathaslacerda/smk-37-pro-docs/tree/ab85d7340163ba9e171dc9396e31c034829bbc02


## October 10 recovery and calibration update

The owner's SMK entered WL82 ROM recovery through the XIAO adapter. A fixed
RAM helper read the first 1 MiB twice with flash writes disabled; both reads
match, and the stock firmware portion exactly matches Pro 016. The actual
556-byte usrtrim record is at 0xC1000 and all 37 key curves validate. The owner
confirmed normal stock operation after disconnecting the adapter. See
[recovery evidence](smk37/RECOVERY_RESEARCH.md) for hashes and limitations.

`tools/smk37_backup.py FIRST SECOND PRO016_PACKAGE` reproduces the offline
checks and rejects mismatched reads, invalid calibration, or unreviewed stock
packages. `tools/build_smk37.py --stock-backups FIRST SECOND --stock-package
PRO016_PACKAGE` validates those inputs before building and records their
metadata in the manifest. It does not embed private backup/calibration data
or enable calibrated velocity without physical scanner timing validation.
Installation remains disabled; neither custom startup nor SMK flash restoration
has been hardware tested. The historical October 8 table above is superseded
for backup, calibration location and ROM entry only.

## October 10 playable hardware and Test09 update

The historical installation/startup gates above are superseded for the owner's
specific unit by Test01-08 application installs with complete readback. Test08
has working piano/pads/audio/display, all-eight clockwise knob directions,
function/bank buttons, analog wheels/faders and indicator lights. Detailed
qualitative results and remaining gaps are recorded in smk37/CONTROL_TRACE.md.
This does not certify stock restoration or another unit's compatibility.

Test09 adds velocity curves, USB editor replies and RAM step editing, plus
read-only storage and pedal diagnostics. Saving remains compile-time disabled;
physical sustain mapping is pending. See smk37/SOUND_CONTROLS.md for controls,
the four-slot persistence design, host coverage and next hardware gates. Private
packaging never changes the public installer or the FM-1 firmware.
