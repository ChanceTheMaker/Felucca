# Pro 016 control and display candidates

Offline instruction analysis, October 7, 2026; same pinned application as
[STARTUP_ANALYSIS.md](STARTUP_ANALYSIS.md). No code was run on the SMK.
Addresses here are CPU addresses, not physical flash offsets. The mapped
listing is local `build/research/startup/pro016-mapped.disasm.txt`.

## Scanner candidate: 0x0201BE6A

This routine has an interrupt save/restore sequence and ends with `rti` at
`0x0201BF9E`. Initialization at `0x02004DE6` loads its address at
`0x02004E58` and passes it with arguments 37, 5 and 0 to `0x0200164C`.
That supports an interrupt-registration interpretation; argument 37 must not
be mistaken for the number of piano keys without tracing the callee.

Direct observations:

- `0x0201BE9A` loads `0x13100`; subsequent instructions select bits 8-11 and
  write a control word. This matches the ADC register address used in the
  existing FM-1 HAL, but physical ADC inputs remain unmapped.
- `0x0201BEE6` loads `0x501C4` and reads it; `0x0201BEF8` loads `0x50000`
  and subsequent instructions read offset 4. Bit extraction combines these
  reads into a byte, saved at state-base + 1767 + bank*16 + slot.
- The slot increments and wraps after 15 (`0x0201BF38` through `0x0201BF44`).
- `0x0201BF46` loads table address `0x02059028`, indexes 32-bit entries,
  applies two state-dependent masks, and stores a word into a buffer.
- At `0x0201BF64` and `0x0201BF68`, peripheral `0x11E00` receives that
  buffer address at offset 12 and count 4 at offset 16. GPIO address
  `0x50000` bit 1 is set before this setup and cleared afterward.

Under the shared GPIO-register interpretation in `firmware/hal/fm1_gpio.h`,
the sampled bits would be:

| Packed bit | Candidate input |
| --- | --- |
| 0 | PA2 |
| 1, 2, 3 | PA5, PA6, PA7 |
| 4 | PA9 |
| 5 | PH6 |
| 6, 7 | PH8, PH9 |

The GPIO numbers are derived from instructions plus the FM-1 register map,
not verified connections to specific keys/buttons on this unit.

The 16 little-endian words at app offset `0x58F08` are:

```text
FEFFFFFF FDFFFFFF FBFFFFFF F7FFFFFF EFFFFFFF DFFFFFFF BFFFFFFF 7FFFFFFF
FFFEFFFF FFFDFFFF FFFBFFFF FFF7FFFF FFEFFFFF FFDFFFFF FFBFFFFF FF7FFFFF
```

This selects one low bit among bits 24-31, then 16-23, before masks are applied.
Combined with the photographs of 74HC595s, an eight-input, sixteen-slot scan
with serial output is a strong hypothesis. It does not yet establish which
slots correspond to keys, encoders or buttons, chain order, byte/bit order on
the wire, latch polarity, velocity timing, or the purpose of the other bits.
Do not assume the four-byte transfer covers every photographed shift register.

## Display-transfer candidate: 0x0201B44C

This interrupt routine accesses `0x11D00` (SPI1 in the FM-1 HAL). It sets
`0x50080` bit 8, subsequently clears it before transfers, and changes
`0x50040` bit 5 around writes of commands `0x2B` and `0x2C` at
`0x0201B49A` and `0x0201B4EC`. Buffer addresses and transfer counts follow.
Those values and transfer patterns are consistent with a display transaction.

Working interpretation: PC8 is a chip-select candidate and PB5 is a data/command
candidate. Their association and polarity still require tracing initialization
and corroboration. In contrast, the existing FM-1 LCD driver uses PC7 for CS
and PC8 for D/C. Copying that driver unchanged is not justified.

The nearby setup at `0x0201B412` writes SPI baud value 8 and a transfer count
`0x1A4`; other paths change baud to 4. These are observed register values,
not a measured clock rate or proof of panel dimensions/controller identity.

## Copied-RAM entry call resolved

The six-byte call at `0x0200019E` has displacement -4191000. Its target is
`0x01C00E8C` (address after the instruction plus displacement), where the
mapped RAM listing has a function prologue. It accesses `0x01EEE008` and other
registers; no control-scanner role is assigned. This confirms why copied bytes
must be disassembled at their RAM execution addresses.

## Contact reader and 37-pair consumer

The routine at `0x0201BFA0` reads the completed frame pointer at state + 416.
For the observed table codes it uses the high nibble as the byte/scan-slot
index and the low nibble as the input-bit index. It returns 1 for a low input
(closed contact), 0 for a high input. The actual low-bit instruction masks with
`0xFFFFFF0F`; for the validated byte-sized table arguments this is equivalent
to selecting the low nibble. Do not generalize it to arbitrary 32-bit input.

The second interrupt consumer begins at `0x0201C0F0`. At `0x0201C188` it
indexes pairs from `0x02059178` (base `0x02057C10` + 5480) and calls the reader
twice. It combines them as `a | (b << 1)`. The loop terminates after exactly
37 iterations at `0x0201C324`. This strongly identifies the piano-contact path.
The pairs, in firmware index order, are:

```text
00:10/52 01:20/72 02:30/92 03:40/B2 04:50/D2 05:60/F2 06:70/E2
07:80/C2 08:90/A2 09:A0/82 10:C0/62 11:D0/42 12:E0/22 13:F0/12
14:B0/32 15:11/33 16:21/43 17:31/53 18:41/63 19:51/73 20:61/83
21:71/93 22:81/A3 23:91/B3 24:A1/C3 25:B1/D3 26:C1/E3 27:D1/23
28:E1/13 29:F1/14 30:64/24 31:74/34 32:84/44 33:94/54 34:A4/D4
35:B4/F4 36:C4/E4
```

Each code is hex `slot:bit`, not a MIDI note number. All 74 contacts are unique.
Physical left-to-right order, which contact closes first, and corresponding MIDI
note names have not yet been traced. Do not label index 0 as C3 just because
stock MIDI capture covered notes 48-84.

### Timing and response observations

Per-index state has a 16-byte stride. The routine compares repeated two-bit
samples, resets a counter on changes, and accepts a differing state after the
counter reaches 10 (`0x0201C24E` through `0x0201C256`). This is ten consumer
iterations, not ten milliseconds: the invocation rate is not established.

While the accepted contact state is 1, a 32-bit timing counter is incremented
at `0x0201C1E4` through `0x0201C1FA`. Transition paths initialize that counter
to 10 and latch it into another field when the second contact state arrives
(`0x0201C2CC` through `0x0201C30E`). Release paths clear the latched value.

When that latched value changes, the code derives an inverted value using
1023 minus the duration, handles endpoints, and interpolates against five
16-bit breakpoints per index. Those breakpoints are in RAM at state + `0x2C80`
with ten bytes per index. It clamps the mapped nonzero response to an internal
1-1023 range and queues an event; zero follows a separate release path.
This is evidence of timing-based key response with per-index response data,
not yet the complete MIDI 1-127 velocity conversion. Breakpoint initialization,
stored calibration, counter overflow behavior and event consumers remain to trace.

## Eight encoder pairs

Immediately afterward, `0x0201C33E` reads pairs from `0x02058510`
(base `0x02057C10` + 2304). The loop advances state by 20 bytes and ends at
160, giving eight entries. It maintains a two-bit state history and increments
separate counters for different transition sequences (`0x0201C37E` through
`0x0201C3C0`), supporting the quadrature-encoder interpretation.

```text
0:97/A7 1:37/47 2:17/27 3:B7/C7 4:57/67 5:77/87 6:D7/E7 7:F7/07
```

All encoder contacts use packed input bit 7, the candidate PH9 input, and span
all sixteen slots. They are distinct from the 74 key contacts. Physical knob
order, clockwise sign and detent scaling still need corroboration.

The eight-entry input-initialization table at `0x02058B44` contains decimal
`118, 120, 121, 2, 5, 6, 7, 9`. It is passed to `0x02004CF0` by the setup
loop. Keep these as raw GPIO-helper arguments until the helper encoding is
fully traced; their apparent correspondence to PH6/8/9 and PA2/5/6/7/9 is
supporting evidence, not independent electrical verification.

## Reproduce and remaining work

### Button path and analog-channel leads

`0x0201C4F8` includes an 18-entry loop (`0x0201C5DE` to `0x0201C668`)
using contact bytes at `0x02058685`. These are all distinct from the key and
encoder tables, bringing the traced digital contacts to 108 of 128 possible
slot/bit combinations:

```text
26 76 96 56 66 36 75 65 25 45 15 86 95 55 85 35 16 46
```

