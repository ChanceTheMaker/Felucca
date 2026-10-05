# SMK37 stock flash and boot evidence

The stock Pro and Elite v16 packages share their boot configuration and region
descriptors, but contain different application binaries. The FM-1 storage
addresses cannot be reused: its main save area begins at `0x97000`, inside the
SMK Pro application's span in the decoded flash payload.

This extends the [package comparison](PACKAGES.md). The complete
[layout inventory](layout-inventory.json) records 13 packages from research
revision `ab85d7340163ba9e171dc9396e31c034829bbc02`. No vendor executable bytes
are committed. All values below come from those local package inspections;
they have not been validated on the arriving keyboard.

## Version 16 application

| Field | Pro | Elite |
| --- | --- | --- |
| Application directory base in flash payload | `0x4000` | `0x4000` |
| Declared CPU entry point | `0x02000120` | `0x02000120` |
| Application file start in flash payload | `0x4120` | `0x4120` |
| Application file bytes | 619704 | 621320 |
| Flash payload bytes | 643072 | 643072 |
| Bootloader file bytes | 14384 | 14384 |
| Chip-key value | `0x980F` | `0x980F` |
| Flash header size field | `0xFF000` | `0xFF000` |

Pro's application file occupies `[0x4120, 0x9B5D8)` in the flash payload.
The entry point matches Felucca's current XIP entry value, but that alone does
not validate its startup code, RAM layout or boot mailbox addresses. Application
hashes differ; the decoded `cfg_tool.bin` file hashes match. No conclusion
about the purpose of the application differences follows from their sizes.

## Boot configuration

The config blob and its chip-key prefix pass their CRC checks. Records following
the key prefix use a length byte, a NUL-terminated key, and the specified value
bytes, consistent with the [JieLi format research][format]. The inspector
preserves raw values and shows four-byte values as little-endian integers.

| Record | Stock Pro and Elite v16 value | Bring-up use |
| --- | --- | --- |
| `OSC` | `OSC0` | Oscillator selection lead |
| `OSC_FREQ` | 24000000 | 24 MHz configuration; check the board crystal |
| `SYS_CLK` | 360000000 | Requested system clock; measure runtime clock/timing |
| `HSB_DIV` / `LSB_DIV` | 1 / 2 | Preserve as config values until divider semantics are checked |
| `ENABLE_SDRAM` | 0 | Do not assume external SDRAM is available |
| `SPI` | `4_1_0_0` | Decode with the matching SDK and board wiring |
| `RESET` | `PB01_08_0` | Configuration string, not a verified recovery procedure |
| `UPDATE_JUMP` | 0 | Configuration value only |

These are bootloader inputs. Runtime firmware may reconfigure clocks and
peripherals. Disabled external SDRAM does not establish usable internal RAM.
The [JieLi SDK documentation][sdk] is the reference for interpreting settings;
its development-board pinout is not an SMK pinout.

## Region descriptors

Both v16 models declare the following raw fields in the application directory.
These are **descriptor offsets**, not validated physical erase addresses. Keep
their coordinate system explicit when implementing storage. Some entries are
reserved regions and are not backed by bytes in the update payload.

| Name | Raw offset | Raw size |
| --- | --- | --- |
| VM | `0x9D000` | `0x23000` |
| PRCT | `0x00000` | `0x9D000` |
| BTIF | `0xC0000` | `0x1000` |
| USRTRIM | `0xC1000` | `0x1000` |
| USRFLASH | `0xC2000` | `0x29000` |
| USR | `0xF4000` | `0xA000` |

The root directory separately declares `key_mac` at `0xFF000`, size `0x1000`.
Neither that entry nor the flash-size header proves the physical device's
capacity. Region names suggest possible trim/user-data roles but their actual
contents and preservation requirements must be established through recovery
research and hardware observation.

Felucca currently writes its main FM-1 store from `0x97000` (`fm1_flash.h`), and
its loader writes `[0x4000, 0x93000)` (`ldr_core.c`). The stock SMK application
extends beyond both addresses. Implement a separately verified SMK storage and
update layout before enabling persistent saves or OTA. The current reserved
`--board smk37` target continues to refuse package generation.

## Reproduce and validate

```sh
python tools/smk37_layout.py path/to/SMK-37_Pro_016.fwsc path/to/SMK-37_Elite_016.fwsc
python -m unittest discover -s tests -p "smk37_*_test.py"
```

The CLI reads packages into memory and prints only JSON metadata. It checks
the outer package, offset-zero flash header, directory headers, boot config,
chip key, first application block and application-file CRCs. It bounds entry
counts and rejects out-of-range or overlapping application files and files
overlapping directory records. It does not decode later resource directories,
execute bootloaders or validate hardware recovery. Unknown formats fail instead
of being treated as a new flashable target.

Eleven layout/config tests cover valid synthetic data, both FWSC wrapper lengths,
truncation, corruption, misplaced application bases, invalid file bounds and
malformed/duplicate config records. All 13 archived packages pass these checks.
The combined preparation suite has 30 passing tests. No firmware build or
hardware write is part of this round.

## Board-photo leads

The pinned research archive's [internal photographs][photos] show a keybed
ribbon, wheel wiring and a JieLi-marked main IC. They are useful for planning
arrival photographs and identifying connectors. The photographs do not supply
a complete pin map, LCD-controller identification or verified RAM capacity.
Record the arriving board revision before applying observations from another
unit. Pad scanning, ADC channels, DAC routing and display wiring remain open.

[format]: https://kagaimiq.github.io/jielie/datafmt/newfw.html
[sdk]: https://doc.zh-jieli.com/AC79/zh-cn/release_v1.2.0/
[photos]: https://github.com/jonathaslacerda/smk-37-pro-docs/tree/ab85d7340163ba9e171dc9396e31c034829bbc02/images/smk37pro
