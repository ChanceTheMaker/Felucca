# SPDX-License-Identifier: GPL-3.0-only
import copy
import json
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_usb import SCHEMA, compare, identity, normalize


def node(path=r'USB\VID_1234&PID_ABCD\PRIVATE-SERIAL', **kwargs):
    return dict(instance_id=path, parent_id=r'USB\ROOT_HUB30\PRIVATE-PARENT',
                name='Test keyboard', device_class='MEDIA', status='OK', **kwargs)


def snap(rows, label='test'):
    return dict(schema=SCHEMA, label=label, devices=normalize(rows))


class UsbTest(unittest.TestCase):
    def test_extract_identity_without_serial(self):
        r = normalize([node()])[0]
        self.assertEqual((r['vid'], r['pid'], r['interface']), ('1234', 'ABCD', None))
        self.assertNotIn('PRIVATE', json.dumps(r))
        self.assertEqual(r['parent_key'], identity(r'USB\ROOT_HUB30\PRIVATE-PARENT'))

    def test_interface_and_case_insensitivity(self):
        path = r'usb\vid_1234&pid_abcd&mi_02\SERIAL'
        a = normalize([node(path)])[0]
        b = normalize([node(path.upper())])[0]
        self.assertEqual(a, b)
        self.assertEqual(a['interface'], '02')

    def test_root_hubs_without_vid_pid(self):
        r = normalize([node(r'USB\ROOT_HUB30\ROOT')])[0]
        self.assertIsNone(r['vid'])
        self.assertIsNone(r['pid'])

    def test_add_remove_change_and_unchanged(self):
        a = node()
        b = node(r'USB\VID_1234&PID_ABCD&MI_00\SECOND')
        c = node(r'USB\VID_5678&PID_0001\NEW')
        changed = copy.deepcopy(a)
        changed['status'] = 'Error'
        before, after = snap([a, b]), snap([changed, c])
        r = compare(before, after)
        self.assertEqual(len(r['added']), 1)
        self.assertEqual(len(r['removed']), 1)
        self.assertEqual(len(r['changed']), 1)
        self.assertEqual(compare(before, before)['changed'], [])

    def test_two_identical_models_remain_distinct(self):
        r = normalize([node(), node(r'USB\VID_1234&PID_ABCD\SECOND-SERIAL')])
        self.assertEqual(len(r), 2)
        self.assertNotEqual(r[0]['instance_key'], r[1]['instance_key'])

    def test_bad_query_and_duplicates_rejected(self):
        for rows in ({}, [None], [node('PCI\\OTHER')], [node(), node()]):
            with self.assertRaises(ValueError):
                normalize(rows)

    def test_bad_snapshot_rejected(self):
        for invalid in ({}, dict(schema=SCHEMA, devices=[{}]),
                        dict(schema=SCHEMA, devices=[dict(instance_key='0'*64)]*2)):
            with self.assertRaises(ValueError):
                compare(invalid, snap([]))


if __name__ == '__main__':
    unittest.main()
