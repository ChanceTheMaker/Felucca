"""Reject misleading maps and independently check section bytes/addresses."""
import struct
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_startup import RAM, XIP, analysis_elf, startup_layout


def fixture():
    app = bytearray(256)
    for offset, opcode, value in [(4, 0xee, RAM+100), (10, 0xed, RAM+120),
                                  (22, 0xc3, RAM+64), (30, 0xc2, 128),
                                  (44, 0xc4, RAM), (50, 0xc1, XIP+128),
                                  (56, 0xc2, 64)]:
        app[offset:offset+6] = bytes((opcode, 0xff)) + struct.pack('<I', value)
    app[128:192] = bytes(range(64))
    return app


class StartupTests(unittest.TestCase):
    def test_map_bytes_and_nonloadable_header(self):
        app = fixture()
        elf = analysis_elf(app, startup_layout(app))
        h = struct.unpack_from('<16sHHIIIIIHHHHHH', elf)
        self.assertEqual((h[1], h[4], h[5], h[10]), (1, 0, 0, 0))
        sections = [struct.unpack_from('<10I', elf, h[6]+40*i) for i in range(h[12])]
        names = sections[4]
        strings = elf[names[4]:names[4]+names[5]]
        for i, name, address, data in [(1, b'.text', XIP, app[:128]),
                                       (2, b'.ram_copy', RAM, app[128:192]),
                                       (3, b'.tail', XIP+192, app[192:])]:
            s = sections[i]
            self.assertEqual(strings[s[0]:].split(b'\0')[0], name)
            self.assertEqual(s[3], address)
            self.assertEqual(elf[s[4]:s[4]+s[5]], data)

    def test_truncations(self):
        for length in range(192):
            with self.subTest(length=length), self.assertRaises(ValueError):
                startup_layout(fixture()[:length])

    def test_wrong_instruction(self):
        for offset in (4, 10, 22, 30, 44, 50, 56):
            app = fixture()
            app[offset] ^= 1
            with self.assertRaises(ValueError):
                startup_layout(app)

    def test_invalid_extents(self):
        for offset, value in [(52, XIP-1), (58, 0), (58, 65),
                               (24, RAM+60), (32, 0x80000),
                               (46, RAM+4), (6, RAM+256)]:
            app = fixture()
            struct.pack_into('<I', app, offset, value)
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                startup_layout(app)


if __name__ == '__main__':
    unittest.main()
