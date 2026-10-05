# SMK37 preparation

Run Felucca natively on the incoming M-VAVE SMK-37 Pro, with a shared Pro/Elite
hardware target. This is pre-arrival development, not installable SMK firmware.
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

| Stage | Exit evidence before proceeding |
| --- | --- |
| Recovery | Exact board identity, restorable stock image, protected regions documented |
| Startup | Repeatable boot/return to stock, USB enumeration, stable clocks/watchdog |
| Display and inputs | Correct panel geometry, every control mapped, velocity/pressure and debounce checked |
| Audio | Quiet startup, correct rate/channels, stable pitch, no underruns under load |
| Synth | Engine/FX host regressions, physical polyphony/load and stuck-note tests |
| Persistence/update | Calibration preserved; saves, power cycles, interruption recovery tested |
| Product integration | Capability detection, exact model packages, backup and Studio reconnect tested |

No SMK binary is built or published before these prerequisites are resolved.
FM-1 host tests do not substitute for SMK hardware validation.

## Prepared tools and validation

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
