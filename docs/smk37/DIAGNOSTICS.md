# SMK37 arrival diagnostics

Capture the stock keyboard's behavior before changing firmware. The recorder
opens only the exact MIDI input selected by name. It does not request identity,
open an output, send SysEx, reset controls or flash anything. `analyze` reads a
local recording and works without MIDI dependencies or hardware.

## Set up on Windows

Run in the SMK worktree. Captures and this virtual environment stay ignored
under `build/`. Close software that has exclusive access to the chosen MIDI port.

```powershell
python -m venv build/midi-env
build/midi-env/Scripts/python -m pip install -r tools/smk37-requirements.txt
New-Item -ItemType Directory -Force build/arrival
build/midi-env/Scripts/python tools/smk37_midi.py list
```

Copy the full input name from the list. If the keyboard exposes more than one
port, capture each separately with a new filename. The example name below is a
placeholder, not an assumption about the device's actual enumeration.

```powershell
build/midi-env/Scripts/python tools/smk37_midi.py capture --port "EXACT INPUT NAME" --out build/arrival/keys.jsonl --seconds 120 --label "Pro stock version recorded separately; keys soft medium hard; mode and channel noted"
python tools/smk37_midi.py analyze build/arrival/keys.jsonl
```

Ctrl+C ends the recording and closes the input. Existing capture files are
never overwritten. Release all keys/pads and the pedal before ending. A partial
file without an end record can still be analyzed and is marked incomplete.
An end reason of `error` also marks the report incomplete. A USB unplug may
appear as silence to the OS; an elapsed capture does not prove continuous
connection. Check enumeration again after disconnect tests.

## Test passes

| Recording | Actions and notes |
| --- | --- |
| `keys.jsonl` | All 37 keys, soft/medium/hard; repeated notes and chords; octave extremes |
| `pads.jsonl` | Every pad; soft/hard hits; hold and vary pressure; record pad bank/mode |
| `encoder-1.jsonl` through `encoder-8.jsonl` | One encoder per file, slow clockwise/counterclockwise, then fast; note page/latch mode |
| `faders.jsonl` | One fader at a time, full travel and return; record control order |
| `wheels.jsonl` | Pitch minimum/center/maximum, release to center; modulation full travel |
| `pedal.jsonl` | Record pedal type; press/release, hold notes, release notes before pedal |
| `transport.jsonl` | Play, stop, record and mode buttons; note which emit no MIDI |
| `reconnect.jsonl` | Record pre-disconnect behavior; reconnect and start a separate recording |

The JSON report shows channel numbers 1-16, MIDI note numbers, note-on velocity
ranges, CC ranges, 14-bit pitch-bend values, channel/poly pressure and message
counts. Relative encoders may use values such as 1/127 rather than a continuous
range: keep raw event order and directional labels before interpreting them.

Outstanding/repeated note-ons and unmatched note-offs are diagnostic leads,
not proof of audible stuck notes. Capturing mid-note, repeated triggers,
sustain and device voice allocation can affect interpretation. The balance
tracks channel/note events and honors CC120/123 and system reset, but does not
model sounding voices or sustain. Host polling timestamps cannot measure the
keyboard's internal latency. No raw SysEx content is repeated in summaries;
original recordings may contain device-specific data and remain local.

## Record hardware identity

Use [ARRIVAL_REPORT.md](ARRIVAL_REPORT.md) as the local session record. The USB
helper reads Windows' present USB PnP nodes without opening hardware handles
or sending MIDI. It hashes instance paths before saving; do not publish the
local captures merely because serial suffixes are omitted.

Run before connecting the SMK, then again after connecting it:

```powershell
python tools/smk37_usb.py snapshot --label before-connect --out build/arrival/usb-before.json
python tools/smk37_usb.py snapshot --label stock-connected --out build/arrival/usb-stock.json
python tools/smk37_usb.py diff build/arrival/usb-before.json build/arrival/usb-stock.json
```

The diff reports added, removed and changed nodes with VID/PID, interface number,
device class and status where Windows exposes them. It does not automatically
identify the keyboard: other devices can change during the same interval.
Compare composite-device parents and child interfaces, then separately list
MIDI inputs and OS audio endpoints. Some audio/MIDI endpoints live outside the
USB PnP namespace and are not included. Instance keys may change with ports or
update modes; a changed key is not proof of a different physical device.

Snapshot requires Windows PowerShell's `Get-PnpDevice` and permission to query
PnP information; offline `diff` requires only Python on any platform. Existing
files are never overwritten. After locating a documented stock update procedure,
capture that mode under a new filename and compare it with normal operation.

Save stock version, model/board revision when known, USB VID/PID, port names,
mode/channel settings, and package checksums alongside the captures. On Windows,
Device Manager's device properties provide Hardware IDs. Note which USB audio
and MIDI endpoints appear, and whether they change in the documented update
mode. Do not try undocumented button combinations or FM-1 update commands.

Use [SMK37_PREPARATION.md](../SMK37_PREPARATION.md) for the recovery and staged
bring-up requirements. These diagnostics characterize stock behavior; they do
not demonstrate that Felucca runs on the keyboard yet.

## Offline verification

```sh
python -m unittest discover -s tests -p "smk37_*_test.py"
```

The suite covers both FWSC wrappers and malformed containers, board selection,
MIDI channels/zero-velocity releases, repeated notes, controller/pressure ranges,
SysEx framing, malformed records and receive-only capture lifecycle. The MIDI
backend is simulated; physical port reliability remains an arrival test.

USB tests cover composite interfaces, missing VID/PID on hubs, case-insensitive
instance paths, serial omission, separate identical models, malformed snapshots
and added/removed/changed nodes. Actual host enumeration can be checked before
arrival; it does not establish keyboard behavior.
