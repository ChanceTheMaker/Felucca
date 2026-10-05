# Upstream Felucca 1.0.1 review

Reviewed on 2026-10-05 for the Salt owner. Upstream 1.0.1 adds panel-lighting
improvements and live scale quantization of sequenced notes. It does not add
another sound engine or change the editor command allocation. The owner authorized importing this update into the candidate. It is now
combined with Salt as **1.0.1-salt1rc1**; hardware validation remains pending.

Exact comparison:

- Base v1.0: `727f272015da26eb2d0291bd652eba28ff57cb37`
- New v1.0.1: `20c275e39f75fa820978032efaceddfc5283c8cb`
- Published upstream release: 2026-10-05.
- Delta: 21 files, 733 insertions and 54 deletions.
- The updated RC1 is based on v1.0.1. Public Salt firmware remains 0.9-salt14.

Sources: [official release notes](https://github.com/hugelton/Felucca/releases/tag/v1.0.1)
and the fetched source comparison
[v1.0 to v1.0.1](https://github.com/hugelton/Felucca/compare/v1.0...v1.0.1).
The implementation details and integration recommendations below come from
reviewing that source delta; they are not claims of testing on an FM-1.

## Changes visible to users

| Change | Behavior |
| --- | --- |
| **Idle LED glow** | Otherwise inactive buttons and keys glow dimly, making controls easier to find in a dark room. Active indicators remain brighter and blinking states remain distinct. |
| **MENU > LEDS** | Choose DIM, the new default, or INV. INV lights idle controls and darkens active ones, similar to the official firmware. The preference is saved. |
| **Green PLAY light** | Uses PLAY's green LED while transport is running. |
| **Playback lights on the keys** | The selected track's sequencer and arpeggiator illuminate corresponding keys in the current keyboard mapping/octave. Existing special key maps retain their own meaning. |
| **QNT SEQ** | A fourth SCL quantization mode: keys behave like SNAP, and sequenced notes are mapped into the current root/scale as they play. Stored steps are unchanged; changing scale can reharmonize a pattern live. It is opt-in. |
| **Song playback indicator** | A disc replaces the play triangle in the header during song playback. Advertised in the release notes, but already present in the v1.0 source and our earlier RC1; no additional patch needed. |

For example, a previously recorded melodic pattern can be played through a
different root/scale with QNT SEQ while retaining the original note data. Turning
the mode off restores playback of the stored pitches. This is pitch mapping,
not timing quantization. Drum/lane hits and engines with their own key mapping,
such as slice triggering, are excluded by the implementation.

## Implementation and compatibility details

- **Lighting:** `firmware/hal/fm1_input.h` adds dim-LED state and pulses during
  the existing shift-register scan. Console input diagnostics report pulse
  timing. Upstream describes the approach as avoiding flicker and extra CPU;
  we have not independently measured those hardware claims.
- **LED preference storage:** `panel.c` and `settings_persist.c` encode LED mode
  with a tag in the retired `zoom` field. Legacy values 0/1 remain accepted and
  default to DIM. `editor_backup.c` accepts the tagged setting during restore.
- **Key lights:** `ui_input.c` maps the selected track's currently sounding
  sequencer/arp notes back to available keys. Notes outside that mapping are
  not shown. Special layers, name entry and other key maps remain distinct.
- **Scale playback:** `params.c` appends SEQ to OFF/SNAP/WHITE, preserving old
  numeric values. `seq.c` maps playback pitches downward to the scale, merges
  duplicate pitches created by snapping, and keeps note-off tracking aligned
  with the pitches actually played. Source steps are not rewritten.
- **Studio:** the mock device's QNT choices in `web/editor.html` gain SEQ.
  Real-device parameter descriptors continue to supply their choices.
- **Build numbering:** `tools/build.py --release` accepts an X.Y.Z release
  string. The device product identity still derives from X.Y: both 1.0 and
  1.0.1 use `FM-1_910`. That identity alone cannot distinguish the patch release.
- **Tests:** upstream adds/extends input, UI, scale, settings and backup tests,
  plus an instruction-budget check. These tests are included in the combined candidate validation.

There are no edits to `firmware/src/midi_uart.c` or `firmware/hal/fm1_uart.h`
between these tags. Upstream 1.0.1 still uses the receive-count-based ring drain;
the Salt stuck-note fix remains a separate contribution candidate. No editor
protocol-document or preference-command allocation change appears in this delta.

## Impact on Salt

The update is divided into lighting, scale quantization, and candidate build/docs
PRs. The earlier 1.0-salt1rc1 artifact is retained; the new package is named
1.0.1-salt1rc1. Salt14 stays published until hardware validation is complete.
The new LEDS row coexists with FONT and MIDI MON in a scrolling menu; combined
persistence and layout checks cover these retained Salt features.

| Area | Reconciliation needed |
| --- | --- |
| Menu and display | Merge the new LEDS row with Salt's FONT and MIDI MON rows; verify spacing, navigation and all palettes. Both branches edit the same menu. |
| Persistent settings | Preserve LEDS in the retired zoom field and Salt's HOLD/font/monitor encoding in the bold field. They use different fields, but share import/export and validation code. Test old settings, reboot, pending saves and backup/restore. |
| Sequencer | Retain the MIDI-monitor receive hook while incorporating QNT SEQ. Verify note-off behavior, ties, duplicates, root/scale changes and excluded drum/slice mappings. |
| Input hardware | Exercise dim/inverted modes, PLAY, key lights, layers, encoder/key scanning and playback on a real FM-1. |
| Studio and browser | Update the candidate mock QNT choice and review the browser's parameter tables. Browser mode remains a one-sound preview; importing SEQ does not add browser sequencing. Rebuild dependent assets where their inputs change. |
| TRS fix | Retain the separate ring fix and rerun its regression/hardware checks. Upstream's new LED activity needs to coexist with reliable MIDI handling. |
| Versioning | Choose a new candidate version before distributing rebuilt firmware, regenerate matching source/checksums and retain Salt14 until hardware validation passes. |

Candidate software checks include: upstream's new LED/input/
scale/backup tests, Salt monitor/font/palette/persistence tests, the combined host
suite and target budgets, browser/editor checks, with physical FM-1 verification still pending.
See [feature inventory](FEATURE_INVENTORY.md),
[integration status](UPSTREAM_STATUS.md) and
[upstream workflow](UPSTREAM_WORKFLOW.md).
