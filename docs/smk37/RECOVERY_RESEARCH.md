# Recovery tooling evidence — October 8, 2026

No device commands, reset, flash, purchases or installation were performed.
At the October 8 audit, the recovery parts had not arrived; compatibility was unverified.

## Official tool

JieLi's [AC79 download documentation](https://doc.zh-jieli.com/AC79/zh-cn/release_v1.1.0/getting_started/preparation/update.html)
links its [Forced Upgrade Tool 4.0 manual](https://pkgman.jieliapp.com/s/updator4doc).
The manual's printed pages 8–9 describe USB-key and ISP-key modes. Entry requires
a chip reset coinciding with the handshake; tool power cycling interrupts its
output for at least 250 ms. A continuous-key mode also exists. Its JTAG/flash
features are tool capabilities, not confirmed connections on these boards.

Inference for our battery-powered instruments: USB power interruption may not
reset the SoC. Establish the actual reset/power behavior before interpreting a
failed entry attempt. This is not an instruction to disconnect batteries or
short unidentified pads. Do not purchase a tool based solely on the family name.

## Community implementation limit

The [jl-uboot-tool README](https://github.com/kagaimiq/jl-uboot-tool)
lists **WL82 / AC791N as unknown**, despite providing generic flash-read commands.
Its existence does not establish a usable SMK calibration-backup path.
The author's [entry research](https://github.com/kagaimiq/jl-uboot-tool/blob/main/docs/how-to-enter-uboot.md)
separates handshake generation from subsequent USB-host connection, and notes
that host SOF timing matters. A home-built key sender alone is not a demonstrated
flash reader or restore solution.

## Next evidence needed

### Loader audit

Reviewed community revision `adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a`.
The [loader configuration](https://github.com/kagaimiq/jl-uboot-tool/blob/adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a/data/usb-loaders.yaml)
defines a WL82 RAM loader at `0x01C02000`, encrypted as MengLi.
The [main script](https://github.com/kagaimiq/jl-uboot-tool/blob/adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a/jluboottool.py)
automatically uploads and jumps to that loader when it encounters UBOOT1.00,
before executing a user command such as read. Therefore requesting read alone
does not mean no target code is executed. Its command-line defaults select NOR
with divider 0 and SPI mode 0; board compatibility is not established by them.
The [device opener](https://github.com/kagaimiq/jl-uboot-tool/blob/adb3f18889e88ac512ce0a3c4d8cc3d3cb30696a/jltech/uboot.py)
also probes non-UBOOT products with a loader command, so it is not a substitute
for passive OS enumeration.

Downloaded only the loader into ignored research storage and compared bytes:

| File | Bytes | SHA256 |
| --- | ---: | --- |
| Community WL82 loader | 24064 | `d41da6126760c9d66660bcc0cac8d27d221806c5e369a8036921efe68dca5376` |
| Existing AC79 SDK `cpu/wl82/tools/wl82loader.bin` | 30720 | `d9de9f1c81a3bb52998e5bced678f0ca851223014a7d43965b03f0719a44eb55` |

They differ. This does not establish which is appropriate or whether the
difference is only version/encoding. Neither binary was run. Local report:
`build/research/recovery/wl82-loader-comparison.json`. Keep vendor binaries out
of commits/releases. Loader presence improves the research lead, but does not
override the upstream README's unknown WL82 compatibility status.

### Remaining verification

1. Match the observed chip/package and board revision to supported tooling.
2. With hardware available, verify reset and low-level USB enumeration without
   installing the diagnostic application. Record descriptors and reproducibility.
3. Audit a WL82-specific readback implementation before executing a loader;
   obtain repeatable full backups with matching hashes and preserve original bytes.
4. Confirm calibration/identity regions and a restore method independently of
   the custom application's software-update request.

The existing vendor Pro016 update is useful restoration material, but is not
the unit's complete flash/calibration backup. Ordinary MIDI/audio enumeration
and a Windows connection sound prove neither recovery entry nor readback.

## October 10 arrival and passive enumeration

The owner reports that the parts and another FM-1 have arrived. Exact adapter
model, pin labels and wiring have not yet been confirmed. Arrival removes the
shipping wait; it does not establish a working recovery adapter.

A fresh Windows `Get-PnpDevice -PresentOnly` query found Felucca audio/MIDI
interfaces with status OK, including `USB\VID_1209&PID_0001&MI_00`. It found no
present entries matching the SMK VID 4353, RP2040 VID 2E8A, Seeed VID 2886, or
the names SMK-37, XIAO, RP2040 or UBOOT. This filtered observation cannot rule
out an adapter enumerating under another identity. It also cannot identify
which FM-1 is connected or prove that the stalled unit recovered. No device
output, reset, loader execution or flash operation was performed.

The existing SMK analysis image was checked against its manifest: 338200 bytes,
SHA256 `f9cce8a8fc54b18de01a2d003931df8e2ea1e726002f0252e69de5c3c96004dd`.
This confirms artifact integrity only, not a rebuild or device compatibility.
It remains non-installable. Next hardware steps require confirmed adapter
identity/wiring and identifying the connected instrument. Full calibration
readback and independent recovery remain outstanding for the SMK itself;
FM-1 recovery success alone would not prove those requirements.

## October 10 FM-1 recovery completed; SMK still unverified

The owner's photographs confirm a Seeed XIAO RP2040 and USB-A breakout.
Wiring was D6/GP0 to D+, D7/GP1 to D-, and GND to GND; VBUS remained
unconnected. The owner confirmed the obscured GND connection.

FM-1 Transporter source revision `a632d923203170e3a08565f4056dbb3fd3d65ca5`
was built with Pico SDK 2.2.0 and the 24064-byte community WL82 loader
identified above. The adapter's USB data and console ports work on Windows
with pyserial 3.5 and an explicit COM port. No firmware logic was changed;
two downloaded text files needed LF normalization for the build-time patch.

The stalled FM-1 entered `WL80UBOOT1.00`, VID/PID `4C4A:8057`, answered
INQUIRY as WL82/UBOOT1.00, and reported key 980F, device type 3, flash 856014.
Two independent full 1 MiB reads were identical, SHA256
`cc7940763e554c73a2746f65e329907fdfe6b2dccb1aafe1efada30f38f2f086`.
A third read matched immediately before restore.

With explicit owner approval, 143 application sectors in [0x4000,0x93000)
were restored from stock FM-1_015. Each sector was verified by Transporter.
The final full read matched the prepared image, SHA256
`be87cfdaab068ee5d0287070111e6c24e9a845bf335404ce4923a7cd96e79abd`.
Both [0,0x4000) and [0x93000,0x100000) were byte-identical to the backup.
The owner then confirmed successful normal operation after the requested
power cycle and sound check. FM-1 recovery is complete.

Local backup/report location: `E:/AI_Apps/Felucca/recovery-backups/`
`bad-fm1-20261010T210809Z/`. Keep unit dumps private; do not commit/publish.
The original Transporter write CLI imports unavailable private research
modules. It was not modified or given a fabricated review result. The
local restore used its sector transport functions with explicit stock-package
and raw-image SHA256 checks, chip/adapter checks, full pre-write comparison,
protected-region checks, a write journal and final full-flash comparison.

This supersedes the earlier shipping/adapter uncertainty and demonstrates
recovery on this FM-1 only. It does not validate the SMK flash geometry,
SMK chip/flash IDs, trim mapping, or use of this loader on the SMK. Do not use
Transporter's FM-1 write range or fixed 1 MiB assumptions for SMK restoration.
The next SMK hardware milestone is its own verified readback/calibration backup
and recovery-entry check before custom firmware installation.


## October 10 SMK baseline and separate flash-read-only adapter

Owner confirmed SMK connected directly to PC and running stock firmware.
Passive Windows enumeration reports USB VID 4353 PID B64B, serial
4134FD623E22996A, audio MI_00 and MIDI MI_03, all OK. Three MIDI inputs,
three MIDI outputs and audio playback/recording endpoints are present.
No MIDI commands or SMK writes were issued for this baseline.

Local FM-1-transporter now has a separate `fm1_transporter_readonly` CMake
 target with `FM1T_READ_ONLY=1`. Compiled ARM disassembly confirms
`jl_flash_write_sector`, `jl_rom_mem_write`, and `jl_rom_jump` each consist
only of return-false instructions, with no calls. Status includes readonly=1;
USB product is WL82 Read-only Transporter. This disables host-requested flash
writes and arbitrary RAM uploads/jumps. It still permits uploading/executing
the fixed embedded RAM loader for flash reads; it is NOT a purely passive
USB observer and has not yet been tested on SMK hardware.

Artifact: `E:/AI_Apps/Felucca/FM-1-transporter/build/fm1_transporter_readonly.uf2`
SHA-256: `1cb59d4fbc0eda19d58f9e84e9a6f9a48d729f719434f094d9e46c8675fd43f5`.
Validated 420 UF2 blocks, RP2040 family ID, contiguous addresses within XIAO
flash, matching binary payload, and exact embedded reviewed loader.
Original successful FM-1 recovery UF2 remains unchanged (SHA-256
`98f3797ed75049705191972d1a7f38f69e7ae4e3087a7a34366721f1a508567f`).

Next: load the separate adapter build with no instrument connected, verify
its identity/status, then inspect SMK recovery descriptors before deciding
whether to run the fixed loader. SMK flash capacity, unique-data backup,
and recovery compatibility remain unverified. No SMK installation authorized.


## October 10 SMK ROM entry and verified stock backup

Protected adapter UF2 was loaded onto XIAO with SMK disconnected. Owner then
connected the powered-off SMK through the three-wire adapter and powered it
from battery. ROM recovery enumerated as VID 4C4A PID 8057, product and
manufacturer WL80UBOOT1.00; INQUIRY WL82 UBOOT1.00. Adapter confirmed
`uboot=1 readonly=1 loader_running=0`. Fixed RAM loader then ran successfully;
info returned `key=980F type=3 id=856014`, with readonly=1 still reported.
No flash-write command was issued.

Two independent reads of addresses 0x000000 through 0x0FFFFF are identical:
SHA-256 `cf2e49a82cff0ca569731462caad658949d7ca98b424f29afa846a471f17ed23`.
Local backup directory:
`E:/AI_Apps/Felucca/recovery-backups/smk37-stock-20261010T215221Z/`.
This preserves the entire first 1 MiB; physical chip capacity has not been
independently established beyond the reported ID. The first 643072 bytes
match the validated Pro 016 package flash.bin exactly, including boot and app.

Actual unit usrtrim record starts at 0xC1000, is 556 bytes, and ends with
trimdat plus NUL at 0xC1224. All 37 five-point key curves pass the existing
strict-increasing 10-bit validation. A separate local record copy and decoded
curves are retained in the backup directory. Preserve unique device data
privately; do not publish full backups as release firmware.

SMK ROM entry and flash reads are now hardware verified. SMK flash restore,
custom firmware execution, and return to normal stock boot after this read
are not yet verified. FM-1 write-range restrictions remain inappropriate for
SMK and must not be reused blindly. No SMK flash installation performed.

Owner subsequently confirmed the requested normal stock boot/notes check ("I chekced it"). Return to normal operation after the read is now owner-confirmed. SMK custom startup and flash restore remain untested.


Offline follow-through: `smk37_backup.py` now checks both reads, the pinned
Pro 016 package, exact stock payload match, and calibration record validity.
Six regression tests cover differing reads, lengths, marker corruption,
non-monotonic/out-of-range curves across all keys, and wrong package rejection.
All 58 SMK Python preparation tests pass. Rebuilt the application with optional
validated backup metadata: 338200 bytes, SHA-256
`f9cce8a8fc54b18de01a2d003931df8e2ea1e726002f0252e69de5c3c96004dd`.
The binary remains unchanged and non-installable, with no flash writer linked.
Build metadata carries hashes only, not private calibration contents. Scanner
cadence has not been measured, so physical-key velocity remains fixed.


## Private Test 01 image (October 10)

At the owner's request for a build, `tools/smk37_test_image.py` now creates a
local recovery-adapter test image from the validated stock backup and linked
application. It replaces only the existing app.bin slot and four CRC16 fields,
then round-trip validates the encrypted layout and all file CRCs. Bootloader,
resource files, reserved descriptors and saved data remain unchanged. No FWSC,
OTA loader or device writes are generated. The bundle contains private stock
material and must not be published or used on another device.

Private bundle: `E:/AI_Apps/Felucca/recovery-backups/Felucca-Salt-SMK37-Test01-PRIVATE.zip`.
Test image SHA-256: `2703a62a132f1465b9b3ebe4ebcd317ddc15cca38f08e97710aaf49f226ad581`.
Restore image SHA-256: `cf2e49a82cff0ca569731462caad658949d7ca98b424f29afa846a471f17ed23`.
Changed sectors: 152, from 0x4000 through 0x9B000 inclusive. SMK-specific
writer/restore workflow is still required; the protected adapter cannot write,
and the FM-1 write limit is insufficient. Physical operation is untested.


## Test 01 installed with owner authorization (October 10)

Owner explicitly authorized installation ("Let's do it") after reviewing the experimental features and limitations. All 152 application sectors were written and individually verified. The complete final 1 MiB read matches Test 01 SHA-256 `2703a62a132f1465b9b3ebe4ebcd317ddc15cca38f08e97710aaf49f226ad581`. Bootloader below 0x4000 and all bytes from 0x9D000 onward are unchanged. No automatic reboot occurred. Physical startup/screen/audio testing is pending. Stock restoration remains available but has not been executed on this SMK.

Session evidence: `E:/AI_Apps/Felucca/recovery-backups/smk37-test01-flash-kit/session-install-20261010T221855.337950Z`. Contains fsynced pre-write backup, per-sector journal, and complete verified after-image. Original build manifests describe pre-install artifact status and are retained unchanged.


Test 01 hardware feedback: title and sound-page display appear; owner corrected Mute to OFF. No audible sound and no apparent knob changes. Button LEDs, wheels/faders/pads were not implemented. Keys/encoder commands depend on audio IRQ consumption, so lack of response does not establish scanner failure. Diagnostic 02 opens directly to counters and main-loop uptime, with raw input indicators independent of audio consumption. Local image edda82d91eaccb97302f8bd65ba8f4819b33976fe960341b3ae445f76acd91a0; not yet flashed.


Diagnostic 02 owner observations: uptime, scan, audio and audio IRQ counters all increase. BUTTON/ENCODER do not respond to physical controls. This rules out absence of those interrupts, not GPIO/mux/timing errors. Diagnostic 03 now exposes all 16 completed scan bytes with sticky bit-change detection and changed-frame count, snapshot copied briefly under interrupt mask. No scanner or audio register setup changed. Raw tracking host test passed ASan/UBSan, including brief press/release, initial baseline, multi-byte changes and counter saturation. Build 97cfc19a33a4d79a298532eeed470d369349e0b68783fb501cb5ba178e4e7f7d; device image b98f6cf751db4995c43fe140b73643b923b7b34c0e437862d356a2c6ddfc41df. Local kit `E:/AI_Apps/Felucca/recovery-backups/smk37-diag03-flash-kit`; not yet flashed.


## Save-only recovery preparation after Test09 read probe

Owner reports READ OK / SAVES DISABLED on Test09: runtime JEDEC, private trim CRC and the blank0xEB000..0xF3000 candidate allocation passed. This does not validate erasing/programming it. Test10 pedal candidate remains save-disabled.

The transporter now has a separately compiled `smk37_save_recovery` target. Its enforced write window is ONLY0xEB000..0xF3000, 4KiB aligned. It cannot write applications, boot, stock VM, trim, USRFLASH, USR or key/MAC. Existing FM-1 and SMK application writers retain their original windows. The save-only target retains the SMK key/flash/type identity check and advertises distinct save_recovery capabilities. Exhaustive address tests cover all three policies. It has not been installed on the adapter or exercised on hardware.

`FM-1-transporter/tools/smk37_clear_saves.py` is dry-run by default. It pins reviewed unit references (stock and Test09 currently), verifies every byte outside the save allocation, records a full durable pre-write backup/journal, clears only changed save sectors, and verifies every sector and full1MiB. Invalid plans, wrong identities/references, unexpected protected bytes, failures and changed baselines stop without retry. It cannot install an app or replace calibration.15 Python tests cover the app writer and save clearer; policy tests exhaustively check all byte addresses through1MiB.

Before enabling saves: finish/pin the separate UF2 and recovery tool; add the exact next candidate full-image hash after review; validate the save-only adapter capability/read path on this unit; prepare the save-enabled app to accept valid A/B records after reboot and handle interrupted writes; then obtain approval for that concrete new hardware test. Keep the known app-only adapter image and Test09 kit intact. A write/clear/power-cycle test is still required; no guarantee of restoration follows from host tests.

Save-only adapter target linked successfully and private kit prepared at ../recovery-backups/smk37-save-recovery-kit with source snapshot and checksums. Still not installed; no save-area write has occurred.