This path waits for three observations before press/release events, and has a
14-observation held-state transition plus combination handling before the loop.
Do not convert those counts to milliseconds or assign physical button names yet.
It queues events with the button index in bits 16-23 and action in the low bits.

The ADC interrupt at `0x0201D9AC` reads results from `0x13104`. A 16-entry
path uses the permutation at `0x02058540`:
`1,2,3,0,4,7,5,6,9,10,11,8,12,15,13,14`.
Other branches use a four-entry order `1,2,3,0` at `0x02057E1F` and a two-entry
order `4,5` at `0x02057E23`. Their cardinalities suggest controls to investigate,
but do not prove which is pads, faders or wheels. The four-entry branch averages
eight readings and applies a change threshold before queuing type-7 events.
Physical ADC channels, mux select wiring, and the complete pad pressure path
remain unresolved.

### Response defaults and persistent trim evidence

Initialization at `0x02005074` onward obtains a storage reference, checks an
eight-byte marker at offset 548, and reads 548 bytes into state + 11224.
The fallback path constructs the marker `trimdat\0`, populates response tables,
and passes the block and marker to storage-writing helpers. Runtime code at
`0x0201E30A` also reads/writes this block. These operations were inspected
offline only; none was invoked on the keyboard.

The fallback tables, decoded as five little-endian 16-bit values, are:

| Application address | Initialization destination | Values |
| --- | --- | --- |
| `0x0205811E` | State + 11232, copied 16 times at ten-byte stride | 50, 550, 720, 850, 950 |
| `0x02058128` | State + `0x2C80`, copied 37 times at ten-byte stride | 5, 640, 800, 920, 980 |
| `0x02058132` | State + 11762, one table | 20, 850, 920, 980, 1010 |

The key consumer's 370-byte response bank lies entirely inside the 548-byte
block. This is concrete evidence that key response data participates in stock
trim persistence. Defaults are not a backup of this unit's calibration. The
physical storage address and storage helper semantics still need tracing.
Further tracing of `0x02005086` resolves its file argument to
`mnt/sdfile/app/usrtrim` (string at `0x020587D9`), opened with `r+w` at
`0x02057C79`. This connects the trim block to the stock usrtrim file, rather
than relying only on the descriptor name. File-attribute helper `0x0200475C`
returns an address subsequently converted by `0x02004770` as
`address - state[1620] + state[1616]`. Reads reach the flash helper at
`0x020048FA`, writes at `0x02004B9C`. The two address bases still need tracing
before assigning a physical flash location.

The block has eight leading bytes whose meaning remains opaque, then 16
five-point curves (offset 8), 37 key curves (offset 168, 370 bytes), and the final
five-point curve (offset 538). The marker follows at offset 548. No actual
unit trim dump is available yet.

`firmware/hal/smk37_trim.h` reads this layout without modifying it. It validates
the marker and all 37 key curves, rejects malformed or erased data, and applies
the key timing interpolation traced at `0x0201C20E..0x0201C29A`. Zero duration
means release; other durations are converted through `max(1,1023-duration)`
and the five calibration breakpoints. Four interpolation intervals span 256
response units each. Unlike the stock arithmetic's possible 1024 endpoint,
the new implementation bounds the result at 1023 before MIDI conversion.
Duration units are consumer iterations, not yet a measured time interval.

The caller must validate the complete decoded record before accessing a key
curve. The decoder does not read flash, generate defaults or overwrite trim.
Tests use a clearly identified synthetic default fixture, cover all 37 keys,
monotonic timing, breakpoints, malformed markers/curves, truncation, invalid
indices and unchanged input bytes. `tools/test_smk37_controls.sh` runs the
three native C tests under address/undefined-behavior sanitizers and optionally
cross-compiles with `JIELI_TOOLCHAIN` set. All pass; the combined compilation
probe now contains 728 code bytes and 116 constant bytes, with no unresolved
external symbols. This is a component object, not an installable image.

### Event queue consumer located

The control event queue starts at RAM `0x01C371C0`. The loop at `0x0202933C`
compares read/write indices at state + 164/+162, reads a 32-bit event, wraps
at 256, and separates the type byte (bits 24-31) from the control index
(bits 16-23). Dispatch depends on operating mode and includes indirect tables.
Keys queue type 1 with response in the low 16 bits; encoders queue type 4
with signed steps in the low 16 bits. The queue and subsequent dispatch are
identified, but final MIDI velocity conversion and physical note ordering are
not yet established. Other code reuses this RAM area at offsets, so a reference
to its base alone is not proof of an event consumer.

`tools/smk37_controls.py` accepts only the pinned decoded application and emits
the contact pairs, slot/bit interpretation, candidate GPIOs and raw init table.
It rejects malformed/duplicate contacts and overlap between the two tables.

```powershell
python tools/smk37_controls.py E:/AI_Apps/Felucca/SMK37-research/pro016/app.bin --out build/research/startup/pro016-controls.json
```

Tests exercise every single-contact closure across the 128-bit frame, open
inputs, pair ordering, malformed tables and rejection of an unknown application.
These verify the host extraction/model, not physical scanner behavior.

### First isolated firmware components

`firmware/hal/smk37_contacts.h` decodes a coherent 16-byte scan frame into two
37-bit key-contact masks, 18 buttons and eight encoder phase pairs. It retains
keys 32-36 using 64-bit masks. It performs no register access or debounce.
`tests/smk37_contacts_test.c` covers all 108 individual contacts, unused frame
positions, simultaneous closures and releases.

The normal-key event dispatch at `0x020275FC` reaches `0x02026416` via
`0x0202828E`. The latter calculates `48 + index + 12*octave + transpose`, clipped
to 0..120, before sending notes. This agrees with the owner's captured 48..84
range; physical contact ordering still needs checking when the scanner runs.

Response helper `0x020263B0` has a byte jump table at `0x020263BA`:
`02 08 0E 11`, selecting code at `0x020263BE`, `0x020263CA`, `0x020263D6`,
and `0x020263DC`. For calibrated response `r` in 1..1023 the four modes are:
`max(1,r>>3)`, `(3*r>>5)+32`, `(r>>4)+64`, and fixed 127. Input zero returns
zero. Subsequent helper `0x020263E8` applies preset velocity endpoints, including
descending ranges. Stock release dispatch bypasses that range helper.

`firmware/hal/smk37_response.h` implements these bounded conversions and normal
note calculation. It explicitly preserves releases and rejects invalid key
indices. These are response conversions, not replacements for contact timing or
unit-specific calibration. Curve labels in the header are descriptive; stock
menu names have not been recovered.

Both C host tests pass with `-std=c99 -Wall -Wextra -Werror
-fsanitize=undefined,address`. Response tests cover every 10-bit input and every
pair of 7-bit velocity endpoints. `tests/smk37_target_compile.c` forces code
emission for a freestanding device-toolchain check. Compiling with pi32v2 clang,
`-Os -ffreestanding`, succeeds: 444 bytes code, 108 bytes contact tables, and no
unresolved external symbols. This object is not a bootable firmware.

### Display initialization sequence

The loop at `0x02024520..0x02024590` reads 17 records of 18 bytes from
`0x0205964C`. Records 1 and 16 call delay helper `0x02064706`; their first byte
is not sent as an LCD command. Delay units remain unverified. The sequence is:

```text
11
delay 120
36:00
3A:05
B2:0C 0C 00 33 33
B7:35
BB:32
C2:01
C3:15
C4:20
C6:0F
D0:A4 A1
E0:D0 08 0E 09 09 05 31 33 48 17 14 15 31 34
E1:D0 08 0E 09 09 05 31 33 48 17 14 15 31 34
21
28
delay 50
```

The table ends with command 0x28. Next call `0x0201EB1E` sets 2A/2B windows,
sends 115200 bytes with 2C (consistent with a 240x240 RGB565 frame), and finally
sends display-on 0x29 at `0x0201EC22`. Backlight control remains unresolved.

LCD initialization clears IOMAP `0x51020` bit 4. Stock also uses SPI1 for a
420-byte transfer initialized at `0x02004FD4` with that route bit set; the latter
appears related to LED data but its role remains unverified. Do not assume all
SPI1 traffic is LCD or copy the FM-1 SPI route. PC8 chip select and PB5 command/
data remain strong static candidates. The local AC79 SDK's
`include_lib/driver/cpu/wl82/asm/spi.h` confirms that bit 4 clear selects
PB6 clock and PB7 output, matching the stock GPIO setup. Bit 4 set selects
PC9/PC10. These are static wiring findings, not continuity measurements.

Scanner initialization at `0x02004E20` clears route bit 16 and sets bit 17;
the same SDK maps that SPI2 route to PA3 clock and PA4 output (PA2 input).
Stock separately configures PA1 as an output and toggles it around transfers.
It sets SPI2 control 0x6020, baud 5, and starts four-byte transfers before
enabling bit 0. PA1's external latch role and complete timing remain to verify.

