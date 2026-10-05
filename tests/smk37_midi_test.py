# SPDX-License-Identifier: GPL-3.0-only
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import Mock

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from smk37_midi import SCHEMA, analyze, capture


def records(packets):
    return [json.dumps(dict(kind='session', schema=SCHEMA, receive_only=True)),
            *(json.dumps(dict(kind='midi', t=i / 10, data=data)) for i, data in enumerate(packets))]


class MidiTest(unittest.TestCase):
    def test_zero_velocity_off_and_channels_are_separate(self):
        r = analyze(records([[0x90, 60, 1], [0x91, 60, 127], [0x90, 60, 0]]))
        self.assertEqual(r['outstanding_note_ons'], [dict(channel=2, note=60, count=1)])
        self.assertEqual(r['ranges']['ch1/note/60/velocity']['min'], 1)
        self.assertEqual(r['message_counts']['note_off'], 1)
        self.assertFalse(r['complete'])

    def test_repeated_notes_unmatched_off_and_all_notes_off(self):
        r = analyze(records([[0x90, 60, 20], [0x90, 60, 30], [0x80, 60, 0],
                             [0x80, 61, 0], [0xb0, 123, 0]]))
        self.assertEqual(r['repeated_note_ons'], 1)
        self.assertEqual(r['unmatched_note_offs'], 1)
        self.assertEqual(r['outstanding_note_ons'], [])

    def test_pitch_pressure_faders_sustain(self):
        r = analyze(records([[0xe0, 0, 0], [0xe0, 0, 64], [0xe0, 127, 127],
                             [0xa0, 36, 99], [0xd0, 100], [0xb0, 7, 0], [0xb0, 7, 127],
                             [0x90, 60, 80], [0xb0, 64, 127], [0x80, 60, 0]]))
        self.assertEqual(r['ranges']['ch1/pitch_bend'], dict(min=0, max=16383, count=3))
        self.assertEqual(r['ranges']['ch1/cc/7']['max'], 127)
        self.assertEqual(r['ranges']['ch1/note/36/pressure']['max'], 99)
        self.assertEqual(r['outstanding_note_ons'], [])

    def test_system_packets(self):
        r = analyze(records([[0xf0, 0, 50, 0xf7], [0xf8], [0xfa], [0xfc]]))
        self.assertEqual(r['events'], 4)
        self.assertEqual(r['message_counts']['system_f0'], 1)

    def test_reject_malformed_messages_and_time(self):
        for data in ([1], [0x90, 60], [0x90, 60, 128], [0xf0, 1], [0xf0, 0x90, 0xf7], [0xc0, 1, 2]):
            with self.subTest(data=data), self.assertRaises(ValueError):
                analyze(records([data]))
        for t in (-1, float('nan'), float('inf')):
            with self.assertRaisesRegex(ValueError, 'timestamps'):
                analyze(records([]) + [json.dumps(dict(kind='midi', t=t, data=[0xf8]))])

    def test_receive_only_capture_and_no_overwrite(self):
        # This fake exposes no send, output, or bidirectional port operations.
        class Input:
            def __enter__(self): return self
            def __exit__(self, *args): self.closed = True
            def poll(self):
                if self.used: raise KeyboardInterrupt
                self.used = True
                return Mock(bytes=lambda: [0x90, 60, 100])
            used, closed = False, False
        inp = Input()
        backend = Mock(spec=['get_input_names', 'open_input'])
        backend.get_input_names.return_value = ['SMK test input']
        backend.open_input.return_value = inp
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / 'capture.jsonl'
            self.assertEqual(capture(backend, 'SMK test input', dest, 60, 'key'), 1)
            self.assertTrue(inp.closed)
            result = analyze(dest.read_text().splitlines())
            self.assertTrue(result['complete'])
            self.assertEqual(result['recording_end']['reason'], 'interrupted')
            with self.assertRaises(FileExistsError):
                capture(backend, 'SMK test input', dest, 60, 'key')
            backend.open_input.assert_called_once_with('SMK test input')

    def test_missing_port_and_bad_duration_do_not_open(self):
        backend = Mock(spec=['get_input_names', 'open_input'])
        backend.get_input_names.return_value = []
        for seconds in (60, -1, float('nan')):
            with self.assertRaises(ValueError):
                capture(backend, 'absent', Path('unused'), seconds, 'test')
        backend.open_input.assert_not_called()

    def test_end_record_integrity(self):
        header = records([])
        with self.assertRaisesRegex(ValueError, 'incorrect end'):
            analyze(header + [json.dumps(dict(kind='end', reason='duration', events=1))])
        with self.assertRaisesRegex(ValueError, 'record after end'):
            analyze(header + [json.dumps(dict(kind='end', reason='duration', events=0)), '{}'])

    def test_backend_failure_preserves_incomplete_capture(self):
        backend = Mock(spec=['get_input_names', 'open_input'])
        backend.get_input_names.return_value = ['SMK input']
        backend.open_input.side_effect = OSError('disconnected')
        with tempfile.TemporaryDirectory() as tmp:
            dest = Path(tmp) / 'capture.jsonl'
            with self.assertRaisesRegex(OSError, 'disconnected'):
                capture(backend, 'SMK input', dest, 60, 'disconnect test')
            result = analyze(dest.read_text().splitlines())
            self.assertFalse(result['complete'])
            self.assertEqual(result['recording_end']['reason'], 'error')

    def test_backward_time_rejected(self):
        rows = records([[0xf8], [0xf8]])
        rows.append(json.dumps(dict(kind='midi', t=0.01, data=[0xf8])))
        with self.assertRaisesRegex(ValueError, 'timestamps'):
            analyze(rows)


if __name__ == '__main__':
    unittest.main()
