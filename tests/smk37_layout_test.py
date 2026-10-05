# SPDX-License-Identifier: GPL-3.0-only
import sys
import struct
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
import fm1pkg_make as pack
from smk37_layout import boot_config, decode_flash, inspect_layout


def sdk_file(name):
    return {'uboot.boot': b'BOOT' * 64, 'cfg_tool.bin': b'CFG' * 10,
            'cfg/eq_cfg_hw.bin': b'EQ' * 20}[name]


def encrypted_entry(item):
    body = bytearray(item)
    pack.enc(body, 0, 32)
    return body


class LayoutTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        with patch.object(pack, 'sdk_file', side_effect=sdk_file):
            data = bytearray(pack.flash_image(b'APPLICATION' * 100, pack.KEY))
        config = pack.key_blob(pack.KEY) + b'\x04OSC_FREQ\0' + struct.pack('<I', 24000000)
        config_offset = 0xa0 + len(sdk_file('uboot.boot'))
        data[config_offset:config_offset+len(config)] = config
        data[64:96] = encrypted_entry(pack.entry(pack.crc16(config), config_offset, len(config), 2, 0x80, 0, 'isd_config.ini'))
        cls.flash = bytes(data)

    def test_valid_layout_and_payload_fingerprints(self):
        report, payloads = decode_flash(self.flash)
        self.assertEqual(report['entry_point'], 0x02000120)
        self.assertEqual(report['application_base'], 0x4000)
        self.assertEqual(report['chip_key'], pack.KEY)
        self.assertEqual(report['boot_configuration']['OSC_FREQ']['uint32_le'], 24000000)
        self.assertTrue(payloads['app.bin'].startswith(b'APPLICATION' * 100))
        self.assertEqual(len(payloads['app.bin']), pack.APP_SLOT)
        # Reserved flash regions can extend beyond the update payload.
        self.assertTrue(any(r['offset'] >= len(self.flash) for r in report['regions']))

    def test_both_fwsc_wrappers(self):
        raw = pack.ufw(self.flash, b'OTA' * 16, 'SMK-37 Pro_016')
        a, pa = inspect_layout(raw)
        logical = b''.join(raw[i:i+47] for i in range(0, 960, 48)) + raw[960:]
        product = 'SMK-37 Pro_016'
        raw36 = b''.join(logical[i*47:(i+1)*47] + bytes([ord(product[i])+i+1 if i < len(product) else 0x7d]) for i in range(36)) + logical[36*47:]
        b, pb = inspect_layout(raw36)
        self.assertEqual(a['files'], b['files'])
        self.assertEqual(pa, pb)

    def test_truncated_flash(self):
        for size in (0, 31, 63, 100, 0x4000, len(self.flash)-10000):
            with self.subTest(size=size), self.assertRaises(ValueError):
                decode_flash(self.flash[:size])

    def test_header_root_key_and_application_corruption(self):
        # Fake boot length is 256 bytes; config follows at 0xa0 + 256.
        for offset in (0, 32, 0xa0 + 256, 0x4000, 0x4200):
            data = bytearray(self.flash)
            data[offset] ^= 1
            with self.subTest(offset=offset), self.assertRaisesRegex(ValueError, 'CRC'):
                decode_flash(data)

    def test_app_base_out_of_bounds(self):
        data = bytearray(self.flash)
        data[96:128] = encrypted_entry(pack.entry(0xffff, len(data)+32, 0xffffffff, 0x81, 0xff, 0, 'app_dir_head'))
        with self.assertRaisesRegex(ValueError, 'outside'):
            decode_flash(data)

    def test_app_base_overlaps_root(self):
        data = bytearray(self.flash)
        data[96:128] = encrypted_entry(pack.entry(0xffff, 32, 0xffffffff, 0x81, 0xff, 0, 'app_dir_head'))
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            decode_flash(data)

    def test_missing_or_duplicate_key_entry(self):
        for name in ('no_config', 'app_dir_head'):
            data = bytearray(self.flash)
            data[64:96] = encrypted_entry(pack.entry(0, 0xa0+256, 34, 2, 0x80, 0, name))
            with self.assertRaisesRegex(ValueError, 'exactly one'):
                decode_flash(data)

    def test_application_file_bounds_after_valid_block_crc(self):
        data = bytearray(self.flash)
        area = bytearray(data[0x4000:])
        pack.sfc(area, 0, len(area), 0, pack.KEY)
        block_size = 0x120 + pack.APP_SLOT + len(sdk_file('cfg_tool.bin'))
        area[32:64] = pack.entry(0, block_size+32, 10, 0x82, 0xff, 0, 'app.bin')
        area[:32] = pack.entry(pack.crc16(area[32:block_size]), 0x02000120, block_size, 0x83, 0xff, 0, 'app_area_head')
        pack.sfc(area, 0, len(area), 0, pack.KEY)
        data[0x4000:] = area
        with self.assertRaisesRegex(ValueError, 'file outside'):
            decode_flash(data)

    def test_configuration_strings_and_integers(self):
        r = boot_config(b'\x04OSC\0OSC0\x04ENABLE_SDRAM\0\0\0\0\0')
        self.assertEqual(r['OSC']['ascii'], 'OSC0')
        self.assertEqual(r['ENABLE_SDRAM']['uint32_le'], 0)

    def test_malformed_configuration(self):
        for data in (b'\x04NO_TERMINATOR', b'\x04A\0x', b'\x01A\0x\x01A\0y', b'\x01../\0x'):
            with self.subTest(data=data), self.assertRaises(ValueError):
                boot_config(data)

    def test_file_overlapping_directory(self):
        data = bytearray(self.flash)
        area = bytearray(data[0x4000:])
        pack.sfc(area, 0, len(area), 0, pack.KEY)
        block_size = 0x120 + pack.APP_SLOT + len(sdk_file('cfg_tool.bin'))
        # The final region record is stable and has a known CRC; point an
        # application file at it and repair CRCs to exercise the bounds check.
        pos = 32 + 5*32
        body = area[pos:pos+32]
        area[32:64] = pack.entry(pack.crc16(body), pos, 32, 0x82, 0xff, 0, 'app.bin')
        area[:32] = pack.entry(pack.crc16(area[32:block_size]), 0x02000120, block_size, 0x83, 0xff, 0, 'app_area_head')
        pack.sfc(area, 0, len(area), 0, pack.KEY)
        data[0x4000:] = area
        with self.assertRaisesRegex(ValueError, 'overlaps directory'):
            decode_flash(data)


if __name__ == '__main__':
    unittest.main()