Delay helper `0x02064706` divides by ten, rounds upward when the remainder is
greater than five, enforces at least one tick, then calls `0x02062988`.
This is consistent with a millisecond-to-OS-tick wrapper, but the tick duration
still needs independent verification before implementing the delays.

Next implementation work: complete scan timing/ADC and display wiring, preserve
the physical trim storage, and establish a recoverable SMK boot/update path.
The new components have not been enabled in hardware firmware.

### Scan cadence and dual-contact state machine

The SDK identifies scanner IRQ 37 as SPI2 and consumer IRQ 122 as software
interrupt 2 (`asm/hwi.h`). At `0x0201BF70..0x0201BF92`, the scanner publishes
the completed 16-slot buffer, switches capture banks, and sets bit 2 at
`0x01EEF1A0` to request that software interrupt. Consequently the key timing
unit is a completed scan frame, not a fixed OS tick. SPI2 baud 5, peripheral
clock selection and interrupt overhead all affect the physical duration.
Actual scan cadence still needs measurement or a complete clock trace.

`firmware/hal/smk37_keyscan.h` implements the independent contact state machine.
A candidate requires ten repeated observations after its first observation.
First-contact acceptance starts the elapsed counter at ten; accepting both
contacts latches duration. Direct acceptance of both contacts gives duration
ten. The counter is disabled while releasing through the first contact, so a
bounce back to both contacts does not emit another press. A stable open state
emits a zero-duration release. Counter saturation avoids wraparound.

There is one intentional abnormal-sequence difference: accepting second-contact
only releases an existing note. Stock clears part of the state without clearly
clearing the latched duration on that branch; the port explicitly prevents a
stuck note. Physical behavior has not been tested. Native tests cover noise,
incomplete presses, timed presses, release bounce, retrigger, reversed contacts,
overflow and all 37 independent keys. The four C suites pass under ASan/UBSan.
The combined target compilation probe now contains 886 bytes of code and 116
bytes of constants, with no unresolved external symbols.

### Audio transport recovered and separately implemented

Stock setup `0x02007198..0x02007322` resets ALNK0 at `0x12E00`, clears
IOMAP CON5 bits 6-7, enables clock pins PC0/PC2/PC1, sets DMA length 128,
and configures CON3 to 0x83. The channel table at `0x020591C4` has four
16-byte records: the first three are zero; channel 3 has enabled=1,
direction=0 and DMA buffer `0x01C422D4`. Pin table `0x020587AE` begins
32,33,34,35,36,37,38, selecting PC6 for channel 3. PC6 is driven low before
setup and high after a settling loop. The table at `0x020585C0` identifies
the channel DMA address registers, ending at `0x12E1C`.

IRQ 11 is attached to RAM routine `0x01C0440C`. It acknowledges other pending
bits, identifies the free buffer half as `((CON0 >> 15) & 1) ^ 1`, posts work,
and acknowledges channel 3. This matches the existing FM-1 transport boundary.
Stock stores a nominal rate value 44118 at `0x0200716E`; that is not an
independent sample-clock measurement.

`firmware/hal/smk37_audio.h` adapts the existing GPL audio transport with its
credits retained. It is a separate board driver, accepts delay/IRQ hooks from
SMK startup, validates arguments before register writes, and supplies pending,
free-half, acknowledge and stop operations. It does not import FM-1 vector,
timer or clock initialization. Required audio-clock setup, RAM placement and
actual DAC output remain unverified, and the driver is not enabled in a build.

`tests/smk37_audio_test.c` uses register-memory simulation to check configured
pins, preserved unrelated bits, DMA address/length, IRQ-before-enable ordering,
pending acknowledgments and shutdown. All five C suites pass with ASan/UBSan;
the combined target compilation probe emits 1316 code bytes and 116 constant
bytes with no unresolved external symbols. Simulation does not establish
electrical correctness, sound, sample format or measured rate.

### Separate LCD transport implemented

`firmware/hal/smk37_lcd_hw.h` implements the traced SPI1 route A with PC8
chip select and PB5 command/data. GPIO direction setup retains the stock
PB5/PB6 output and PB7 input-direction configuration before peripheral enable.
The driver accepts a baud divisor from board startup, performs synchronous
command/DMA transfers, and bounds polling. A timeout disables SPI and its
interrupt, deselects the panel, and returns failure; callers must reinitialize
before retrying. It neither drives PA2 nor invents a backlight assignment.
SPI1 ownership must be coordinated with any future LED driver.

The register simulation covers GPIO/routing setup, unrelated register-bit
preservation, invalid arguments, command/data selection, DMA programming,
timeout cleanup and rejection after failure. The simple RAM register model
does not emulate write-one-to-clear semantics or actual SPI completion.
Panel startup commands, clock rate, reset/backlight and visible rendering are
not yet integrated or validated. Six C suites now pass; the combined processor
compilation probe contains 1518 code bytes and 116 constant bytes, with no
unresolved external symbols. It remains an object file, not a bootable build.

### Panel startup sequence implemented

`firmware/hal/smk37_panel.h` now sends the recovered settings through a
synchronous transport interface, stages setting bytes in RAM for DMA, clears
115200 RGB565 bytes, then sends display-on. Window coordinates are bounded to
0..239 inclusive; this deliberately avoids reproducing stock's initial 240
endpoint. Invalid dimensions and failed transfers stop immediately. No reset
or backlight GPIO is guessed.

The startup delays are 100, 120 and 50 ms. Stock's delay wrapper divides by
ten and the matching local SDK defines `OS_TICKS_PER_SEC=100` in `os_type.h`
and `os_cfg.h`, supporting that interpretation. The board must still supply a
working millisecond timebase; stock's runtime OS tick has not been measured.

`tests/smk37_panel_test.c` checks pixel format, complete black frame, window
bounds and display-on ordering, and injects failure at every transport call
to ensure no later commands are issued. All seven C suites pass with ASan and
UBSan. The combined processor probe compiles to 1822 code bytes and 308 bytes
of constants without unresolved externals. Panel command/data callbacks still
were subsequently bound in the offline SMK integration target described below.

## Application integration

`firmware/src/smk37_app.c` is a separate entry point that connects the panel
transport/sequence, ALNK channel 3 DMA, USB MIDI, and Felucca's original
DSP/sequencer. It does not include the FM-1 ADC (PB6 conflicts with the SMK
display), TRS UART (PH8 conflicts with the scanner), FM-1 GPIO matrix, radio,
flash storage or OTA code. User sample slots resolve to an empty RAM-independent
header rather than FM-1 flash locations. The panel displays a bring-up status
screen with scan, audio and late-render counters. Full SMK control menus are
not implemented in this target yet.

`tools/build_smk37.py` generates an ELF, disassembly, analysis-only binary and
manifest under `build/smk37-integration/`. It does not build an updater or `.fwsc`.
The shared conservative AC79 linker allocation fits this 332008-byte build,
but SMK physical RAM, clocks and loader handoff still require validation.

`smk37_synth.c` reuses the actual sound and sequencer sources, with 37 local
key slots and a velocity-aware note-on path. Its event adapter preserves the
track and note chosen on press until release, and upper keys participate in
chord/note ownership. The existing FM-1 path retains 27 keys and velocity 100.
`tests/smk37_synth_test.c` renders audible samples from all 37 keys and checks
velocity output, duplicate-note ownership, bounds, and releases after selection
changes under ASan. This test supplies events in software; physical scanner
operation, calibrated velocity timing and physical note ordering remain open.

The standalone application now also enables SPI2 IRQ 37 and feeds the contact
timing state machines from completed 16-slot frames. It discards the first
frame, uses a bounded single-producer/single-consumer queue, and applies key
events only in the audio IRQ. Overflow releases local notes. Velocity is
explicitly fixed at 100 for bring-up until actual calibration is available.
The auxiliary selection masks start at `0xffffffff`, matching the stock copied
RAM data at `0x01C08B68` / `0x01C08B70` (app offsets `0x965E8` / `0x965F0`).
No ADC mux schedule or pad behavior is enabled yet. The subsequent digital
control diagnostic integration is described below.
The scanner register test checks GPIO packing, the complete selection cycle,
route preservation, startup-frame discard and stop. A separate SPL handoff
test checks the field mapping from stock `0x020001E6`, including preservation
of untouched reserved bytes. Both pass under ASan/UBSan.

Validation command: `sh tools/test_smk37_application.sh` with the same Python
Pillow/fontTools dependencies as the normal firmware build. It also runs the
existing FM-1 chord and expressive MIDI regression suites after generating assets.

### Digital control diagnostics (October 8)

