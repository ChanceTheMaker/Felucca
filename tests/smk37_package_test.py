# SPDX-License-Identifier: GPL-3.0-only
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import fm1pkg_make as pack
from smk37_package import inspect_bytes


def unwrap(raw):
    return bytearray(b''.join(raw[i:i+47] for i in range(0, 960, 48)) + raw[960:])


def wrap(logical, blocks=20):
    product = 'SMK-37 Pro_016'
    raw = bytearray()
    for i in range(blocks):
        marker = ord(product[i]) + i + 1 if i < len(product) else 0x7d
        raw += logical[i*47:(i+1)*47] + bytes([marker])
    return bytes(raw + logical[blocks*47:])


def rewrite_entry(raw, mutate):
    logical = unwrap(raw)
    entry = bytearray(logical[64:144])
    pack.enc(entry, 0, 80)
    mutate(entry)
    pack.enc(entry, 0, 80)
    logical[64:144] = entry
    header = bytearray(logical[:64])
    pack.enc(header, 0, 64)
    struct.pack_into('<H', header, 2, pack.crc16(logical[64:224]))
    struct.pack_into('<H', header, 0, pack.crc16(header[2:]))
    pack.enc(header, 0, 64)
    logical[:64] = header
    return wrap(logical)


class PackageTest(unittest.TestCase):
    def setUp(self):
        self.raw = pack.ufw(b'F' * 4096, b'O' * 256, 'SMK-37 Pro_016')

    def test_20_and_36_marker_wrappers_have_identical_payloads(self):
        a = inspect_bytes(self.raw)
        b = inspect_bytes(wrap(unwrap(self.raw), 36))
        self.assertEqual(a['marker_blocks'], 20)
        self.assertEqual(b['marker_blocks'], 36)
        self.assertNotEqual(a['sha256'], b['sha256'])
        self.assertEqual(a['logical_sha256'], b['logical_sha256'])
        self.assertEqual(a['entries'], b['entries'])
        self.assertEqual(b['compatibility'], 'unverified')

    def test_corrupt_header_directory_and_payload(self):
        for index in (0, 85, 3000):
            raw = bytearray(self.raw)
            raw[index] ^= 1
            with self.subTest(index=index), self.assertRaisesRegex(ValueError, 'CRC mismatch'):
                inspect_bytes(raw)

    def test_truncation_and_trailing_bytes(self):
        for raw in (b'', self.raw[:959], self.raw[:-1], self.raw + b'X'):
            with self.assertRaises(ValueError):
                inspect_bytes(raw)

    def test_directory_cannot_point_into_header(self):
        raw = rewrite_entry(self.raw, lambda e: struct.pack_into('<I', e, 8, 0))
        with self.assertRaisesRegex(ValueError, 'bounds'):
            inspect_bytes(raw)

    def test_directory_cannot_point_outside_file(self):
        raw = rewrite_entry(self.raw, lambda e: struct.pack_into('<I', e, 12, 0xffffffff))
        with self.assertRaisesRegex(ValueError, 'bounds'):
            inspect_bytes(raw)

    def test_overlapping_payloads(self):
        raw = rewrite_entry(self.raw, lambda e: struct.pack_into('<III', e, 8, 5120, 256, 256))
        # Payload CRC must match to get as far as the second entry's overlap check.
        raw = rewrite_entry(raw, lambda e: struct.pack_into('<H', e, 4, pack.crc16(b'O' * 256)))
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            inspect_bytes(raw)

    def test_model_marker_is_not_a_device_compatibility_check(self):
        raw = bytearray(self.raw)
        raw[47] = ord('X') + 1
        result = inspect_bytes(raw)
        self.assertTrue(result['product'].startswith('X'))
        self.assertEqual(result['compatibility'], 'unverified')


if __name__ == '__main__':
    unittest.main()
