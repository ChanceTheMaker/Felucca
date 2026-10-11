# SMK-37 Pro 016 startup analysis

Offline analysis, October 7, 2026. Source branch begins from the SMK interface
research stack based on Salt RC1 `7b1164abd2322c279faf9bb57ae22283466b9f53`
and upstream v1.0.1 `20c275e39f75fa820978032efaceddfc5283c8cb`.
No upstream changes, Bluetooth code, firmware drivers or update commands were
introduced. This document records evidence, not a flashable port.

## Method and identities

The previously decoded local `app.bin` files were wrapped in minimal ELF32
containers for the installed JieLi pi32v2 disassembler. The ELF machine value
241 was taken from a locally built Felucca ELF. The assigned XIP base is
`0x02000120`, matching the package's declared entry. These containers are for
analysis only; they must not be installed or executed.

| Decoded application | SHA-256 |
| --- | --- |
| Pro 016 | `0f64fbf6ffc454eea686d78cf89f35e593229765bdcf166f10a8bd714abcb09a` |
| FM-1 015 | `306e47065f35d7a7a05ada7f5dd092f6e770952054f33f0a86b75fd10ffe3203` |

Local listings and computed values live under `build/research/startup/`.
Linear disassembly across the full file includes data and relocated RAM bytes;
only traced executable regions should be used as code evidence. A decoded
instruction in an arbitrary data region does not prove an executed operation.

## Entry-path differences

The entry path contains explicit stack loads, a zero-fill loop and a RAM-copy
loop. Immediate values below were checked against their instruction bytes and
the copy's source bounds were checked against the decoded application length.
Intervals are half-open. RAM addresses are CPU addresses, not flash offsets.

| Entry-path operation | Pro 016 | FM-1 015 |
| --- | --- | --- |
| Initial SP | `0x01C3A464` | `0x01C16FD0` |
| Initial SSP | `0x01C3B464` | `0x01C17FD0` |
| Zero-fill start | `0x01C09A18` | `0x01C0A05C` |
| Zero-fill size | `0x3CCA4` (248996 bytes) | `0x195C0` (103872 bytes) |
| Zero-fill end | `0x01C466BC` | `0x01C2361C` |
| RAM-copy destination | `0x01C00000` | `0x01C00000` |
| RAM-copy source in XIP | `0x0208DBA0` | `0x02084060` |
| RAM-copy size | `0x9A18` (39448 bytes) | `0xA05C` (41052 bytes) |
| RAM source offset in app.bin | `0x8DA80` | `0x83F40` |
| Later indirect dispatch target | `0x020020A2` | `0x02001C32` |

The stack loads are at `0x02000124` / `0x0200012A`; the zero-fill setup is
at `0x02000136` / `0x0200013E`; RAM-copy setup is at `0x0200014C` through
`0x02000158`; dispatch is loaded at `0x020001B0`.

This supports a shared startup convention with different linked layouts.
It does not establish available heap, total fitted RAM, or permission to reuse
Felucca's complete linker map. The RAM copy includes executable/data content;
it cannot all be labeled initialized data without further analysis. Calls into
that region require disassembly at its RAM destination, not its file/XIP address.

A routine near `0x02000302` loads 24000000 and 240000000 as arguments before
calling a PLL/register-manipulating routine. This is a clock-configuration lead,
not a measured operating frequency or proof of its invocation during normal boot.
The boot config's 360 MHz request must not be substituted for a runtime measurement.

## Felucca source boundaries that require actual changes

| Boundary | Current FM-1 assumption | SMK requirement / missing evidence |
| --- | --- | --- |
| `firmware/hal/fm1_input.h` | 11-column/6-row matrix, 27 note keys, 7 encoders; `uint32_t notes` and press-edge mask | 37 keys cannot fit a 32-bit mask; need velocity events, 8 encoders, pads and pressure; scanner wiring remains unknown |
| `firmware/src/ui_input.c` | Repeated 27-key loops and FM-1 key/LED layout | Separate SMK input/layout adapter; do not globally rename or extend FM-1 constants |
| `firmware/hal/fm1_adc.h` | Only master pot and battery channels (PB6/PB1) | Four faders, two wheels and pedal plus battery; channel/multiplexer/calibration map unknown |
| `firmware/hal/fm1_lcd_hw.h` | SPI1, PC7-10, PA2 active-low backlight | Similar-looking screen does not establish pins, controller or backlight polarity |
| `firmware/hal/fm1_audio.h` | ALNK0 with specific PC0/1/2/6 codec controls | Verify the fitted DAC and clock/data/mute wiring on this board |
| `firmware/app.ld` | FM-1 RAM sections, stacks and reserved boot area | Validate SMK RAM/boot reservations before allocating buffers or moving stacks |
| Flash / loader | Save base `0x97000`, loader application end `0x93000` | Both conflict with stock SMK application extent; independent storage/update layout required |

The useful next binary step is to map the copied RAM region and follow call
references to GPIO, LCD, ADC and audio initialization, separating shared SDK
routines from board-specific callers. Record candidate pins as hypotheses until
corroborated by board photographs or measurements.

