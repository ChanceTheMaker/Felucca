# Owner's SMK-37 Pro board photographs

Inspected October 7, 2026. Original JPEGs remain local in the owner's
`Downloads/iCloud Photos` folder. This inventory records visible evidence,
not a verified schematic. No further disassembly is requested for this stage.
The owner was advised to reassemble and check stock operation.

## Observed markings and construction

| Photo(s) | Observation | Limit |
| --- | --- | --- |
| IMG_3226 | U10, JieLi logo, `C161195-11B8`; adjacent crystal `24.000` | Top marking does not independently confirm an AC7911 variant or RAM capacity |
| IMG_3229 | Main board marking includes `MBV04`, `2025/10/28`, `260125` | This unit differs in revision marking from the earlier V02 family report |
| IMG_3227 | U13 marked `344C / LYE1`, near audio connectors | CS4344 remains an identification lead, not a confirmed pinout |
| IMG_3224-3225 | Eight-pin device marked `ACC3610S / A446` | Function not established |
| IMG_3230 | U2 clearly marked `HCF4051`; second similar package partly visible | Do not infer every nearby chip is the same type |
| IMG_3231-3233 | U6, U8 and U9 marked `74HC595D` | Chain order and controlled signals unknown |
| IMG_3235-3236 | Wider views place multiple 595-marked packages among the faders | Physical proximity does not mean they read the faders |
| Earlier wheel close-ups | Board labels `+3.3D`, `ADC5`, `GND`, and nearby `ADC6` | These are board net labels, not proven SoC ADC-channel numbers |
| IMG_3237-3238 | Keybed ribbon/connector, pin-1 marking, C1-C20 footprints | C labels identify capacitor footprints; not a row/column map |
| IMG_3239-3241 | Repeated keybed diode pairs and traces; no IC visible in these views | Does not establish the complete board has no IC, or prove the contact timing scheme |
| Earlier display photos | Display ribbon and metal back | Controller model, bus/pins and backlight polarity unresolved |

## How this changes the investigation

The [HCF4051 family](https://www.st.com/resource/en/datasheet/hcf4051.pdf)
is an eight-channel analog multiplexer. The
[74HC595 family](https://assets.nexperia.com/documents/data-sheet/74HC_HCT595.pdf)
is an eight-bit serial-in shift register with output latches. These datasheets
establish family functions; they do not establish this board's connections or
the manufacturer of every photographed package.

The working hypothesis is multiplexed analog acquisition for some controls and
shift-register-driven outputs for scanning and/or lighting. Keybed diodes and
the ribbon support investigating a directly scanned contact matrix before
assuming a separate keybed processor. The observed velocity response makes
dual-contact timing worth investigating, but photographs do not prove it.

Search the stock code for serial data/clock/latch writes, multiplexer address
changes followed by ADC acquisition, and recurring row sampling. Correlate
those routines with the known 37-key, 16-pad, 8-encoder and 4-fader behavior.
Do not transfer FM-1 GPIO numbers or its two-register chain length to SMK.

## Next milestones

1. Map stock copied RAM at its actual execution address and identify candidate
   scan/ADC/display/audio routines, with instruction locations and confidence.
2. Establish GPIO connections and timing from code plus corroborating evidence;
   preserve unknowns explicitly. Continue offline while the unit is assembled.
3. Resolve forced recovery and calibration/user-data preservation before a
   custom hardware test. The successful official 016 update proves normal
   updating, not restoration of an unbootable application.
4. Build a separate SMK adapter for input events, including 37 keys, velocity,
   pad pressure and analog controls; keep FM-1 drivers and storage unchanged.
5. Bring up startup/display, input, audio and persistence in separately validated
   stages, followed by Studio capability detection and exact-model installation.

No new firmware image, device write or claim of safe custom flashing follows
from these photographs.
