import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_controls import contact, contact_pairs, inspect_controls, read_contact


class ControlTests(unittest.TestCase):
    def test_every_single_contact_closure(self):
        for slot in range(16):
            for bit in range(8):
                frame = bytearray([255]*16)
                frame[slot] &= ~(1 << bit)
                active = [(s, b) for s in range(16) for b in range(8)
                          if read_contact(frame, s*16+b)]
                self.assertEqual(active, [(slot, bit)])

    def test_open_frame_and_pair_order(self):
        self.assertEqual(read_contact(bytes([255]*16), 0xF7), 0)
        pairs = contact_pairs(bytes([0x10, 0x52, 0x20, 0x72]), 2)
        self.assertEqual(pairs[0]['a']['slot'], 1)
        self.assertEqual(pairs[0]['b']['bit'], 2)
        self.assertEqual(pairs[1]['b']['code'], '72')

    def test_malformed_contact_and_table(self):
        for code in (-1, 8, 15, 0xF8, 256):
            with self.assertRaises(ValueError):
                contact(code)
        for data in (b'\x10', b'\x10\x10', b'\x10\x18', b'\x10\x20\x30'):
            with self.assertRaises(ValueError):
                contact_pairs(data, 1)
        with self.assertRaises(ValueError):
            read_contact(bytes(15), 0)

    def test_unknown_application_rejected(self):
        with self.assertRaisesRegex(ValueError, 'pinned'):
            inspect_controls(bytes(619704))


if __name__ == '__main__':
    unittest.main()
