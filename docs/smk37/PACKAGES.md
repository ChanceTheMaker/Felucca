# SMK37 package comparison

The offline inventory in [package-inventory.json](package-inventory.json)
fingerprints 13 mirrored vendor packages from the pinned research revision in
[the preparation plan](../SMK37_PREPARATION.md). It contains metadata and hashes,
not vendor binaries. This evidence supports a shared Pro/Elite implementation
with model-specific packages; it does not establish cross-flash compatibility.

## Version 16 observations

| Field | Pro | Elite |
| --- | --- | --- |
| Embedded product | `SMK-37 Pro_016` | `SMK-37 Elite_016` |
| Container chip | `AC791N` | `AC791N` |
| File size | 705252 bytes | 705236 bytes |
| FWSC marker blocks | 36 | 20 |
| Logical UFW size | 705216 bytes | 705216 bytes |
| Flash payload | 643072 bytes, different SHA-256 | 643072 bytes, different SHA-256 |
| OTA loader | 19969 bytes, identical SHA-256 | 19969 bytes, identical SHA-256 |

The `USR`, `isd_config.ini`, `script.ver` and `tail.bin` payload hashes also
match. `blimit.bin` differs. This comparison does not decode the flash payload
to establish whether application differences are branding, configuration or
executable logic. The follow-on [flash inspection](FLASH_LAYOUT.md) confirms
that the decoded application files themselves differ and records their shared
boot configuration and storage descriptors. The manufacturer's same-circuit declaration remains the
strongest evidence for sharing the board implementation.

The Pro v16 wrapper has 36 marker blocks; the FM-1 installer currently handles
20. A future SMK updater needs its own verified format/model handling. This
research tool does not change the existing FM-1 installer.

## Inspect without connecting hardware

```sh
python tools/smk37_package.py path/to/SMK-37_Pro_016.fwsc path/to/SMK-37_Elite_016.fwsc
python tests/smk37_package_test.py
```

The command prints JSON and never opens a MIDI device, extracts executable
files or writes firmware. It rejects invalid header/directory CRCs, unsupported
wrapper lengths, overlapping/out-of-range entries and bad flash/OTA/tail CRCs.
It reports raw CRC equality for other vendor metadata but does not decode or
validate their specialized encodings. `raw_crc_matches: false` on those entries
does not by itself establish corruption. `compatibility` always remains
`unverified`; checksum agreement is not authentication or device validation.

Both wrapper variants are covered by synthetic fixtures. Tests also cover
truncation, trailing data, corruption, overlaps and forged directory offsets.
All 13 research samples pass the implemented checks. No sample was flashed.
