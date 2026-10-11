# SPDX-License-Identifier: GPL-3.0-only
import sys
import struct
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_backup import compare_reads, validate_trim, inspect_backup, TRIM_ADDRESS


class BackupTest(unittest.TestCase):
    def record(self):
        r = bytearray(556)
        r[548:] = b'trimdat\0'
        for key in range(37):
            struct.pack_into('<5H', r, 168 + key * 10, 5, 640, 800, 920, 980)
        return r

    def backup(self):
        b = bytearray(b'\xff' * 0x100000)
        b[TRIM_ADDRESS:TRIM_ADDRESS + 556] = self.record()
        return b

    def test_valid_record_and_identical_reads(self):
        b = self.backup()
        self.assertEqual(compare_reads(b, b)['validated_key_curves'], 37)

    def test_difference_anywhere_is_rejected(self):
        a = self.backup()
        for address in (0, 0x4000, 0xc1000, 0xfffff):
            b = a.copy()
            b[address] ^= 1
            with self.subTest(address=address), self.assertRaisesRegex(ValueError, 'differ'):
                compare_reads(a, b)

    def test_incomplete_or_oversized_read(self):
        a = self.backup()
        for b in (a[:-1], a + b'x'):
            with self.assertRaisesRegex(ValueError, 'exactly'):
                compare_reads(a, b)

    def test_marker_and_length(self):
        r = self.record()
        for bad in (r[:-1], r + b'x', bytes(556)):
            with self.assertRaises(ValueError):
                validate_trim(bad)

    def test_every_key_rejects_nonmonotonic_and_out_of_range(self):
        for key in range(37):
            for values in ((5, 640, 640, 920, 980), (5, 800, 640, 920, 980), (5, 640, 800, 920, 1024)):
                r = self.record()
                struct.pack_into('<5H', r, 168 + key * 10, *values)
                with self.subTest(key=key, values=values), self.assertRaises(ValueError):
                    validate_trim(r)

    def test_unreviewed_package_is_rejected(self):
        b = self.backup()
        with self.assertRaisesRegex(ValueError, 'pinned'):
            inspect_backup(b, b, b'wrong model or version')


if __name__ == '__main__':
    unittest.main()
