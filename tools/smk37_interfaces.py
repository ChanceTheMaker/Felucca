#!/usr/bin/env python3
"""Find USB descriptor and diagnostic-string candidates in stock SMK packages.

Offline, metadata only. Candidates are not observed devices or a board pinout.
Package/layout CRCs are checked before scanning the decoded application.
"""
import argparse
import json
import re
import struct
from pathlib import Path

from smk37_layout import inspect_layout
from smk37_package import MAX_BYTES


# USB 2 device descriptor fields, not a vendor/model allowlist. Other USB
# generations/formats are outside this deliberately narrow research scanner.
DEVICE = struct.Struct('<BBHBBBBHHHBBBB')
LEADS = re.compile(rb'lcd|st77|ili9|adc|alnk|uart|gpio|iic|matrix|keyscan|'
                   rb'cs434|key_scan|usb|midi|smk|spi|sdram', re.I)


def bcd_valid(value):
    return all(((value >> shift) & 15) <= 9 for shift in (0, 4, 8, 12))


def scan_application(app):
    """Offsets are into decoded app.bin, never physical flash addresses."""
    candidates = []
    for match in re.finditer(b'\x12\x01', app):
        offset = match.start()
        if offset + DEVICE.size > len(app):
            continue
        (_, _, usb, cls, sub, proto, packet, vid, pid, release,
         manufacturer, product, serial, configs) = DEVICE.unpack_from(app, offset)
        if (usb not in (0x0100, 0x0110, 0x0200, 0x0210) or
                packet not in (8, 16, 32, 64) or not configs or
                vid in (0, 0xffff) or not bcd_valid(release)):
            continue
        candidates.append(dict(
            app_offset=offset, usb_bcd=f'{usb:04X}', device_class=cls,
            device_subclass=sub, device_protocol=proto, ep0_packet_bytes=packet,
            vid=f'{vid:04X}', pid=f'{pid:04X}', device_bcd=f'{release:04X}',
            manufacturer_string_index=manufacturer, product_string_index=product,
            serial_string_index=serial, configurations=configs))
    # Limit text length and require a real string boundary so arbitrary long
    # printable blobs are not broken into misleading short matches.
    strings = []
    for match in re.finditer(rb'[\x20-\x7e]{5,}', app):
        value = match.group()
        if len(value) <= 160 and LEADS.search(value):
            strings.append(dict(app_offset=match.start(), text=value.decode('ascii')))
    return dict(usb_device_candidates=candidates, diagnostic_strings=strings)


def inspect_interfaces(raw):
    layout, payloads = inspect_layout(raw)
    app = payloads['app.bin']
    identity = next(item for item in layout['files'] if item['name'] == 'app.bin')
    return dict(
        product=layout['product'], package_sha256=layout['package_sha256'],
        application_sha256=identity['sha256'], application_bytes=len(app),
        offset_basis='Byte offset within decoded app.bin; not a physical address',
        evidence='Static candidates only; runtime enumeration and code references unverified',
        **scan_application(app))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('packages', type=Path, nargs='+')
    args = parser.parse_args()
    reports = []
    try:
        for path in args.packages:
            with path.open('rb') as source:
                report = inspect_interfaces(source.read(MAX_BYTES + 1))
            reports.append(dict(file=path.name, **report))
    except (OSError, ValueError) as error:
        parser.exit(2, f'smk37_interfaces: {error}\n')
    print(json.dumps(reports, indent=2))


if __name__ == '__main__':
    main()
