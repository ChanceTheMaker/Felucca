# SMK37 stock interface evidence

Stock Pro and Elite v16 applications contain different USB product-ID
candidates. Earlier packages and other model variants share IDs, so an ID
alone cannot select firmware safely. Verify the arriving device's label,
firmware screen, observed descriptors and exact-model package together.

The [inventory](interface-inventory.json) covers the same 13 packages as the
[flash analysis](FLASH_LAYOUT.md), pinned to research revision
`ab85d7340163ba9e171dc9396e31c034829bbc02`. It contains hashes, offsets, parsed
descriptor fields and short diagnostic strings, not vendor executable images.
All offsets here are within decoded `app.bin`, not physical flash addresses.

## USB device descriptor candidates

The scanner checks a complete 18-byte USB device descriptor structure, known
USB 1/2 version fields, endpoint-zero packet size, vendor ID, configuration
count and device-release BCD. It found one candidate in each application.
Compiled-in descriptors can be unused or changed at runtime; these values
remain static evidence until compared with actual USB enumeration.

| Archived packages | Candidate VID | Candidate PID |
| --- | --- | --- |
| Pro 011, 012, 014 | `4C4A` | `4155` |
| Pro 013, 015; Elite 015; MKE 013, 015; Starrykey 015 | `4353` | `4B4D` |
| Pro 016; MKE 016; Starrykey 016 | `4353` | `344B` |
| Elite 016 | `4353` | `354B` |

Pro v16's candidate is at `0x58553`; Elite v16's is at `0x58B6C`.
Both declare USB `0x0200`, device release `0x0100`, a 64-byte endpoint zero,
one configuration, class/subclass/protocol zero, and product/serial string
indices 2/3. Keep the USB release field separate from the package's `_016`
firmware identity; it does not identify the installed software version.

Class zero defers classification to the interfaces. This descriptor does not
establish MIDI port count, audio support, endpoint allocation, port names, or
the actual serial string. No complete configuration descriptor was identified
by the initial contiguous-header search; runtime construction or other layouts
remain possible. That absence does not establish missing USB functionality.
Field interpretation follows Microsoft's [USB device descriptor reference][usb].
No VID ownership claim is inferred from these firmware bytes.

## Hardware and recovery leads

| String in both v16 applications | Follow-up | What it does not prove |
| --- | --- | --- |
| `midi_route` | Compare stock ports and routing modes during capture | Physical UART wiring or host port names |
| `usb_update_mode` | Locate code references and compare vendor update behavior | A working recovery-entry sequence |
| `usb_update2.bin`, `uart_user.bin`, `usb_hid_ota.bin` within printable spans | Inspect resource loading and official updater behavior | That any named updater is reachable or safe on this board |
| `sdram`, `sdram_powerup_ok` | Trace memory initialization if needed | Fitted SDRAM, its size, or usable application RAM |

The earlier boot configuration has `ENABLE_SDRAM=0`; SDRAM strings are not
grounds to override it. Neither source gives a measured RAM map. No LCD
controller, pad/key scanner pin map or DAC wiring was established by this scan.
Reused SDK strings may describe code that never runs.

Next hardware work: compare the ordinary boot's USB snapshot with these
candidates; capture every observed MIDI input independently; then record a
documented stock recovery route. Disassembly may narrow peripheral setup,
but GPIO assignments need corroboration before driving pins.

## Reproduce and validate

```sh
python tools/smk37_interfaces.py path/to/SMK-37_Pro_016.fwsc path/to/SMK-37_Elite_016.fwsc
python -m unittest discover -s tests -p "smk37_*_test.py"
```

The tool validates the package and decoded application through the existing
layout inspector before scanning. It reads files only and prints JSON metadata;
it never opens a device, flashes, executes firmware or extracts executable files.
Seven tests cover unaligned descriptors, distinct IDs, every descriptor
truncation, malformed candidates, string boundaries, invalid packages, and
report identity/uncertainty. All 44 preparation tests pass, and all 13 pinned
research packages were processed. Hardware checks remain pending.

[usb]: https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-device-descriptors
