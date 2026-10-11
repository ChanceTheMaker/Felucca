#!/usr/bin/env python3
"""Offline Pro 016 analysis wrapper. No device access or loadable ELF segments.

Only the pinned decoded stock application is accepted by the CLI. Sections
describe observed startup copying, not a proposed linker map or flash image.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

PRO_SHA256 = '0f64fbf6ffc454eea686d78cf89f35e593229765bdcf166f10a8bd714abcb09a'
XIP = 0x02000120
RAM = 0x01C00000


def startup_layout(app):
    """Decode only the observed entry template; reject different instructions."""
    fields = {'sp': (4, 0xee), 'ssp': (10, 0xed),
              'zero_start': (22, 0xc3), 'zero_size': (30, 0xc2),
              'ram_start': (44, 0xc4), 'copy_source': (50, 0xc1),
              'copy_size': (56, 0xc2)}
    values = {}
    for name, (offset, opcode) in fields.items():
        if len(app) < offset + 6 or app[offset:offset+2] != bytes((opcode, 0xff)):
            raise ValueError(f'Unsupported startup instruction: {name}')
        values[name] = struct.unpack_from('<I', app, offset+2)[0]
    offset = values['copy_source'] - XIP
    size = values['copy_size']
    if offset < 62 or size <= 0 or offset + size > len(app):
        raise ValueError('RAM copy lies outside application')
    if values['ram_start'] != RAM or size % 4 or values['zero_size'] % 4:
        raise ValueError('Unsupported RAM destination/alignment')
    if values['zero_start'] != RAM + size:
        raise ValueError('Zero-fill does not follow copied RAM')
    end = values['zero_start'] + values['zero_size']
    if not RAM < end <= 0x01C7FD50:
        raise ValueError('RAM extent reaches reserved boot area')
    if not all(values['zero_start'] <= values[n] < end for n in ('sp', 'ssp')):
        raise ValueError('Stack outside observed zero-filled extent')
    values.update(copy_offset=offset, zero_end=end,
                  sha256=hashlib.sha256(app).hexdigest())
    return values


def analysis_elf(app, layout):
    """ELF32/pi32v2 sections only; RAM code/data stays explicitly mixed."""
    start, size = layout['copy_offset'], layout['copy_size']
    names = b'\0.text\0.ram_copy\0.tail\0.shstrtab\0'
    sections = [(1, 1, 6, XIP, app[:start]),
                (7, 1, 7, RAM, app[start:start+size]),
                (17, 1, 2, XIP+start+size, app[start+size:]),
                (23, 3, 0, 0, names)]
    out = bytearray(0x100)
    headers = [bytes(40)]
    for name, kind, flags, address, data in sections:
        out.extend(bytes((-len(out)) % 4))
        headers.append(struct.pack('<10I', name, kind, flags, address,
                                   len(out), len(data), 0, 0, 4, 0))
        out.extend(data)
    out.extend(bytes((-len(out)) % 4))
    shoff = len(out)
    out.extend(b''.join(headers))
    ident = b'\x7fELF\x01\x01\x01' + bytes(9)
    # ET_REL, entry zero, no program headers: not an executable firmware image.
    out[:52] = struct.pack('<16sHHIIIIIHHHHHH', ident, 1, 241, 1, 0, 0,
                           shoff, 0, 52, 0, 0, 40, len(headers), 4)
    return bytes(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('application', type=Path)
    parser.add_argument('--out-dir', required=True, type=Path)
    args = parser.parse_args()
    app = args.application.read_bytes()
    if hashlib.sha256(app).hexdigest() != PRO_SHA256:
        parser.error('Expected pinned decoded Pro 016 app.bin; no output written')
    layout = startup_layout(app)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    (args.out_dir / 'pro016-mapped-analysis-only.elf').write_bytes(analysis_elf(app, layout))
    (args.out_dir / 'pro016-mapped-layout.json').write_text(
        json.dumps(layout, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(layout, indent=2))


if __name__ == '__main__':
    main()
