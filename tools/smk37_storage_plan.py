#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only SMK expansion inventory. Never opens a device or rewrites an image.

The proposed sample/preset regions contain stock user data. This report is not
permission to erase them and is deliberately not imported by the flash driver.
"""
import argparse
import hashlib
import json
from pathlib import Path

REGIONS = (
    ('sample1', 0x9d000, 0xb1000),
    ('presets32_a', 0xb1000, 0xb4000),
    ('presets32_b', 0xb4000, 0xb7000),
    ('fm6bank_a', 0xb7000, 0xb9000),
    ('fm6bank_b', 0xb9000, 0xbb000),
    ('reserved_metadata', 0xbb000, 0xc0000),
    ('sample2', 0xc2000, 0xd6000),
    ('sample3', 0xd6000, 0xea000),
    ('reserved_journal', 0xea000, 0xeb000),
)
PROTECTED = ((0, 0x9d000), (0xc0000, 0xc2000), (0xeb000, 0x100000))

def inspect(first, second):
    if len(first) != 0x100000 or first != second:
        raise ValueError('two matching complete 1 MiB stock backups are required')
    result = []
    end = 0
    for name, lo, hi in REGIONS:
        if lo < end or lo % 4096 or hi % 4096 or lo >= hi:
            raise ValueError('invalid/overlapping proposed allocation')
        if any(lo < b and hi > a for a, b in PROTECTED):
            raise ValueError('proposal overlaps boot/application/calibration/existing stores')
        raw = first[lo:hi]
        result.append(dict(name=name, start=hex(lo), end=hex(hi), bytes=hi-lo,
                           non_erased_bytes=sum(b != 255 for b in raw),
                           sha256=hashlib.sha256(raw).hexdigest()))
        end = hi
    return dict(schema=1, mode='offline proposal only', device_writes=False,
                enabled_in_firmware=False, migration_required=True,
                stock_sha256=hashlib.sha256(first).hexdigest(), regions=result,
                preserved=[dict(start=hex(a), end=hex(b)) for a,b in PROTECTED],
                prerequisites=['export stock VM and USRFLASH before formatting',
                    'versioned format and interrupted-migration recovery',
                    'runtime ownership/JEDEC/calibration verification',
                    'new updater preservation and downgrade tests',
                    'sample CRC and upstream file/protocol compatibility',
                    'hardware write/readback and restart test'])

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('backups', nargs=2, type=Path)
    a=p.parse_args()
    try: result=inspect(*(path.read_bytes() for path in a.backups))
    except (OSError,ValueError) as e: p.error(str(e))
    print(json.dumps(result,indent=2))

if __name__=='__main__': main()
