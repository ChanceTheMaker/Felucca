# FM-1 and SMK-37 processing and memory

Evidence reviewed October 10, 2026. This compares the FM-1 with SMK-37 Pro/Elite
stock packages and our current Felucca port; it does not infer more processing
power from the larger keyboard.

| Resource | FM-1 | SMK-37 Pro / Elite | Evidence / qualification |
| --- | --- | --- | --- |
| Processor family | JieLi AC791N / AC79 | Same family | All three inspected vendor packages identify AC791N. Exact silicon/package identity is not established by that string alone. |
| Boot-configured system clock | 360 MHz | 360 MHz | Byte-identical embedded boot configurations; SYS_CLK=360000000. This is a requested boot clock, not a runtime measurement. |
| Reference oscillator | 24 MHz | 24 MHz | OSC_FREQ=24000000; SMK board photo also shows a 24.000 crystal. |
| Main SRAM address window used by Felucca | 512 KiB | Same 512 KiB window | 0x01C00000..0x01C80000; both builds use firmware/app.ld. Includes code, stacks and reserved boot state. A linker map is not a destructive full-memory hardware test. |
| Current application RAM + buffer budget | 432 KiB | 432 KiB | 72 KiB RAM plus 360 KiB POOL in app.ld. Budget, not free memory. |
| Flash | 1 MiB | 1 MiB on the tested Pro | FM-1 driver expects JEDEC 0x856014; Pro runtime probe returned that ID and two matching 1 MiB stock reads. Elite's matching layout is package evidence, not a separate physical read. |
| External SDRAM enabled at boot | No | No | Shared configuration has ENABLE_SDRAM=0. This does not identify every possible package option. |

JieLi's [AC79 family documentation](https://doc.zh-jieli.com/AC79/zh-cn/master/getting_started/overview/index.html)
lists 578 Kbytes of total on-chip SRAM and a nominal maximum clock of 320 MHz.
That family SRAM total includes multiple regions; it is not 578 KiB of freely
allocatable application memory. The documented family clock specification and
the vendor's 360 MHz boot request are distinct facts. Do not describe either as
a measured running clock. SMK stock disassembly also contains a routine passing
24 MHz and 240 MHz to clock configuration; its presence alone does not prove the
normal runtime frequency. Our SMK startup preserves the inherited CPU PLL state.

There is no demonstrated CPU-speed or RAM-capacity advantage for the SMK over
the FM-1. Its extra keys, pads, faders, wheels and controls are the practical
hardware advantages. Board-specific scanning, LED, audio and MIDI wiring and
different flash allocations require a separate firmware target despite the
shared computing platform.

## Reproducible evidence

- [Stock package comparison](PACKAGES.md) and [flash/boot configuration](FLASH_LAYOUT.md).
- [Startup/RAM analysis](STARTUP_ANALYSIS.md) and [board photographs](BOARD_PHOTOS.md).
- [Device test record](CONTROL_TRACE.md): Test09 JEDEC/trim/storage read probe.
- `firmware/app.ld`, `firmware/hal/fm1_flash.h`, `tools/build_smk37.py`.
- Private local research `../SMK37-research/FINDINGS.txt` and `comparison.json`:
  FM-1 015, Pro 016 and Elite 016 have byte-identical 14,384-byte bootloaders,
  699-byte embedded configurations and 19,969-byte OTA loaders. Vendor binaries
  and unit backups remain private and are not redistributed in this document.

To resolve actual CPU frequency, add a read-only clock-register diagnostic with
the matching AC79 divider interpretation, or measure CPU cycles against the
24 MHz reference. Do not change PLLs merely to establish the comparison.