## Photographs and reproducible RAM mapping

The owner has now supplied internal photographs, including readable control
ICs and keybed connections. See [Board photographs](BOARD_PHOTOS.md). Further
disassembly is not requested for this stage. Recovery and calibration/user
storage preservation remain unresolved.

`tools/smk37_startup.py` accepts only the SHA-256-pinned decoded Pro 016
application. It checks the entry instruction template, copy bounds, contiguous
RAM/zero-fill regions and stack bounds before writing an analysis-only ELF.
The ELF has no program headers and no executable entry point. It is not a
firmware package. `.text` retains XIP addresses before the copied region;
`.ram_copy` maps the 39448 copied bytes to `0x01C00000`; `.tail` retains the
remaining file bytes at their XIP addresses, without an executable flag.
RAM content still mixes instructions and data: successful disassembly alone
does not establish code reachability or a hardware function.

```powershell
python tools/smk37_startup.py E:/AI_Apps/Felucca/SMK37-research/pro016/app.bin --out-dir build/research/startup
python -m unittest discover -s tests -p 'smk37_*_test.py'
```

The layout JSON and mapped ELF remain under ignored `build/research/startup/`.
The mapped listing was generated successfully with the installed JieLi
disassembler under WSL. [Control tracing](CONTROL_TRACE.md) records the first
scanner/display candidates and the resolved startup call into copied RAM.
Tests independently verify ELF section names, addresses and original bytes,
absence of loadable program segments, truncated input rejection and malformed
instruction/layout rejection.

## Reserved boot state and software update entry traced

Follow-up static tracing identifies shared boot conventions directly in Pro016:

- `0x020001E6` copies incoming boot parameters into `0x01C7FD50`. Incoming
  words at +4/+8 are copied to +16/+20 of that structure, consistent with the
  SDK's physical SFC base and CPU application address fields. Flash size is
  read from the referenced flash header; the chip key, trim value and MAC
  fields are also copied. Preserve this area before initializing application RAM.
- IRQ registration at `0x0200164C` uses the vector base `0x01C7FE00` (load at
  `0x0200167C`). This is a stock SMK reference, not an inferred FM-1 address.
- RAM function `0x01C037F4` disables interrupts and copies 16 bytes from
  application address `0x0205E5D1` to `0x01C7FD80`. The bytes are exactly
  `usb_update_mode\0`. It then calls the P33 bit-field helper at `0x01C0108C`
  with register 0xA0, bit position 4, width 1 and value 1 before looping.
  That helper performs a read/modify/write through the P33 access routines.

These findings support a future SMK software update-entry implementation and
protecting the top-of-RAM boot/vector region in the linker script. They do not
establish successful forced recovery after an early boot failure, nor prove
every byte of the proposed application RAM allocation is usable. No update
entry or reset command was sent to the device during this tracing.

The manufacturer's [AC79 documentation](https://doc.zh-jieli.com/) describes
578 KiB total on-chip SRAM for the family, including multiple memory regions;
that total must not be treated as a single contiguous application allocation.
The observed stock addresses and reserved regions remain the relevant evidence
for the SMK linker map.

## Integrated startup safeguards (October 8)

The memory initializer zeros only BSS/pool and copies data/RAM-text from the
linked image. Host tests cover empty sections and untouched adjacent words;
the pi32v2 output contains inline loops followed by `csync`. RAM-text is empty
in the current image, so execution/cache behavior of future RAM routines is
not established. Reset-source and pre-arm watchdog snapshots are saved across
BSS clearing rather than being erased, and P33's timeout counter is explicitly
initialized before those early reads. These changes do not validate the
physical RAM allocation or SPL handoff.

The separate application now preserves the stock SPL parameter layout before
clearing application memory, clears the update mailbox after IRQ initialization,
and initializes TIMER4 to OSC /1 with a full-width period. This avoids inheriting
a different prescaler or short timer period merely because its mode/source bits
match. It deliberately leaves CPU/PLL clock configuration untouched pending
evidence of the actual handoff state.

`smk37_bootguard.h` tracks startup stages in `.noinit`. After two failed warm
starts it requests the traced software-update path before starting USB, display,
scanner or audio. A healthy main loop with recent scanner/audio progress after
30 seconds clears the pending-start flag; intentional update entry also clears
it. The host tests cover retry thresholds, persistent fallback selection,
cold-start reset, invalid retained state and successful/intentional transitions.
The screen shows the previous boot stage for later device testing.

This policy cannot establish physical SRAM retention, watchdog/P33 operation,
or a usable ROM/update USB connection. Those remain device tests, alongside a
recovery route independent of successful application startup. Read-only Windows
USB and MIDI enumeration initially found no matching connected device on
October 8. The subsequently reconnected stock unit enumerates all three MIDI
inputs and USB 4353:B64B successfully (see ARRIVAL_RESULTS.md). The owner confirms
the recovery parts have not arrived and has explicitly requested no flashing.