The integrated scanner now feeds `smk37_controls.h`: 18 button contacts use
three consecutive complete frames of debounce, and eight quadrature pairs
report signed electrical quarter transitions. The first encoder sample only
seeds state; simultaneous changes of both phase bits resynchronize without
inventing a direction. These are bring-up policies, not claims to reproduce
stock hold/combo timing or detent scaling. Physical clockwise direction, scan
rate, button names and knob ordering remain to be measured.

The diagnostic display reports the last button index (1..18) with up/down and
last encoder index (1..8) with electrical +/- direction; zero means no event
yet. With simultaneous events only the last stock-order index is displayed.
These controls do not yet alter patches or transmit MIDI. All 14 driver suites
pass with ASan/UBSan, including debounce/bounce, simultaneous button releases,
all eight encoder cycles in both directions and invalid-transition recovery.
The processor application links successfully; this is not hardware validation.

### Analog scheduling and scanner coupling (October 8)

Further offline tracing resolves the conversion handshake, without enabling
ADC or changing the connected unit. Addresses below refer to the pinned Pro016
application/disassembly. State base is `0x01C332A0`; auxiliary base is
`0x01C08B50`.

- `0x02004F38..0x02004F8E` attaches ADC IRQ 24 at priority 5, sets
  auxiliary +32 to `0xFFFFFFC7`, desired channel at state +436 to 1, and
  handshake byte state +29 to 1.
- At the start of the SPI2 ISR (`0x0201BE76..0x0201BEDA`), handshake values
  1 and 2 advance without starting conversion; value 3 starts it and advances
  to 4. It compares desired channel +436 against auxiliary +28. An unchanged
  channel restarts through ADC CON bit `0x40`; a changed normal channel writes
  `((channel << 8) & 0xF00) | 0x407D` to `0x13100`. Channel 15 has additional
  setup and must not be treated as an ordinary external pin.
- The same scanner ISR builds its next shift word at `0x0201BF44..0x0201BF68`
  by ANDing the digital selection table with auxiliary +24 and +32. Thus an
  ADC implementation must coordinate the mux mask with SPI2 completion; an
  independent unsynchronized GPIO scan is not equivalent.
- The ADC ISR acknowledges CON bit `0x40`, handles the result at `0x13104`,
  then schedules the next conversion through state +436 and resets handshake
  +29 to 1 (`0x0201DEA8..0x0201DEAE`). It does not immediately start the next
  conversion itself.
- For primary indices 0..15, the mux mask uses bits 3..5 of auxiliary +32:
  `0xFFFFFFC7 | ((index & 7) << 3)`. The channel lookup at `0x02058294` contains
  32-bit values 1, 8, 2 (directly extracted from app.bin). The first eight
  indices use channel 1, the second eight channel 8. The branch at
  `0x0201DBBC..0x0201DBD4` selects these through byte offset
  `(index >> 1) & 0x7C`, not the logical pad permutation.
- Primary indices 16 and 17 select channel 2 with mux positions 4 and 5.
  Primary index 18 selects a secondary index: 0..3 use channel 2/mux 0..3;
  4..5 use channel 2/mux 6..7. Secondary completion increments its index,
  wraps at 8, and returns primary to 0 (`0x0201DBA4..0x0201DBBA`). The path
  for secondary 6..7 clears bit `0x4000` at `0x11900` and returns without the
  normal rearm (`0x0201DEBC`); its continuation still needs tracing.

The 16-input path also has baseline tracking, peak/history handling and
type-2/type-3 events, beyond its previously noted average filter. Some floating
point instructions in that path are not decoded by the available objdump;
do not infer their operation from the misaligned text that follows them.
Physical pin names, mux output wiring, secondary 6..7 handling, complete pad
response and calibration are still open. This evidence supports a coordinated
SMK ADC scheduler, not enabling the existing FM-1 ADC driver.

The local primary SDK `include_lib/driver/cpu/wl82/asm/adc_api.h` maps channels
1/8/2 to PA8/PC9/PA10, consistent with stock initialization of GPIO IDs
8/41/10 at `0x02004F62..0x02004F6C`. This establishes the chip input names,
not which front-panel control each mux position represents. `WL82.h` identifies
`0x11900` as `JL_ANA->WLA_CON0`; the SDK's `cpu/wl82/key/adc_api.c:adc_sample`
labels bit 14 `TEST_TO_ADC_EN_12v` and sets it specifically for PMU VBG.
The unresolved stock branch is therefore an internal analog-monitor path;
its continuation is still not proven.

`smk37_analog_scan.h` implements a separate **external-only bring-up schedule**
over all 24 channel/mux pairs: two eight-way banks on channels 1/8 and all eight
mux positions on channel 2. It retains the traced 4,5,0,1,2,3,6,7 order for that
last bank, waits three SPI completions, accepts exactly one result per start,
and associates each result with the selection that produced it. Unlike stock,
it sweeps each external input once per cycle rather than repeating the pad
bank between secondary samples. It does not implement internal monitoring,
stock sample cadence, pressure response, or GPIO/ADC register access, and is
not connected to the application yet. This is a building block for observing
raw controls, not a finished analog-control driver. Host tests under ASan/UBSan
cover all 24 selections, preserved non-mux bits, settling, stalled conversions,
duplicate/early results, sample ownership and full-sweep wrap.

### External ADC transport integrated (October 8)

`smk37_adc.h` now connects that diagnostic schedule to PA8/PA10/PC9 and SARADC.
It configures only those GPIO bits as analog inputs without pulls, clears the
internal-test selection, and uses stock's divisor/start delay with ADC IRQ
disabled (`0x405D | channel<<8`). SPI2 IRQ owns both scheduling and completion
polling, avoiding a second producer racing the mux selection. Completed
conversions are stopped, stored under their original input index, and followed
by a new three-completion settling interval. After 128 SPI completions without
an ADC result, it disables ADC and records a timeout; digital scanning remains
active. The bound is in SPI completions, not a claimed wall-clock interval.

The scanner mask setter owns only bits 3..5, preserving digital selection bits.
Application startup enables this path; the diagnostic screen rotates through
inputs 01..24 at 750 ms intervals and shows raw ten-bit values or ADC TIMEOUT.
These are electrical indices, not verified front-panel labels. Shutdown stops
the scanner before ADC so it cannot start another conversion during teardown.
No analog values are yet applied to synthesis, MIDI, or persistent calibration.

Sixteen host driver suites pass with ASan/UBSan. The joint ADC/scanner test
checks two complete sweeps, GPIO bit preservation, conversion ownership,
settling, IRQ exclusion, mux/digital-bit separation and timeout behavior.
Register RAM simulation does not model analog settling or W1C electronics.
The offline application builds with the new driver; physical behavior, pressure
response, stock-equivalent cadence and internal monitoring remain unverified.


## October 10 scanner cross-check before Diagnostic 03

Re-read pinned Pro016 scanner initialization and sampling instructions rather
than relying only on the earlier table. The eight u32 pin identifiers loaded
from `0x02057CD2 + 3698` are `(118,120,121,2,5,6,7,9)`, corresponding under
the stock port numbering to PH6/PH8/PH9 and PA2/PA5/PA6/PA7/PA9.
The direct sample packing at 0x0201BEE6..0x0201BF30 matches the current
`smk37_scan_tick` expression. Stock SPI2 control 0x6020 and baud 5 also match.
The row-input helper 0x02004C62 sets register offset +8; digital enable helper
0x02004454 updates offset +12, matching the relevant current direction/DIE
writes. These checks reduce the likelihood of a simple table transcription
error; they do NOT verify routing, row-selection output, physical timing or
shift-register wiring. ADC setup's PA8/PA10 and PC9 masks do not overlap these
eight row inputs. Diagnostic 03 remains unchanged pending the raw-byte test.

Latest user feedback for Diagnostic 02: uptime, scan, audio and IRQ counters
increase, but interpreted button/encoder indicators do not react. Next hardware
evidence required is whether any raw scan byte changes when one control is
operated, including after release (latched star/change count). No additional
flash operation was performed during this offline cross-check.


### October 10 audio cross-check while awaiting raw-input evidence

Re-read stock 0x02007198..0x02007322 and the output-high helper at
0x02006874 (calls 0x0200436E with value 1). ALNK reset, IOMAP CON5 mask,
PC0/PC2/PC1 setup, channel-3 configuration and DMA register selection remain
consistent with our driver. This does not prove PLL source, actual sample rate,
DAC output or analog gain. With no confirmed note events, silent audio alone
cannot isolate a DAC fault. Do not change clocks/pins based on silence alone.
The earlier statement that the audio driver was not enabled is historical:
Test 01 and Diagnostic 02 include it, and the owner reports increasing render
and audio IRQ counters. Raw scan changes are still the immediate hardware gate.

