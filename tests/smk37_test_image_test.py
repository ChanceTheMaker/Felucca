# SPDX-License-Identifier: GPL-3.0-only
import unittest
import smk37_layout_test as fixture
from smk37_test_image import replace_application
from smk37_layout import decode_flash


class TestImageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        fixture.LayoutTest.setUpClass()
        cls.stock = fixture.LayoutTest.flash + bytes(range(256)) * 256
        cls.app = bytes.fromhex('04818000') + b'TEST' * 127

    def test_roundtrip_preserves_boot_resources_and_tail(self):
        candidate = replace_application(self.stock, self.app)
        old, oldfiles = decode_flash(self.stock)
        new, files = decode_flash(candidate)
        self.assertEqual(len(candidate), len(self.stock))
        self.assertEqual(candidate[:0x4000], self.stock[:0x4000])
        end = 0x4000 + ((old['application_block_bytes'] + 31) & ~31)
        self.assertEqual(candidate[end:], self.stock[end:])
        slot = oldfiles.pop('app.bin')
        self.assertEqual(files.pop('app.bin'), self.app + b'\xff' * (len(slot)-len(self.app)))
        self.assertEqual(oldfiles, files)
        self.assertEqual(old['regions'], new['regions'])
        self.assertEqual(old['root'], new['root'])

    def test_bad_entry_empty_alignment_and_oversize(self):
        for app in (b'', b'FAIL', self.app + b'x', self.app + b'x' * 0x100000):
            with self.subTest(length=len(app)), self.assertRaises(ValueError):
                replace_application(self.stock, app)

    def test_corrupted_source_rejected(self):
        for offset in (0, 0x4000, 0x5000):
            stock = bytearray(self.stock)
            stock[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                replace_application(bytes(stock), self.app)

    def test_reproducible(self):
        self.assertEqual(replace_application(self.stock,self.app), replace_application(self.stock,self.app))


if __name__ == '__main__':
    unittest.main()
