#!/usr/bin/env python3
"""Private SMK Test11 bootstrap/rollback, preserving existing project saves. Dry run unless --write and confirmation.
No automatic boot, no retries after write failure, no writes outside app sectors.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path

SIZE, MIN, END, SECTOR = 0x100000, 0x4000, 0x9d000, 0x1000
STOCK_SHA = 'cf2e49a82cff0ca569731462caad658949d7ca98b424f29afa846a471f17ed23'
TEST_SHA = '880f8238cc8581916d00dc515266327e2d46438c33689b057893e191744a4589'
PREVIOUS_SHA = '5c03959b59d199d0b5752527c655387641bb02ff9aab230d1eb61f1292821daa'
CAPS = 'OK model=SMK37 writer=1 min=0x4000 end=0x9D000'
IDENTITY = 'OK key=980F type=3 id=856014'
ADAPTER = '4250305539333104'


def sha(b):
    return hashlib.sha256(b).hexdigest()


def check_images(stock, test):
    if len(stock) != SIZE or sha(stock) != STOCK_SHA:
        raise ValueError('Stock restore image does not match this unit backup')
    if len(test) != SIZE or sha(test) != TEST_SHA:
        raise ValueError('Test image is not reviewed Test 11')
    if stock[:MIN] != test[:MIN] or stock[END:] != test[END:]:
        raise ValueError('Protected bytes differ')


def plan(current, stock, test, mode, previous):
    check_images(stock,test)
    if len(previous)!=SIZE or sha(previous)!=PREVIOUS_SHA:
        raise ValueError('Previous image is not verified Test10')
    if len(current)!=SIZE or current[:MIN]!=stock[:MIN] or current[END:0xeb000]!=stock[END:0xeb000] or current[0xf3000:]!=stock[0xf3000:]:
        raise ValueError('Device protected data differs outside the project allocation')
    if mode not in ('install','restore'):raise ValueError('Unknown operation')
    if current[MIN:END] not in (previous[MIN:END],test[MIN:END]):
        raise ValueError('Application is neither exact Test10 nor this Test11 candidate')
    # Keep the entire current tail, including all project sectors, byte for byte.
    candidate=test if mode=='install' else previous
    target=current[:MIN]+candidate[MIN:END]+current[END:]
    return target,[a for a in range(MIN,END,SECTOR) if current[a:a+SECTOR]!=target[a:a+SECTOR]]


def apply(io, current, target, addresses, log):
    # Recheck complete plan bounds before the first destructive operation.
    if len(current) != SIZE or len(target) != SIZE:
        raise ValueError('Invalid image size')
    if current[:MIN] != target[:MIN] or current[END:] != target[END:]:
        raise ValueError('Protected range mismatch')
    expected = [a for a in range(MIN, END, SECTOR)
                if current[a:a+SECTOR] != target[a:a+SECTOR]]
    if addresses != expected:
        raise ValueError('Sector plan mismatch')
    for n, a in enumerate(addresses, 1):
        before = io.read(a, SECTOR)
        if before != current[a:a+SECTOR]:
            raise ValueError(f'Device changed before sector {a:#x}; stopped')
        log({'event': 'sector_begin', 'address': hex(a), 'before_sha256': sha(before)})
        reply = io.write(a, target[a:a+SECTOR])
        if reply != 'OK':
            raise ValueError(f'Write failed at {a:#x}: {reply}; stopped, no retry')
        if io.read(a, SECTOR) != target[a:a+SECTOR]:
            raise ValueError(f'Readback mismatch at {a:#x}; stopped')
        log({'event': 'sector_verified', 'address': hex(a)})
        print(f'{n}/{len(addresses)} verified {a:#x}', flush=True)
    result = io.read(0, SIZE)
    if result != target:
        raise ValueError('Final complete image mismatch; do not boot custom firmware')
    log({'event': 'complete', 'sha256': sha(result)})
    return result


class Transport:
    def __init__(self, connection, protocol):
        self.connection, self.protocol = connection, protocol
    def read(self, address, size):
        return self.protocol.read_flash(self.connection, address, size, progress=False)
    def write(self, address, data):
        return self.protocol.write_sector(self.connection, address, data)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('mode', choices=('plan', 'install', 'restore'))
    ap.add_argument('--bundle', type=Path, default=Path(__file__).resolve().parent)
    ap.add_argument('--port')
    ap.add_argument('--write', action='store_true')
    ap.add_argument('--confirm')
    args = ap.parse_args()
    try:
        stock = (args.bundle/'smk37-stock-restore-device-image.bin').read_bytes()
        test = (args.bundle/'smk37-test11-device-image.bin').read_bytes()
        previous = (args.bundle/'smk37-test10-device-image.bin').read_bytes()
        check_images(stock, test)
        if args.mode == 'plan':
            if args.write:
                raise ValueError('plan cannot write')
            _, addresses = plan(previous, stock, test, 'install', previous)
            print(f'Test 11: {len(addresses)} sectors; protected ranges unchanged. No device access.')
            return
        if not args.port:
            raise ValueError('--port is required')
        confirmation = 'INSTALL-SMK37-TEST11' if args.mode == 'install' else 'ROLLBACK-SMK37-TEST10'
        if args.write and args.confirm != confirmation:
            raise ValueError('Writing requires --confirm ' + confirmation)
        import serial.tools.list_ports
        import fm1t
        port = next((p for p in serial.tools.list_ports.comports() if p.device == args.port), None)
        if not port or (port.vid, port.pid, port.serial_number) != (0x2e8a, 0x000a, ADAPTER):
            raise ValueError('Expected the verified XIAO adapter; port identity mismatch')
        with fm1t.find_port(args.port) as s:
            s.write_timeout = 5
            if fm1t.request(s, 'caps', timeout=3) != CAPS:
                raise ValueError('Load the SMK writer UF2; this adapter has wrong capabilities')
            st = fm1t.status(s)
            if st.get('uboot') != '1' or st.get('readonly') != '0':
                raise ValueError('SMK recovery not ready')
            if fm1t.request(s, 'info') != IDENTITY:
                raise ValueError('Unexpected chip identity')
            io = Transport(s, fm1t)
            current = io.read(0, SIZE)
            target, addresses = plan(current, stock, test, args.mode, previous)
            print(f'{args.mode}: {len(addresses)} sectors. Preflight verified.', flush=True)
            if not args.write:
                print('DRY RUN: nothing written.')
                return
            stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
            folder = args.bundle / ('session-' + args.mode + '-' + stamp)
            folder.mkdir()
            with (folder/'before.bin').open('xb') as f:
                f.write(current); f.flush(); os.fsync(f.fileno())
            with (folder/'journal.jsonl').open('x', buffering=1) as f:
                def log(event):
                    f.write(json.dumps(event)+'\n'); f.flush(); os.fsync(f.fileno())
                log({'event': 'preflight', 'mode': args.mode, 'before_sha256': sha(current),
                     'target_sha256': sha(target), 'sectors': addresses})
                try:
                    # No intervening device changes permitted before first write.
                    if io.read(0, SIZE) != current:
                        raise ValueError('Device changed since preflight')
                    result = apply(io, current, target, addresses, log)
                    with (folder/'verified-after.bin').open('xb') as out:
                        out.write(result); out.flush(); os.fsync(out.fileno())
                except BaseException as exc:
                    log({'event': 'stopped', 'reason': str(exc)})
                    raise
            print('Complete image verified. No automatic reboot. Power off and disconnect adapter before normal boot.')
    except (OSError, ValueError) as exc:
        ap.exit(2, f'smk37_flash: {exc}\n')


if __name__ == '__main__':
    main()