### Diagnostic 03 hardware result and Diagnostic 04 candidate (October 10)

Diagnostic 03 was installed with explicit approval, all 83 changed sectors
verified and the full 1 MiB readback matched the candidate. The owner sees the
raw-input display but reports no CHANGES increment when pressing keys. This
places the next investigation before contact interpretation; it does not yet
identify a defective physical component or prove all raw cells read FF.

A previously omitted stock initialization is now identified: at 0x02004EC8
Pro016 clears r1/r0, then calls 0x0200436E at 0x02004ECA. That GPIO output helper
clears OUT at 0x020043FA and DIR at 0x020043FE for pin 0 (PA0), after SPI starts.
The port had configured PA1/PA3/PA4 but never performed this PA0 step.
Diagnostic 04 adds only those two masked writes to scanner initialization,
retaining all scan timing, tables, ADC and audio settings and the raw display.
PA0's external role is not proven; output-enable is a hypothesis, not a finding.

Diagnostic 04 is built and privately packaged, NOT installed or hardware-proven.
Scanner, ADC integration and rawscan ASan/UBSan tests pass, as do 62 SMK Python
tests. Scanner regression initializes PA0 high/input and verifies it becomes
low/output without modifying an unrelated PA12 bit. Image packaging verifies
CRC round trips and protected data preservation; installer accepts the verified
Diagnostic 03 baseline and rejects changed protected data or unknown app bytes.
Private kit: recovery-backups/smk37-diag04-flash-kit (outside this repository).
Application SHA256: c0b2adcd6a061e0d4aa79178b7d74ff8ecd2b8a3f9a929abcad4a0a74efdec47.
Device image SHA256: 4badba4220afbdecd7ce5760812b81a4bbc36bbcc5ddddcb41e651df3205ea92.
82 application sectors differ from Diagnostic 03. No calibration/boot changes.

Recovery procedure correction: OK rekey only acknowledges reboot. Confirm the
COM5 console actually reports KEY: sending and increasing packet counts before
asking the owner to power on. Two premature attempts booted Felucca normally;
waiting for the actual key signal successfully entered UBOOT for Diagnostic 03.

### Diagnostic 04 hardware success; analog diagnostic prepared

Diagnostic 04 was explicitly approved and installed, with 82 changed sectors
and complete final image verified. The owner reports different audible notes
from the piano keys and digital CHANGES activity for keys, the pad-bank group,
right soft buttons and top knobs. This is the first confirmed physical key/audio
success in the port. The PA0 initialization was the only scanner configuration
change. Its exact external electrical function remains unproven. Exact key
pitches, release behavior, individual button/encoder mapping and velocity have
not all been independently checked.

The owner reports no digital CHANGES for pads, faders, pitch or modulation.
That counter observes digital rows only; it does not measure ADC activity.
Do not classify those analog controls as failed solely from that observation.

Diagnostic 05 retains the working hardware initialization and synthesis, but
displays all 24 ADC electrical indices (01..24, down three columns), current
10-bit values, completed sample count and an explicit ADC TIMEOUT state.
Unsampled indices show dashes. An ISR-owned observer preserves low/high ranges
and marks excursions of at least 16 counts with a sticky star, so pad hits can
be visible even when they end between LCD refreshes. LAST identifies the latest
16-count movement from its per-input anchor; RANGE is that input's full range
since startup. These thresholds are for diagnostics, not calibrated pad triggers.
Noise can trigger them; none of the displayed indices is yet a physical name.
No analog values are routed to synthesis, and no calibration is written.

Analog observer and ADC/scanner integration pass ASan/UBSan tests; target build
and image CRC/protected-data checks pass. Diagnostic 05 is prepared, NOT flashed.
Private kit: recovery-backups/smk37-diag05-flash-kit outside the repository.
Application SHA256: 79611ed3deaf69034c6f6a2466665ca2be0d92e84bb137f200e1079cd06b0196.
Device image SHA256: 8e256e0381045a21ca811f907202abb6bc2cfa202a458e6b23dc78b614cb30f9.
82 sectors differ from the owner-confirmed Diagnostic 04, which remains archived.
Next hardware check: SAMPLES advancing, then pitch alone (both endpoints and
center), modulation alone, each fader, and each pad separately. Capture multiple
reacting indices rather than guessing physical identity from the largest value.

Diagnostic 05 was subsequently explicitly approved and installed; all 82 sectors
and the complete image verified (session-install-20261010T230636.944641Z in its
private kit). Owner reports LAST 17 while moving pitch, then LAST 23 when holding
the wheel at either end or center, with displayed RANGE reported from 64 to 242.
This confirms analog observer activity, not a completed physical mapping. LAST
is the latest threshold crossing, and RANGE is that index's accumulated high-low
since startup; neither is the pitch value or a verified endpoint measurement.
Input 17 is ADC channel 2/mux 4; input 23 is channel 2/mux 6 under the current
schedule. Need simultaneous raw grid values at held wheel positions to distinguish
pitch response from background variation or settling/crosstalk. Do not map pitch
to 23 just because it is the last index displayed, or change timing without more
evidence. No additional firmware changes or hardware operations for this report.

### Diagnostic 05 physical analog readings (owner, October 10)

The center-position photo WIN_20261010_16_14_01_Pro.jpg shows input 17 at 0528
and footer LAST 23 RANGE 64. Earlier values 64/242 were footer observations,
not pitch endpoints; do not use them for pitch calibration. Owner then reports
input 17 fully down 404 and fully up 639, with released center 528 from photo.

| Physical control | Diagnostic index (1-based) | Down | Up | Evidence status |
| --- | --- | --- | --- | --- |
| Pitch | 17 | 404 | 639 | Owner identifies index; center 528 in photo |
| Modulation | 18 | 630 | 411 | Owner explicitly confirms row 18; reversed endpoints |
| Fader 1 (leftmost) | 22 | 0000 | 1023 | Owner reports index and endpoints |
| Fader 2 | 19 | 0000 | 1023 | Owner corrected upper endpoint to 1023 for all faders |
| Fader 3 | 20 | 0000 | 1023 | Owner corrected upper endpoint to 1023 for all faders |
| Fader 4 | 21 | 0000 | 1023 | Owner corrected upper endpoint to 1023 for all faders |

These are observations on this unit, not universal calibration constants.
No firmware mappings, stored calibration or new flash writes were applied from
these reports. Pads, unused analog inputs, jitter, response curves and repeatable
wheel endpoints remain to validate before declaring analog controls complete.

Owner then tested the top-left pad (input 04), followed by the remaining pads
in the requested left-to-right row order. Complete 1-based diagnostic input
sequence for physical pads 1..16 in that test order:

`04, 01, 02, 03, 05, 07, 08, 06, 12, 09, 10, 11, 13, 15, 16, 14`.

All inputs 01..16 appear exactly once. This establishes reported input identity,
not pressure/velocity calibration, peak response, or desired MIDI notes. No pad
endpoint values were reported. Owner subsequently confirmed modulation row 18;
its reported reversed endpoints are 630 down / 411 up.

The owner's physical-pad-to-input sequence exactly matches the inverse of the
stock permutation at 0x02058540, after converting zero-based stock indices to
one-based diagnostic indices. This independently supports the pad ordering.
Physical assignment is now reported for 16 pads, 4 faders and both wheels;
indices 23/24 remain unassigned and must not be routed as musical controls.
Before defining pad trigger thresholds, obtain input 04 at rest and under light
and firm pressure; neither its physical assignment nor the sticky activity
marker establishes a suitable velocity/pressure threshold.

### Playable Test 06 candidate (October 10)

Owner reports top-left pad/input04: 0 untouched, about 350 lightly pressed and
960 at the hardest press. This is one pad's observation, not an all-pad response
calibration. Test06 adds an explicitly provisional trigger at 64, release at 32,
and up to four ADC conversions of peak capture, with peak960 mapped to velocity
127. Short taps preserve both attack/release events. A separate bounded queue
isolates pad edges from keys/encoder events; overflow or ADC timeout releases pad
owners. Pressure aftertouch, calibrated velocity and Bank2 behavior are not added.

Physical pads use the confirmed inverse stock permutation and stock bank1 note
order on the selected Felucca part. They are local note owners 37..52, distinct
from the 37 piano keys, so a pad and key sharing a note do not release each other.
Release targets the original part even after selection changes. This is selected
sound playback, not an automatic switch to a drum engine.

Pitch uses this unit's 404/528/639 observations, a provisional +/-4 ADC-count
center deadband and +/-2 semitones. MOD18 uses the observed reversed 630..411
range through the existing modulation matrix (audible effect requires routing).
Wheels affect the selected part and parts with held local notes; incoming MIDI
can subsequently replace expression. Local wheel output over USB is not added.
Faders22/19/20/21 control parts1..4 at 0..127 after movement; their initial sample
does not overwrite preset volume. Continuous controls use latest-value snapshots,
not the note-edge queue. Synth mutations occur only in the audio consumer.

