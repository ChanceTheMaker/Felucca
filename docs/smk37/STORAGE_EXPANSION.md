# SMK storage expansion: migration required

This is the expansion plan. The existing eight presets and four projects remain
the only writable allocations in Test19. Test21 integrates the sample subset
behind explicit initialization; the 32-preset and FM6 objects remain proposed.

The private stock Pro backup contains **20,505 non-FF bytes** in
`0x9d000..0xc0000` (VM) and **85,518 non-FF bytes** in
`0xc2000..0xeb000` (USRFLASH). These are not blank free space. The proposal
replaces stock user-data regions only after a recoverable format/migration;
neither absence from our synth nor an erased-looking sector proves ownership.

| Proposed use | Half-open flash range | Size |
| --- | --- | --- |
| User sample 1 | 0x9d000..0xb1000 | 80 KiB |
| 32-preset bank A | 0xb1000..0xb4000 | 12 KiB |
| 32-preset bank B | 0xb4000..0xb7000 | 12 KiB |
| FM6 voice bank A | 0xb7000..0xb9000 | 8 KiB |
| FM6 voice bank B | 0xb9000..0xbb000 | 8 KiB |
| Format/migration metadata reserve | 0xbb000..0xc0000 | 20 KiB |
| User sample 2 | 0xc2000..0xd6000 | 80 KiB |
| User sample 3 | 0xd6000..0xea000 | 80 KiB |
| Journal reserve | 0xea000..0xeb000 | 4 KiB |

Preserve boot/application `0..0x9d000`, Bluetooth/trim
`0xc0000..0xc2000`, and all existing projects, preset A/B, OEM USR and key/MAC
data `0xeb000..0x100000`. The current application-only updater already ends at
`0x9d000`; the storage proposal does not move user samples into its erase window.

The sample slot size matches upstream's 80 KiB format; addresses need an SMK
adapter because the slots are not contiguous. Manual slice metadata should keep
upstream's sample-bound CRC and last-sector rules. The 32-preset payload needs
multi-sector atomic A/B commits; simply increasing `SMK37_UP_SLOTS` is unsafe.
FM6 wire records and complete archives must retain upstream command IDs and
portable object formats rather than exposing raw unit-specific flash images.

`tools/smk37_storage_plan.py <backup1> <backup2>` validates two matching 1 MiB
reads and the proposed boundaries, then reports region sizes, hashes and
non-erased byte counts. It never connects to hardware or modifies either image.
Its private report is `build/test19-storage-plan.json`.

Required before enabling: stock user-data export/restore, versioned ownership
marker and resumable migration, interrupted multi-sector writes, old eight-slot
import, calibration/JEDEC checks, sample validation and cache invalidation,
archive round-trips, updater/downgrade preservation, then physical restart and
write/readback tests. The backup already exists; firmware must still implement
and test the migration rather than silently formatting on first boot.

## Test20 implementation progress

- `smk37_object_store.h`: multi-sector A/B backend with fixed permitted object
  boundaries, streaming CRC validation, readback before publishing the commit,
  and generation-wrap handling. A write requires explicit ownership from its
  caller. Nothing in the running firmware grants that ownership yet.
- `smk37_expanded_presets.h`: a 10,796-byte 32-slot payload; imports the old
  2,564/3,116-byte eight-slot banks without replacing their source. Presets,
  attached FM6 patches, favorites and preferences are preserved; new slots start
  empty. The running firmware still advertises eight slots.
- Host tests exercise failed multi-sector erases/programs, every partial commit
  header, corrupted-copy fallback, address exclusion, preset migration, an FM6
  object and generation wrap. This is not a device flash validation.
- `tools/smk37_storage_backup.py` exports the two affected stock regions from
  matching full reads, with a private manifest. Restore refuses another unit's
  protected bytes, altered boundaries, corrupt payloads and output overwrite.
  Offline export/restore of the owner's actual paired stock reads reproduced
  SHA-256 `cf2e49a82cff0ca569731462caad658949d7ca98b424f29afa846a471f17ed23`.

Still required: persistent ownership/format journal, resumable runtime migration,
the flash-range gate and UI/editor adapters, actual sample uploads and marker
persistence, FM6-bank/archive interfaces, then device write/read/restart tests.
No new flash-write range is enabled in Test20.

## Test21 sample integration

The three sample slots now have a runtime adapter and upstream FSMP upload
protocol. Ownership uses `0xbf000..0xc0000`, confirmed erased in both private
stock backups, within the metadata reserve. The proposed `0xea000` journal
sector is not used: it contains stock bytes. The remaining metadata, preset and
FM6 object reservations stay untouched. Initialization commits an intent before
invalidating sample headers, verifies erases and publishes completion last.
Normal boots only inspect this journal. Detailed gates, interruption tests,
manual slice persistence and the required hardware checks are in [TEST21.md](TEST21.md).
