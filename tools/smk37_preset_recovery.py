#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare an OFFLINE preset-area reset plan from verified stock and current reads.
Never opens a device and never flashes. Preserves application, projects and all
factory regions. Adapter write support must be checked separately before use.
"""
import argparse,hashlib,json
from pathlib import Path
from smk37_backup import inspect_backup
SECTORS=(0xf3000,0xfe000)
SIZE=0x100000

def reset_plan(current,stock):
    if len(current)!=SIZE or len(stock)!=SIZE:raise ValueError('Expected complete 1 MiB reads')
    for a in SECTORS:
        if stock[a:a+4096]!=bytes([255])*4096:raise ValueError('Stock preset sector is not blank')
    # Only app/project/preset bytes may have changed since stock.
    for lo,hi in ((0,0x4000),(0x9d000,0xeb000),(0xf4000,0xfe000),(0xff000,SIZE)):
        if current[lo:hi]!=stock[lo:hi]:raise ValueError(f'Protected region differs at {lo:#x}')
    target=bytearray(current)
    addresses=[]
    for a in SECTORS:
        if current[a:a+4096]!=stock[a:a+4096]:addresses.append(a)
        target[a:a+4096]=stock[a:a+4096]
    return bytes(target),addresses

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--stock-backups',nargs=2,type=Path,required=True)
    ap.add_argument('--stock-package',type=Path,required=True)
    ap.add_argument('--current-read',type=Path,required=True)
    ap.add_argument('--out',type=Path,required=True)
    a=ap.parse_args();stock=a.stock_backups[0].read_bytes()
    inspect_backup(stock,a.stock_backups[1].read_bytes(),a.stock_package.read_bytes())
    current=a.current_read.read_bytes();target,addresses=reset_plan(current,stock)
    a.out.mkdir(parents=True,exist_ok=True)
    digest=lambda b:hashlib.sha256(b).hexdigest()
    plan={'operation':'offline preset reset plan only','requires_adapter_writer_review':True,
          'current_sha256':digest(current),'result_sha256':digest(target),'sectors':[]}
    for address in addresses:
        name=f'preset-reset-{address:05x}.bin';block=target[address:address+4096];(a.out/name).write_bytes(block)
        plan['sectors'].append({'address':hex(address),'bytes':4096,'file':name,'before_sha256':digest(current[address:address+4096]),'after_sha256':digest(block)})
    (a.out/'preset-reset-plan.json').write_text(json.dumps(plan,indent=2)+'\n',encoding='utf-8')
    print(f'{len(addresses)} preset sectors planned; no device opened or written.')
if __name__=='__main__':main()