Test06 starts on the sound page again. Existing provisional encoder navigation
retains the analog diagnostic as an extra page. PA0 initialization, scanner/ADC
hardware setup, audio setup and factory trim are unchanged from Diagnostic05.
No data is saved persistently. This is a private bench candidate, not a release.

Validation: analog response ASan/UBSan; actual synth ASan tests for 37 keys,
16 pad owners, overlapping key/pad notes, release across track selection, wheel
endpoints/center and fader takeover; all engine/preset checks; 62 SMK Python
checks; shared MIDI/clock regression; target compile; image CRC/protected-data
checks; exact Diagnostic05 baseline and corruption rejection; stock restore plan.
Shared MIDI regression initially used the wrong generated-header include path;
rerun with build/gen passed. No physical Test06 validation or flash yet.

Private kit: recovery-backups/smk37-test06-flash-kit outside the repository.
Application SHA256: 2ce6e6cab1b209e15dc696988f075345fbadf7f3962dd826023c578d7255dd34.
Device image SHA256: 0f0b52a2bd794c5f4e8606f62af9826f4a1db43facf64f79505863bbb222a98a.
83 application sectors differ from verified Diagnostic05. Original boot and all
bytes from 0x9D000 onward remain unchanged. Owner approval required before flash.

Test06 subsequently received explicit owner approval and was installed in session
`session-install-20261010T234344.930377Z`. All 83 changed sectors and the full image
verified. Owner answered "yes to all" to these physical checks: a held piano note
bends with the pitch wheel and returns to normal; a tapped pad plays; fader1
changes volume while playing. These establish initial playable analog integration,
not completion of all controls. All-pad triggering/release, velocity consistency,
faders2..4, audible MOD routing, button behavior, LEDs and persistence still need
validation or implementation. No subsequent flash was performed for this report.

Further Test06 owner feedback: "I pressed them all sounds good" confirms playback
from all 16 pads. It does not separately quantify velocity sensitivity, double-hit
rejection or release timing. Owner then answered "yes" to checking faders2..4 on
their corresponding selected tracks. All four part-level faders now have positive
physical confirmation. Audible MOD matrix routing remains the next focused check.

Owner also confirms top knob4 selects sound pages (observed ARP/SLICER/CHORD),
then successfully sets MOD1 source MODW, destination PITCH, amount +20. Report:
"Mode wheel pitches up when raised". This verifies audible local MOD routing;
return-to-zero was not separately reported in this test. SRC1 can be reset OFF
to remove the temporary test assignment. Test06 now has positive physical checks
for piano playback, pitch bend/recenter, all16 pads sounding, all4 fader levels,
MOD routing and knob4 page selection. This does not certify full velocity curves,
pad aftertouch, every encoder/button, LEDs, pedal, storage or full stock parity.

### Test 07: button identification and lighting candidate (October 10)

Owner explicitly requested exercising lighting alongside the remaining controls.
This candidate retains Test06 sound/input behavior and starts on BUTTON CHECK:
18 electrical button IDs, held state, sticky seen markers, separate press/release
counts, last press retained after release, and last encoder ID/direction. Button
names/actions are deliberately not guessed. E4 advances to LIGHT CHECK, then MIX.
E1/E2/E3 do not change track/engine/preset while on either new diagnostic page.

LIGHT CHECK starts ALL OFF. E1 selects OFF, PAD1..16 or INDICATOR1..19; E2 selects
R/G/B for a pad; E3 sets pad level1..8 of255 (default4). Only one output is selected.
Indicator outputs have fixed color and 25% duty. Exiting the light page requests
all off on the next 250ms service pass. No automatic sweep is included. Physical
locations and observed colors still need owner confirmation, including whether
all 19 stock indicator outputs correspond to populated visible lights.

Stock Pro016 evidence:

- RGB setter `0x02004FB0` and table `0x0205E7A0`: pad-to-chain indices
  `0,15,14,13,12,11,10,9,1,2,3,4,5,6,7,8`; packed green/red/blue bytes.
- Serializer `0x02029454..0x020294AA`: 36 zero bytes followed by16 24-bit colors,
  MSB first, encoded byte `F0` for1 and `C0` for0; total420 bytes.
- SPI1 setup `0x02004FD4` and transfer `0x0201B412`: routeB via IOMAP bit4,
  PC10 low/output, baud8. LCD uses routeA/baud4. New synchronous main-loop
  transfer keeps LCD CS high, leaves IRQs enabled, bounds DMA polling, stops DMA
  before restoring route/baud/control on success or timeout. No PC9 digital
  output is enabled: PC9 remains an ADC input. A timeout disables further LED
  attempts and indicator output; it does not initiate flash writes or resets.
- Indicator table `0x020586BC` contains19 bank/bit codes:
  `0C,12,00,10,0D,1D,02,01,09,08,0B,0A,11,19,18,1A,1C,1E,1B`.
  Stock `0x0201C3D2..0x0201C466` builds active-low shift masks and alternates
  common GPIO150/151. Their helper `0x020042DE..0x0200430E` sets GPIO mode
  at `0x16A0C` and output bits17/18; these are separate from USB0 at `0x51000`.
  New scanner code uses those same outputs, one at a time at1/4 duty, blanks both
  commons before the latch edge, then enables the bank for the completed word.
  The pending bank follows the DMA pipeline rather than the next queued word.
  LED masks do not touch analog mux bits3..5 or key-select bits16..31. Scanner
  stop/fault blanks both commons. Actual timing/brightness is not yet measured.

Host evidence: ASan/UBSan tests decode every pad/color/level combination, validate
both permutations and forbidden indicator bits, verify SPI restoration on success
and timeout, exercise each indicator through the scanner pipeline and verify
USB0/ADC preservation. Button history tests cover all18 IDs and simultaneous holds.
Real synth ASan tests include every engine/preset and diagnostic navigation bounds;
62 SMK Python tests passed. Target application build passed at341940 bytes.
Offline install/restore simulation and baseline/candidate corruption rejection
passed; encrypted image CRC and all protected stock bytes verified unchanged.

Private kit: `recovery-backups/smk37-test07-flash-kit` outside the repository.
Application SHA256: `11a93efbb594c41c49d988c35aee625c59a9394cf632d73e55ef82699cf53406`.
Device image SHA256: `9924e106702d535fde2adf83db8c6b816a258c4324a3e2b66ce0e81f92987778`.
84 sectors differ from verified Test06. Original boot/calibration preserved.
**Prepared only: no Test07 device access, flash, physical validation or publication.**
Next: owner approval for this image, verified recovery entry/preflight, install and
full readback, then one-button naming and one-light/color observations. Check that
playing piano/pads and moving wheels/faders still works while lighting is active.

Test07 subsequently explicitly approved by owner ("aproved, connected and off").
Recovery-key transmission verified before power-on; owner reported black screen.
Exact Test06 baseline/identity/protected-data preflight passed. Installed in
`session-install-20261011T000914.655245Z`: all84 changed sectors and complete1MiB image verified
against the pinned Test07 SHA256 above. No automatic reboot. Physical button/light
checks are pending the owner disconnecting recovery and booting normally.

Test07 physical RGB feedback: owner clarified pads light "one at a time as I
turned it", rather than simultaneously. Owner then confirmed PAD1 cycles through
matching red/green/blue and reported checking the remaining pads as well. Individual
selection and RGB colors on all16 pads now have owner confirmation. Brightness
was described as dim, consistent with the diagnostic cap. Indicator outputs,
button naming, and playback/analog coexistence under lighting remain unverified.

Test07 indicator mapping, owner report: INDICATOR1 lights KNOB BANK.
This identifies LED output1, not the button-input ID.

Test07 indicator mapping, owner report: INDICATOR2 lights FADER BANK.
This identifies LED output2, not the button-input ID.

Test07 indicator mapping, owner report: INDICATOR3 lights the "<" button.
This identifies LED output3, not the button-input ID.

Test07 owner completed the remaining indicator output identification:

| Output | Owner-reported light |
| --- | --- |
| 4 | > button |
| 5 | OCT- |
| 6 | OCT+ |
| 7 | ARP |
| 8 | NOTE REPEAT |
| 9 | SCALE |
| 10 | CHORD |
| 11 | GLOBE |
| 12 | BT |
| 13 | PATCH |
| 14 | PARA |
| 15 | FX |
| 16 | SEQ |
| 17 | SEQ PLAY (green) |
| 18 | SEQ PLAY (red) |
| 19 | SEQ (red) |

