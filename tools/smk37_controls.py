#!/usr/bin/env python3
"""Extract statically traced Pro 016 contact tables, offline and hash-pinned.

Indices are firmware table indices, not verified physical key/encoder order.
No device transport and no installable output.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

from smk37_startup import PRO_SHA256, XIP

KEY_TABLE = 0x02059178
ENCODER_TABLE = 0x02058510
INPUT_INIT_TABLE = 0x02058B44
BUTTON_TABLE = 0x02058685
ROW_CANDIDATES = ('PA2', 'PA5', 'PA6', 'PA7', 'PA9', 'PH6', 'PH8', 'PH9')


def contact(code):
    if not 0 <= code <= 255 or code & 15 > 7:
        raise ValueError('Contact must encode one of 16 slots and 8 input bits')
    return {'code': f'{code:02X}', 'slot': code >> 4, 'bit': code & 15,
            'gpio_candidate': ROW_CANDIDATES[code & 15]}


def contact_pairs(data, count):
    if len(data) != count * 2:
        raise ValueError('Truncated or oversized contact table')
    if len(set(data)) != len(data):
        raise ValueError('Repeated contact in table')
    return [{'index': i, 'a': contact(data[2*i]), 'b': contact(data[2*i+1])}
            for i in range(count)]


def read_contact(frame, code):
    """Host model of 0x0201BFA0: high input means open, low means closed."""
    c = contact(code)
    if len(frame) != 16:
        raise ValueError('Expected a complete 16-byte scan frame')
    return int(not frame[c['slot']] & (1 << c['bit']))


def inspect_controls(app):
    digest = hashlib.sha256(app).hexdigest()
    if digest != PRO_SHA256:
        raise ValueError('Expected pinned decoded Pro 016 app.bin')
    def take(address, size):
        start = address - XIP
        if start < 0 or start + size > len(app):
            raise ValueError('Table outside application')
        return app[start:start+size]
    keys = contact_pairs(take(KEY_TABLE, 74), 37)
    encoders = contact_pairs(take(ENCODER_TABLE, 16), 8)
    if set(take(KEY_TABLE, 74)) & set(take(ENCODER_TABLE, 16)):
        raise ValueError('Key and encoder tables overlap')
    button_codes = take(BUTTON_TABLE, 18)
    all_codes = take(KEY_TABLE, 74) + take(ENCODER_TABLE, 16) + button_codes
    if len(set(all_codes)) != len(all_codes):
        raise ValueError('Repeated contact across key, encoder and button tables')
    buttons = [{'index': i, **contact(code)} for i, code in enumerate(button_codes)]
    defaults = {}
    for name, address in [('sixteen_entry_bank', 0x0205811E),
                          ('keys', 0x02058128), ('other', 0x02058132)]:
        values = list(struct.unpack('<5H', take(address, 10)))
        if any(a >= b for a, b in zip(values, values[1:])):
            raise ValueError('Response defaults are not strictly increasing')
        defaults[name] = {'address': hex(address), 'breakpoints': values}
    return {'application_sha256': digest, 'evidence': 'static; not hardware validated',
            'physical_order': 'unverified; indices must not be assigned note names yet',
            'reader_address': '0x0201BFA0', 'consumer_address': '0x0201C0F0',
            'key_table_address': hex(KEY_TABLE), 'keys': keys,
            'encoder_table_address': hex(ENCODER_TABLE), 'encoders': encoders,
            'button_table_address': hex(BUTTON_TABLE), 'buttons': buttons,
            'adc_permutation_address': '0x02058540',
            'adc_permutation_16': list(take(0x02058540, 16)),
            'adc_four_entry_order': list(take(0x02057E1F, 4)),
            'adc_two_entry_order': list(take(0x02057E23, 2)),
            'response_defaults': defaults,
            'trim_block': {'state_offset': 11224, 'bytes': 548,
                           'marker': 'trimdat', 'physical_storage': 'unresolved'},
            'input_init_table_address': hex(INPUT_INIT_TABLE),
            'input_init_raw_values': list(struct.unpack('<8I', take(INPUT_INIT_TABLE, 32)))}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('application', type=Path)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    try:
        report = inspect_controls(args.application.read_bytes())
    except ValueError as exc:
        parser.error(str(exc))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('Extracted 37 key pairs, 8 encoder pairs, 18 buttons and response defaults; physical order remains unverified.')


if __name__ == '__main__':
    main()
