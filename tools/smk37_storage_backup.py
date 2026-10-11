#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Offline export/restore of stock regions considered for expanded storage.

Never accesses hardware. Export requires two identical 1 MiB reads. Restore
requires the same unit's preserved boot/calibration/project bytes and writes a
NEW image, never overwriting a source. Keep every output private.
"""
import argparse,hashlib,json,zipfile
from pathlib import Path
REGIONS=(('stock-vm.bin',0x9d000,0xc0000),('stock-user.bin',0xc2000,0xeb000))
def digest(data):return hashlib.sha256(data).hexdigest()
def protected(data):return data[:0x9d000]+data[0xc0000:0xc2000]+data[0xeb000:]
def image(path):
    data=Path(path).read_bytes()
    if len(data)!=0x100000:raise ValueError('Expected a complete 1 MiB read')
    return data
def export(first,second,output):
    data=image(first)
    if data!=image(second):raise ValueError('The two reads differ')
    meta={'format':'SMK37-STOCK-REGIONS-1','private':True,'image_sha256':digest(data),'protected_sha256':digest(protected(data)),'regions':[]}
    for name,lo,hi in REGIONS:meta['regions'].append({'name':name,'start':lo,'length':hi-lo,'sha256':digest(data[lo:hi])})
    with zipfile.ZipFile(output,'x',zipfile.ZIP_DEFLATED) as z:
        z.writestr('manifest.json',json.dumps(meta,indent=2)+'\n')
        for name,lo,hi in REGIONS:z.writestr(name,data[lo:hi])
    return meta
def restore(archive,base,output):
    data=bytearray(image(base))
    with zipfile.ZipFile(archive) as z:
        if sorted(z.namelist())!=sorted(['manifest.json']+[r[0] for r in REGIONS]):raise ValueError('Unexpected or duplicate archive entries')
        if z.getinfo('manifest.json').file_size>8192:raise ValueError('Oversized manifest')
        meta=json.loads(z.read('manifest.json'))
        if meta.get('format')!='SMK37-STOCK-REGIONS-1' or meta.get('protected_sha256')!=digest(protected(data)):raise ValueError('Wrong format or different protected device data')
        rows=meta.get('regions',[])
        if len(rows)!=len(REGIONS):raise ValueError('Missing region')
        for row,(name,lo,hi) in zip(rows,REGIONS):
            if (row.get('name'),row.get('start'),row.get('length'))!=(name,lo,hi-lo):raise ValueError('Region boundaries differ')
            if z.getinfo(name).file_size!=hi-lo:raise ValueError('Wrong region size')
            payload=z.read(name)
            if digest(payload)!=row.get('sha256'):raise ValueError('Region hash mismatch')
            data[lo:hi]=payload
    if digest(data)!=meta.get('image_sha256'):raise ValueError('Restored image does not match the original')
    with open(output,'xb') as f:f.write(data)
    return {'bytes':len(data),'sha256':digest(data),'private':True}
def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='action',required=True)
    e=sub.add_parser('export');e.add_argument('first');e.add_argument('second');e.add_argument('output')
    r=sub.add_parser('restore');r.add_argument('archive');r.add_argument('base');r.add_argument('output')
    a=p.parse_args();result=export(a.first,a.second,a.output) if a.action=='export' else restore(a.archive,a.base,a.output);print(json.dumps(result,indent=2))
if __name__=='__main__':main()
