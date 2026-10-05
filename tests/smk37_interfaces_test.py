"""Static research must not turn malformed bytes into asserted hardware IDs."""
import sys
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_interfaces import DEVICE, inspect_interfaces, scan_application


def descriptor(**changes):
    values = dict(length=18, kind=1, usb=0x0200, cls=0, sub=0, proto=0,
                  packet=64, vid=0x4353, pid=0x344b, release=0x0100,
                  manufacturer=0, product=2, serial=3, configs=1)
    values.update(changes)
    return DEVICE.pack(*values.values())


class InterfaceTests(unittest.TestCase):
    def test_unaligned_and_distinct_model_ids(self):
        data = b'abc' + descriptor() + b'X' + descriptor(pid=0x354b)
        items = scan_application(data)['usb_device_candidates']
        self.assertEqual([x['app_offset'] for x in items], [3, 22])
        self.assertEqual([x['pid'] for x in items], ['344B', '354B'])
        self.assertEqual(items[0]['vid'], '4353')
        self.assertEqual(items[0]['product_string_index'], 2)
        self.assertEqual(items[0]['serial_string_index'], 3)

    def test_every_truncation(self):
        for length in range(18):
            with self.subTest(length=length):
                self.assertEqual(scan_application(descriptor()[:length])['usb_device_candidates'], [])

    def test_false_positive_filters(self):
        for field, value in [('usb', 0x9999), ('packet', 3), ('vid', 0),
                             ('vid', 0xffff), ('release', 0x01af), ('configs', 0),
                             ('length', 17), ('kind', 2)]:
            with self.subTest(field=field, value=value):
                self.assertEqual(scan_application(descriptor(**{field: value}))['usb_device_candidates'], [])

    def test_pid_zero_is_not_invalidated(self):
        self.assertEqual(scan_application(descriptor(pid=0))['usb_device_candidates'][0]['pid'], '0000')

    def test_strings_keep_offsets_and_do_not_chunk_long_data(self):
        data = b'\0usb_update_mode\0ordinary text\0' + b'x'*180 + b'lcd\0midi_route\0'
        result = scan_application(data)['diagnostic_strings']
        self.assertEqual([x['text'] for x in result], ['usb_update_mode', 'midi_route'])
        self.assertEqual(result[0]['app_offset'], 1)
        self.assertEqual(result[1]['app_offset'], data.index(b'midi_route'))

    def test_invalid_package_fails_before_scan(self):
        with patch('smk37_interfaces.scan_application') as scan:
            with self.assertRaises(ValueError):
                inspect_interfaces(b'not a validated firmware package')
            scan.assert_not_called()

    def test_report_keeps_hash_and_uncertainty(self):
        layout = dict(product='test', package_sha256='package hash',
                      files=[dict(name='app.bin', sha256='app hash')])
        with patch('smk37_interfaces.inspect_layout', return_value=(layout, {'app.bin': descriptor()})):
            report = inspect_interfaces(b'fixture')
        self.assertEqual(report['application_sha256'], 'app hash')
        self.assertEqual(report['application_bytes'], 18)
        self.assertIn('unverified', report['evidence'])
        self.assertIn('not a physical address', report['offset_basis'])


if __name__ == '__main__':
    unittest.main()