All19 indicator outputs now have owner identification, in addition to all16
RGB pads. Output16 was reported as SEQ without a color; do not infer green.
Output19 was explicitly SEQ (red). These are output IDs, not input button IDs.
Lighting-off on exit and concurrent piano/pad/wheel/fader operation remain to
check before treating lighting integration as fully hardware validated.

Test07 lighting coexistence check: owner answered "Yes. I think it's good.
Light go off when I leave" after being asked to play keys/pads, try wheels/faders
with a light active, then leave LIGHT CHECK. This is positive qualitative
playback/control coexistence feedback and explicit lights-off-on-exit confirmation;
not measured timing/current or a prolonged stress test. Button-input naming is
the next mapping step; LED output numbers must not be reused as button input IDs.

Test07 button input mapping: owner pressed/released KNOB BANK and reported
LAST PRESS B01. Confirmed input B01 = KNOB BANK (independently of LED output1).

Test07 button input mapping: owner pressed/released FADER BANK and reported
LAST PRESS B02. Confirmed input B02 = FADER BANK.

Test07 button input mapping, owner report: B03 = < button; B04 = > button;
B05 = OCT-; B06 = OCT+. These are directly reported input IDs.

Test07 button input mapping, owner report in requested order: B07 = ARP;
B08 = NOTE REPEAT; B09 = SCALE; B10 = CHORD.

Test07 button input mapping, owner report in requested order: B11 = GLOBE;
B12 = BT; B13 = PATCH; B14 = PARA; B15 = FX.

Test07 final button input mapping, owner report in requested order:
B16 = SEQ; B17 = SEQ PLAY; B18 = SEQ REC. All18 button input names are now
owner-confirmed. This completes button naming, not implementation of actions.

| Input | Confirmed physical button |
| --- | --- |
| B01 | KNOB BANK |
| B02 | FADER BANK |
| B03 | < button |
| B04 | > button |
| B05 | OCT- |
| B06 | OCT+ |
| B07 | ARP |
| B08 | NOTE REPEAT |
| B09 | SCALE |
| B10 | CHORD |
| B11 | GLOBE |
| B12 | BT |
| B13 | PATCH |
| B14 | PARA |
| B15 | FX |
| B16 | SEQ |
| B17 | SEQ PLAY |
| B18 | SEQ REC |

LED output numbers remain a separate mapping: output18 was reported as
SEQ PLAY (red), output19 as SEQ (red). Do not infer their final semantic
assignment solely from matching button input numbers.

### Test08 direction correction and button integration (candidate only)

Owner clarified that the rotary knobs, not faders, decrement when turned
clockwise in Test07. Stock Pro016 decoder at 0x0201c33e..0x0201c3c0 increments
counter +6440 for triples 7,30,56,33 (00->01->11->10->00). The consumer at
0x0201c684..0x0201c688 computes counter +6436 MINUS counter +6440 before
queuing signed type-4 events. Our Test07 transition table had the opposite sign.
Test08 reverses that table once at the decoder, affecting both diagnostic signs
and all navigation/edit controls. Fader scaling remains bottom0/top1023.
This validates the code mismatch and agrees with the owner's physical report;
all eight knobs still need clockwise/counterclockwise confirmation after flashing.

Test08 connects the 18 confirmed button inputs to actions and adds normal status
indicators and pad-hit lights. See SOUND_CONTROLS.md for behavior and limitations.
Normal multi-light operation is new and has not been physically verified.
The application does not write persistent flash; recorded patterns are RAM only.
Host tests cover both electrical directions on all8 encoders, actual parameter
edits through the detent accumulator, navigation, bank changes, buttons, note
release ownership and record/replay. No Test08 device write has been performed.

Test08 offline validation completed: peripheral C suite (ASAN/UBSAN), actual
synth/button tests (ASAN), shared chord and MIDI/clock suites, and all62 SMK
Python tests passed. Jieli target linked successfully, 344132 application bytes.
Application SHA256: a0630175c7c0155a071fd07dda4593a7cd1ced8f890eee5d8b9b54c3a69ed3dd.
Private device image SHA256: 55eafbc5abf137a1aa17c72be5815eb1022f71d5c8b52749fba3268044f929f6.
The private Test08 kit changes85 application sectors from verified Test07.
Simulated install/restore and corrupt-image rejection passed; protected regions
and all non-application file contents remain unchanged. No hardware write,
publication or installation approval has occurred for Test08.


Test08 installed after explicit owner approval and black-screen recovery.
Read-only preflight matched verified Test07; all85 changed sectors verified,
then complete1MiB readback matched the pinned Test08 image. Session: session-install-20261011T003556.887150Z.
Protected regions retained. No automatic reboot. Physical Test08 behavior,
including corrected knob direction, remains pending normal boot and owner checks.


Test08 owner checks after normal boot:
- Knob5 waveform moved to SQR and owner confirmed counterclockwise returned to previous waveform; all8 directions not yet confirmed.
- Owner confirmed OCT+/OCT- octave change and return during the requested light check.
- Owner confirmed ARP on/off behavior during the requested indicator check.
- Owner confirmed NOTE REPEAT on/off behavior during the requested indicator check.
- Owner confirmed CHORD on/off behavior.
- Owner confirmed SCALE quantization on/off behavior.
These are qualitative owner confirmations, not measured timing or validation
of every knob. Bank switching, multi-light coexistence and RAM transport/record
still need their specific device checks.


Further Test08 owner checks:
- Owner confirmed KNOB BANK on makes knob1 edit the first parameter and off returns knob1 to track selection.
- Owner confirmed FADER BANK attack test: fader1 raised produces slower onset, lowered restores quick attack. Other ADSR faders not individually checked.
- Owner confirmed pad sound/blue lighting, including two pads together.
- Owner confirmed RAM recording, disarming, stopping and replaying a short phrase via SEQ/REC/PLAY.
These confirm the requested qualitative tests, not all8 encoder directions,
all4 ADSR faders, or prolonged timing/current behavior. No further flash performed.


Test08 final knob-direction check: Owner confirmed all8 knobs advance values/options clockwise and reverse counterclockwise: knobs5-8 with KNOB BANK off, knobs1-4 with KNOB BANK on, on PARA page.
This confirms the polarity correction physically for all eight knobs. It does
not establish exact detent resolution or rapid-motion reliability. No new flash.


Test09 prepared locally (not flashed): velocity curves, USB editor subset, RAM step editing and pad bank2. Sustain logic and A/B session persistence host-tested; physical pedal mapping and runtime flash reads pending. SMK37_ENABLE_SAVES=0; compiled erase/program helpers return -1. Owner reports sustain pedal available.

Private app: 361420 bytes, SHA256 2a3315751c21308d6427aee3f984e2f5f7e78a9cbd0440de0ad6f1e6100d71c3. Unit image SHA256 2debef1d478a5febfe40517e363016dc070f34d5beed86af85a5f760dabe9b9f. 89 application sectors differ from Test08; all protected/non-application bytes preserved. Kit: ../recovery-backups/smk37-test09-flash-kit. Simulated install/restore and artifact/source checksum verification pass. No serial/device access occurred while preparing Test09. New-image approval requested; not yet received.

Validation: peripheral C suites; actual synth/buttons/features with ASAN; 1338 C-generated responses parsed by browser JS; USB SysEx with OTA disabled; FM-1 chord/MIDI/clock/UART regressions; 62 SMK Python tests; keyboard and web tests (optional firmware package tests skipped); Jieli target build with call-free RAM read helper audit and write-stub audit. Local editor at http://localhost:8770/editor.html (localhost only). No publish/push/PR.


Test09 installation: owner explicitly approved, adapter connected/SMK off. First key entry missed (no writes); repeated key entry with SMK off succeeded and owner reported black screen. Exact Test08 baseline preflight passed. All89 changed app sectors and complete1MiB readback verified. Session session-install-20261011T010850.701968Z; image SHA256 2debef1d478a5febfe40517e363016dc070f34d5beed86af85a5f760dabe9b9f. Protected ranges unchanged. Normal boot and Test09 feature checks pending. No automatic reboot, publish or push.

Test09: owner confirmed normal boot and piano sound after instructions to disconnect recovery adapter. Velocity, USB/editor, step editing, storage reads and physical pedal mapping remain pending.

Test09 velocity: owner reports softest49, hardest121; confirms graded response, not full calibration. PEDAL CHECK: owner identified ADC23 and explicitly confirmed released871, pressed0, returning871. Owner says pedal polarity is reversed for another device; leave its switch in the measured position. Next Test10 source connects zero-based ADC22 to sustain with three fresh-sample debounce, hysteresis (press<=200/release>=650), release-to-arm on startup, audio-consumer ownership and USB CC64. No new flash; Test09 remains installed. Storage reads, USB editor and step/pad-bank checks remain pending.

