#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Receive-only SMK stock MIDI baseline capture and offline JSONL analysis.

List/capture needs mido + python-rtmidi. Analyze needs only Python's stdlib.
Never opens an output port or sends discovery, reset or firmware messages.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import time

SCHEMA = 'felucca-smk37-midi-v1'


def capture(backend, port, destination, seconds, label, clock=time.monotonic, sleep=time.sleep):
    if not math.isfinite(seconds) or seconds <= 0:
        raise ValueError('seconds must be a positive finite number')
    if port not in backend.get_input_names():
        raise ValueError('exact input port not found; run list and copy its full name')
    # Exclusive creation avoids destroying a previous baseline.
    with destination.open('x', encoding='utf-8') as output:
        def write(record):
            output.write(json.dumps(record) + '\n')
            output.flush()
        write(dict(kind='session', schema=SCHEMA, port=port, label=label,
                   utc=datetime.now(timezone.utc).isoformat(), receive_only=True))
        count, reason = 0, 'error'
        try:
            with backend.open_input(port) as inp:
                start = clock()
                while clock() - start < seconds:
                    msg = inp.poll()
                    if msg is not None:
                        write(dict(kind='midi', t=round(clock() - start, 6), data=list(msg.bytes())))
                        count += 1
                    else:
                        sleep(0.001)
                reason = 'duration'
        except KeyboardInterrupt:
            reason = 'interrupted'
        finally:
            write(dict(kind='end', reason=reason, events=count))
    return count


def validate_packet(data):
    if not isinstance(data, list) or not data or any(type(b) is not int or not 0 <= b <= 255 for b in data):
        raise ValueError('invalid MIDI byte list')
    status = data[0]
    if status < 0x80:
        raise ValueError('MIDI status missing (capture uses complete messages)')
    if status == 0xf0:
        if len(data) < 2 or data[-1] != 0xf7 or any(b > 127 for b in data[1:-1]):
            raise ValueError('invalid SysEx frame')
        return
    lengths = {0xf1: 2, 0xf2: 3, 0xf3: 2, 0xf6: 1,
               0xf8: 1, 0xfa: 1, 0xfb: 1, 0xfc: 1, 0xfe: 1, 0xff: 1}
    length = (2 if status >> 4 in (0xc, 0xd) else 3) if status < 0xf0 else lengths.get(status)
    if len(data) != length or any(b > 127 for b in data[1:]):
        raise ValueError('invalid MIDI message length or data')


def analyze(lines):
    session, ended, end = None, False, None
    events, types, active, ranges = 0, Counter(), Counter(), {}
    unmatched, repeated, last_t = 0, 0, 0.0

    def observe(key, value):
        r = ranges.setdefault(key, dict(min=value, max=value, count=0))
        r['min'], r['max'] = min(r['min'], value), max(r['max'], value)
        r['count'] += 1

    for number, line in enumerate(lines, 1):
        try:
            r = json.loads(line)
            if not isinstance(r, dict):
                raise ValueError('record must be an object')
            if session is None:
                if r.get('kind') != 'session' or r.get('schema') != SCHEMA or r.get('receive_only') is not True:
                    raise ValueError('missing receive-only session header')
                session = r
                continue
            if ended:
                raise ValueError('record after end')
            if r.get('kind') == 'end':
                if r.get('events') != events or r.get('reason') not in ('duration', 'interrupted', 'error'):
                    raise ValueError('incorrect end record')
                ended, end = True, r
                continue
            if r.get('kind') != 'midi':
                raise ValueError('unknown record kind')
            t, data = r.get('t'), r.get('data')
            if type(t) not in (int, float) or not math.isfinite(t) or t < last_t:
                raise ValueError('timestamps must be finite, nonnegative and monotonic')
            validate_packet(data)
            last_t, events = t, events + 1
            status = data[0]
            if status >= 0xf0:
                types[f'system_{status:02x}'] += 1
                if status == 0xff:
                    active.clear()
                continue
            channel, kind = (status & 15) + 1, status >> 4
            prefix = f'ch{channel}'
            off = kind == 8 or (kind == 9 and data[2] == 0)
            name = 'note_off' if off else {9: 'note_on', 10: 'poly_pressure', 11: 'cc',
                                         12: 'program', 13: 'channel_pressure', 14: 'pitch_bend'}[kind]
            types[name] += 1
            if kind in (8, 9):
                key = (channel, data[1])
                if off:
                    if active[key]:
                        active[key] -= 1
                    else:
                        unmatched += 1
                else:
                    repeated += int(active[key] > 0)
                    active[key] += 1
                    observe(f'{prefix}/note/{data[1]}/velocity', data[2])
            elif kind == 11:
                observe(f'{prefix}/cc/{data[1]}', data[2])
                if data[1] in (120, 123):
                    for key in list(active):
                        if key[0] == channel:
                            del active[key]
            elif kind == 14:
                observe(f'{prefix}/pitch_bend', data[1] | (data[2] << 7))
            elif kind == 10:
                observe(f'{prefix}/note/{data[1]}/pressure', data[2])
            else:
                observe(f'{prefix}/{name}', data[1])
        except (ValueError, KeyError, TypeError) as error:
            raise ValueError(f'line {number}: {error}') from error
    if session is None:
        raise ValueError('empty capture')
    return dict(session=session, events=events, last_event_seconds=last_t,
                recording_end=end, complete=ended and end['reason'] != 'error',
                message_counts=dict(sorted(types.items())), ranges=dict(sorted(ranges.items())),
                unmatched_note_offs=unmatched, repeated_note_ons=repeated,
                outstanding_note_ons=[dict(channel=c, note=n, count=v) for (c, n), v in sorted(active.items()) if v],
                note_balance_scope='MIDI event balance only; sustain and sounding voice state are not inferred')


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    commands = ap.add_subparsers(dest='command', required=True)
    commands.add_parser('list', help='list MIDI input names without opening them')
    cap = commands.add_parser('capture')
    cap.add_argument('--port', required=True, help='exact input name; never auto-selects a device')
    cap.add_argument('--out', type=Path, required=True)
    cap.add_argument('--seconds', type=float, default=60)
    cap.add_argument('--label', required=True, help='physical control, mode and settings being tested')
    report = commands.add_parser('analyze')
    report.add_argument('capture', type=Path)
    args = ap.parse_args()
    try:
        if args.command == 'analyze':
            with args.capture.open(encoding='utf-8') as source:
                print(json.dumps(analyze(source), indent=2))
            return
        import mido
        backend = mido.Backend('mido.backends.rtmidi')
        if args.command == 'list':
            print(json.dumps(backend.get_input_names(), indent=2))
        else:
            count = capture(backend, args.port, args.out, args.seconds, args.label)
            print(f'Saved {count} received events to {args.out}')
    except ImportError:
        ap.exit(2, 'Install mido and python-rtmidi for list/capture; analyze works without them.\n')
    except (OSError, RuntimeError, ValueError) as error:
        ap.exit(2, f'smk37_midi: {error}\n')


if __name__ == '__main__':
    main()
