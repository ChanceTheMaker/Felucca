#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Read-only Windows USB inventory and offline snapshot comparison.

Lists present USB PnP nodes; does not open devices, change drivers or send MIDI.
Instance paths are hashed before saving to avoid storing serial suffixes.
The snapshot is an observation, not SMK identification or flash authorization.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

SCHEMA = 'felucca-smk37-usb-v1'
SCRIPT = r"""
$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = New-Object System.Text.UTF8Encoding($false)
$nodes = @(Get-PnpDevice -PresentOnly | Where-Object { $_.InstanceId -like 'USB\*' })
$result = @(foreach ($node in $nodes) {
    $parent = Get-PnpDeviceProperty -InstanceId $node.InstanceId -KeyName 'DEVPKEY_Device_Parent' -ErrorAction SilentlyContinue
    [pscustomobject]@{
        instance_id = $node.InstanceId
        parent_id = $parent.Data
        name = $node.FriendlyName
        device_class = $node.Class
        status = $node.Status
    }
})
ConvertTo-Json -InputObject $result -Depth 4 -Compress
"""


def identity(value):
    return hashlib.sha256(value.upper().encode('utf-8')).hexdigest()


def normalize(rows):
    if not isinstance(rows, list):
        raise ValueError('USB query must return an array')
    result, seen = [], set()
    for row in rows:
        if not isinstance(row, dict):
            raise ValueError('USB node must be an object')
        path = row.get('instance_id')
        if not isinstance(path, str) or not path.upper().startswith('USB\\'):
            raise ValueError('invalid USB instance path')
        key = identity(path)
        if key in seen:
            raise ValueError('duplicate USB instance path')
        seen.add(key)
        hardware = path.split('\\')[1].upper()
        def field(pattern):
            match = re.search(pattern, hardware)
            return match[1] if match else None
        item = dict(instance_key=key, vid=field(r'(?:^|&)VID_([0-9A-F]{4})(?:&|$)'),
                    pid=field(r'(?:^|&)PID_([0-9A-F]{4})(?:&|$)'),
                    interface=field(r'(?:^|&)MI_([0-9A-F]{2})(?:&|$)'))
        for name in ('name', 'device_class', 'status'):
            value = row.get(name)
            if value is not None and not isinstance(value, str):
                raise ValueError(f'{name} must be text or null')
            item[name] = value
        parent = row.get('parent_id')
        if parent is not None and not isinstance(parent, str):
            raise ValueError('parent_id must be text or null')
        item['parent_key'] = identity(parent) if parent else None
        result.append(item)
    return sorted(result, key=lambda r: r['instance_key'])


def snapshot(label):
    if os.name != 'nt':
        raise ValueError('snapshot requires Windows; diff works on any platform')
    ps = Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe'
    response = subprocess.run([str(ps), '-NoProfile', '-NonInteractive', '-Command', SCRIPT],
                              check=True, capture_output=True, encoding='utf-8-sig', timeout=60)
    return dict(schema=SCHEMA, utc=datetime.now(timezone.utc).isoformat(), label=label,
                scope='Present USB PnP nodes, not all Windows audio/MIDI endpoints',
                devices=normalize(json.loads(response.stdout)))


def load_snapshot(path):
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    index_snapshot(data)
    return data


def index_snapshot(data):
    if not isinstance(data, dict) or data.get('schema') != SCHEMA or not isinstance(data.get('devices'), list):
        raise ValueError('unrecognized USB snapshot')
    indexed = {}
    for device in data['devices']:
        if not isinstance(device, dict):
            raise ValueError('USB node must be an object')
        key = device.get('instance_key')
        if not isinstance(key, str) or not re.fullmatch('[0-9a-f]{64}', key) or key in indexed:
            raise ValueError('invalid or duplicate USB instance key')
        indexed[key] = device
    return indexed


def compare(before, after):
    old, new = index_snapshot(before), index_snapshot(after)
    return dict(before_label=before.get('label'), after_label=after.get('label'),
                added=[new[k] for k in sorted(new.keys() - old.keys())],
                removed=[old[k] for k in sorted(old.keys() - new.keys())],
                changed=[dict(before=old[k], after=new[k]) for k in sorted(old.keys() & new.keys()) if old[k] != new[k]],
                interpretation='Changes are candidates to investigate, not automatic model or firmware compatibility detection')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    commands = ap.add_subparsers(dest='command', required=True)
    snap = commands.add_parser('snapshot')
    snap.add_argument('--label', required=True)
    snap.add_argument('--out', type=Path, required=True)
    diff = commands.add_parser('diff')
    diff.add_argument('before', type=Path)
    diff.add_argument('after', type=Path)
    args = ap.parse_args()
    try:
        if args.command == 'snapshot':
            if args.out.exists():
                raise FileExistsError('snapshot already exists; choose a new filename')
            result = snapshot(args.label)
            with args.out.open('x', encoding='utf-8') as output:
                json.dump(result, output, indent=2)
                output.write('\n')
            print(f'Saved {len(result["devices"])} USB nodes to {args.out}')
        else:
            print(json.dumps(compare(load_snapshot(args.before), load_snapshot(args.after)), indent=2))
    except subprocess.CalledProcessError:
        ap.exit(2, 'USB query failed; confirm Get-PnpDevice is available in Windows PowerShell.\n')
    except subprocess.TimeoutExpired:
        ap.exit(2, 'USB query timed out; no snapshot was saved.\n')
    except (OSError, ValueError) as error:
        ap.exit(2, f'smk37_usb: {error}\n')


if __name__ == '__main__':
    main()