Test09 runtime storage probe: owner reports READ OK / SAVES DISABLED. This confirms runtime JEDEC856014, CRC32 of the private556-byte trim record, and every byte FF in0xEB000..0xF3000 using RAM flash-read routines. No runtime flash write took place. This is read-path/allocation evidence, not an erase/program or power-loss hardware test.

Test09 USB editor: owner confirms direct USB Connect shows FELUCCA SMK37 TEST09 and sound controls load. Hardware-to-browser and browser-to-hardware edits are the next check.

Test09 two-way editor control: owner confirms PARA knob5 changes reach browser and same browser control changes reach SMK display. Test09 STEP EDIT: owner confirms pad step creation/clear, green lights and playback match entered steps. These are qualitative confirmations, not all parameters or stress tests. Pad bank2 and physical sustain behavior remain pending.

Test09 pad bank2: owner confirms changing banks with E6 on CONTROLS changes same-pad notes and produces no stuck note in the requested test.

Test10 SAVE private candidate prepared, NOT INSTALLED.365112-byte app SHA25690caa97bdc71d4b4a368e5a39345d2bdbf938026a1d2e91b23caa10671b865ae; full unit image5c03959b59d199d0b5752527c655387641bb02ff9aab230d1eb61f1292821daa;90 changed app sectors from verified Test09. Runtime writes opt-in and limited0xEB000..0xF3000 after ID/trim/allocation checks; four A/B slots; last valid saved generation resumes with transport stopped. Unknown data fails closed. Pedal ADC23 is debounced and release-armed. Peripheral/application suites, SAVE wire replies,62 Python tests, keyboard/web tests and target RAM-call audit passed. Separate save-only recovery kit built and pinned to stock/Test09/Test10 references; original application writer preserved. Owner approval for adapter read/capability verification then Test10 SAVE install requested, not yet received. No device writes in this preparation round.

Owner approved next hardware round (ok). Save-only XIAO UF2 hash1080c6f38aa6f4039327bfbb54d906836867e9e67d820dc0c21cb86ff2cc4cf9 copied to sole RPI-RP2 G: volume after BOOTSEL confirmation, with SMK requested off/disconnected. Adapter re-enumerated with verified serial4250305539333104 and correct save-only caps0xEB000..0xF3000. Status query before SMK recovery timed out; full recovery/read preflight still pending. No SMK flash writes; Test09 remains installed.

Save-only adapter physical read preflight PASSED after owner reported black screen. Tool verified adapter serial/caps, UBOOT/read-write status, chip identity and complete1MiB against pinned Test09 outside0xEB000..0xF3000. Save area is blank:0 sectors to clear. Dry run, zero writes. This proves the separate recovery read/capability path, not an erase/program/clear operation. Next restore preserved app-writer adapter then install approved Test10 SAVE.

After successful save-only recovery dry run, preserved application-writer UF2 SHA2567b5f596b41738baa652e123da60f37953f8974c0b96c061989f418d9badbf5a5 restored to XIAO with SMK requested off/disconnected. Original adapter serial and caps modelSMK37 writer1 min4000 end9D000 verified. Test10 approval recorded; new SMK image not yet written.

Test10 SAVE installation completed after owner reported black recovery screen. Exact Test09 baseline preflight passed; all90 application sectors and complete1MiB readback verified. Session session-install-20261011T013720.559732Z; image SHA256 5c03959b59d199d0b5752527c655387641bb02ff9aab230d1eb61f1292821daa. Bootloader and protected tail are byte-identical before/after. No automatic reboot. Normal boot, physical sustain, runtime save/load and power-cycle resume remain pending. Save-only physical erase/clear remains untested and is not authorized by this installation. No publish or push.

Test10 SAVE: owner confirms normal boot/audio and reports sustain appears to work out of the box. This records an initial qualitative physical sustain pass, not exhaustive retrigger/track/USB CC64 behavior. Runtime project save/load and power-cycle resume are next; no additional device access or flash in this turn.

Test10 SAVE persistence: owner confirmed PROJECT SAVED for slot1; changed the sound setting, loaded slot1 and confirmed saved setting returned. Owner then confirmed normal power-cycle restored saved sound with sequencer stopped. This establishes basic runtime project erase/program/load and startup resume on the device. Sequence contents, additional slots, replacement saves, USB project writes and physical save-only recovery clearing remain unverified. No host device access, firmware write or publication in this reporting turn.

Test10 SAVE: owner confirms a short recorded phrase saved in project slot2 survived power-cycle and played back correctly. Slots1 and2 now exercised. Replacement save in an existing slot, slots3/4, USB project writes and physical save-only recovery clearing remain unverified. No host device access or publication.

Test10 SAVE replacement: owner confirms changed tempo (requested90 BPM) saved over existing slot2 survived restart with phrase intact. Normal replacement-save path now physically exercised; this does not validate interrupted writes or recovery clearing. Test10 USB project saving and slots3/4 remain unverified. No host device access or publication.

Test10 USB project save: screenshot shows connected FELUCCA SMK37 TEST10 SAV, enabled Save controls, A/B used and C/D empty. Owner confirms Save beside C changes C to used. Browser project save to slot3 passed; browser load and slot3 restart persistence remain unverified. Version label is visibly truncated. No host device access or publication.

Test10 USB project load: owner confirms Load beside C restores the changed sound setting on both webpage and SMK. Browser project save/load in slot3 now passed. Main requested normal-use checks covered: device sound/sequence saves and restart resume, replacement save, velocity response, two-way USB editor, basic physical sustain, pad-bank switch and step editing. Remaining coverage includes slot3 restart/slot4, broader pedal/USB CC64 edge cases and physical recovery clearing/interrupted-write behavior. No new host device access, firmware installation or publication.

Test11 USB preparation: restored local front page at http://localhost:8770/ and prepared dedicated SMK normal-USB installer plus RAM-only loader. Candidate app373728 bytes SHA25699917be405b6d9c996d9bcc47a063b499a1fcfe700ca9c90c4070918b39c6632; blank-save image880f8238cc8581916d00dc515266327e2d46438c33689b057893e191744a4589. Save-preserving bootstrap/rollback kit prepared;92 application sectors differ from Test10. Loader/C/browser/bootstrap regressions and final local page/package checks passed. NO DEVICE ACCESS OR FLASH; Test10 still installed. User requested normal USB like FM-1; first adapter bootstrap and later real USB write remain pending. See USB_INSTALLER.md.

Test11 adapter installation completed following user readiness (connected/off, then black). XIAO initially in BOOTSEL; restored hash-verified application-writer UF2 and verified serial/caps. Actual KEY sending0x16EF confirmed before owner power-on. Read-only exact-Test10 app/protected-data preflight passed. Full current backup saved, all92 changed app sectors verified and complete1MiB readback matched. Session session-install-20261011T021640.356540Z; installed image SHA256 c7d1515efadef13de96d02a712ecae6d7bb874ea5ab5ca72b8bc11bbc4ed2290. Boot head and entire tail including0xEB000..0xF3000 project bytes unchanged. No automatic reboot. Normal boot/audio/project preservation and actual direct-USB update still pending. No publish/push.

Test11: owner confirms normal boot, piano sound and saved project restoration after adapter install and instructions to connect normal USB. Direct-USB loader handoff, same-version readback and a distinct-build erase/program test remain pending. No device access in this reporting turn.

Test11 normal-USB same-version installer: owner reports worked after following browser installation steps. This is owner confirmation of the flow, not an independently captured USB trace. Identical app sectors are skipped, so distinct-build erase/program is still pending. Preparing Test12 with version labels changed to exercise real writes.

Test12 USB candidate prepared after owner confirms Test11 same-version install worked. Version labels/identities changed;4 encrypted flash sectors differ. Jieli build/RAM audit, C updater regression, browser tests and actual Test11 saved-image/Test12 package simulation pass; saved tail preserved. Package SHA25699d7c3d75d39706abb0d32463defba0e5c46ff73041dd5282ed982a6fe60fcec; app7bbcacffc1024a85f713a238d8d9f3387a8ef2975c94a324837a5939e276ec2c. Private kit and intact-image Test11 rollback prepared. Staging locally only; NO hardware write yet.

Test12 direct USB installation: owner explicitly confirms final Test12 installed status, normal device restart and saved project retention after instructions to install the distinct candidate. Four application sectors differed from Test11, so this exercises actual USB erase/program rather than a same-version no-op. This is owner-reported hardware evidence; no independent browser log or adapter readback captured in this turn. Basic private normal-USB installation is now validated on this unit. Public redistribution, stock first-install, power-loss recovery and wider-device validation remain separate gates. No push or public publication.
