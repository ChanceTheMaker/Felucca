# Upstream integration status

Last reviewed: **2026-10-05**. Update this record when importing or offering a
feature; follow [UPSTREAM_WORKFLOW.md](UPSTREAM_WORKFLOW.md).

Installer follow-up on `feat/rc1-installer-retry`: failed discovery, denied MIDI
permission, interrupted updates and stopped resumes show Try Again below the
output. Retrying requests MIDI access and rediscovers the device, hides stale
actions and locks installation controls. Success offers Edit in Studio. Official
recovery retries preserve the selected package and repeat backup/confirmation
checks. Eight-language labels and desktop/mobile simulated-device checks cover
both Salt14 and RC1. This website-only follow-up does not rebuild firmware or
change the draft RC1 release assets. The owner authorized merging the companion
[PR #38](https://github.com/ChanceTheMaker/Felucca/pull/38) into the candidate
branch while [PR #37](https://github.com/ChanceTheMaker/Felucca/pull/37) publishes
the same website feature with Salt14. Candidate firmware promotion remains separate.

## 1.0.1 candidate update

Pinned source: upstream v1.0.1, `20c275e39f75fa820978032efaceddfc5283c8cb`,
compared with the v1.0 baseline below. This is a selective source import,
split into lighting, scale quantization, and candidate build/version PRs.
Lighting is [PR #34](https://github.com/ChanceTheMaker/Felucca/pull/34), followed
by [QNT SEQ #35](https://github.com/ChanceTheMaker/Felucca/pull/35). The final
version/build record is on `feat/1.0.1-rc1-candidate`. These extend draft PR #28.

The lighting import adds DIM/INV preferences, idle glow, green PLAY indication,
and selected-track sequencer/ARP note LEDs. Salt font, monitor, HOLD and palette
preferences remain independent; their combined persistence is regression tested.
The hardware menu scrolls its ninth row into view, retaining readable fonts and
fixed footer controls. Input, settings, backup, UI behavior and renderer checks
cover the combined implementation. No MIDI receive or Bluetooth code is imported.

The scale import adds QNT SEQ to firmware and Studio's mock descriptors. It snaps
melodic sequence playback to the current root/scale without rewriting stored
steps, deduplicates collapsed pitches, and releases the mapped notes correctly.
Drum lanes and engines with their own key maps remain unchanged. Upstream scale
and UI regression coverage travels with the import; the browser DSP is rebuilt
from the combined source before packaging this candidate.

Validation: complete host suite; 87 unchanged golden renders; target budgets;
115 UI screens across 21 palettes; combined LED/font/monitor/HOLD round trips;
Studio descriptors/protocol, package/update and backup simulations; rebuilt WASM
audio; eight languages/479 keys; analytics isolation. Candidate artifact identity:
`1.0.1-salt1rc1`, `FM-1_910`, 609,849 bytes. See the candidate bundle's
`VALIDATION.txt` and `SHA256SUMS` for source identity and artifact checksums.

The [feature inventory](FEATURE_INVENTORY.md) records 40 feature groups and
proposed upstream contribution boundaries; [1.0.1 review](UPSTREAM_1.0.1_REVIEW.md)
records the pinned update. No new upstream PR is authorized or submitted.

Website follow-up: Install opens the initially collapsed Install Progress
accordion, scrolls to its output, and confirmed
success reveals an Edit in Studio link. Desktop/mobile, resumed success, failure,
retry and translations are tested with a simulated updater. The owner authorized
porting the UI round to the fork's main, separately from this RC1 stack. Website PRs #30–33 were merged into fork main `5874362` and published as
`gh-pages` deployment `f370b0c`, retaining Salt14. The earlier RC1 draft assets
remain unchanged; this update has a distinct candidate package.
The header logo also links home from both pages, with its typography preserved
and keyboard navigation checked on desktop and mobile. The compact hero preview
stays clear of both buttons. Section links and Install Progress scroll smoothly,
respecting reduced-motion preferences. These UI features are separate fork PRs.

## Baseline and release state

- Integrated upstream: [hugelton/Felucca v1.0.1](https://github.com/hugelton/Felucca/tree/v1.0.1),
  commit `20c275e39f75fa820978032efaceddfc5283c8cb`.
- Prior integrated baseline: v1.0, `727f272015da26eb2d0291bd652eba28ff57cb37`.
- Earlier upstream reference: `1e838e1` (0.9 plus white-key scale changes).
- Published Salt firmware: **0.9-salt14**. Current website updates retain it.
- Integration candidate: **1.0.1-salt1rc1**, hardware validation pending.
- Foundation: [PR 16](https://github.com/ChanceTheMaker/Felucca/pull/16), a pinned
  source import, not a merge of upstream Git ancestry. The baseline above is
  therefore the authoritative comparison point for the next upstream sync.
- Remaining candidate stack: TRS [#17](https://github.com/ChanceTheMaker/Felucca/pull/17),
  display [#18](https://github.com/ChanceTheMaker/Felucca/pull/18), Studio
  [#24](https://github.com/ChanceTheMaker/Felucca/pull/24), installer
  [#25](https://github.com/ChanceTheMaker/Felucca/pull/25), browser DSP
  [#26](https://github.com/ChanceTheMaker/Felucca/pull/26), regressions
  [#27](https://github.com/ChanceTheMaker/Felucca/pull/27), and final website/docs
  [#28](https://github.com/ChanceTheMaker/Felucca/pull/28). Keep these draft until
  candidate hardware validation is complete. Website PRs #15 and #19–23 are
  already merged; the live installer still serves Salt14.

## Features and retained differences

| Area | Current decision | Upstream contribution state |
| --- | --- | --- |
| 1.0 engines, FM6, songs, motion, modulation, quick layers, USB audio | Import upstream implementation and migrations | Upstream source; credit its authors |
| TRS receive ring | Retain content-driven reads with upstream overflow/timestamp handling; fork PR 17 | Potential focused contribution; check upstream before proposing |
| Palettes, fonts and MIDI monitor | Retain 21 palettes, regular/bold font and Events/Notes monitor; fork PR 18 | Additional Salt behavior; split any proposed contribution by feature |
| MIDI, palettes, favorites already credited in upstream README | Reuse current upstream behavior and test Salt extensions | Do not resubmit already adopted work |
| Protocol | SONG 33, preferences/favorites 34–38; capability detection | Follow upstream allocation; experimental Salt command 33 retired |
| Editor and installer | Current protocol, FM6/song/motion, sample trim/recording, backup-first recovery | Upstream functionality integrated into Salt interface |
| Browser synth | Rebuild from 1.0.1 DSP; 13 selectable engines, 69 presets, imported FM6 sounds | Fork feature; a contribution needs an independent upstream proposal |
| Website, themes, branding, analytics | Maintain independently in the fork | Not selected for upstream submission |
| Bluetooth experiment | Excluded from integration and release | Not selected for upstream submission |

The complete firmware host suite passes on the candidate, including 87 golden
renders and target disassembly budgets. Browser DSP and editor/backup tests pass.
Real-device installation and MIDI validation remain pending; optional DaisySP
reference checks and actual vendor V15 restoration were not run. Detailed build
evidence accompanies the candidate PRs and release artifacts.

## Contribution ledger

Studio visual panels are prepared separately on `feat/studio-visual-panels`,
above the RC1 logo branch. Related controls now share cards with editable
envelope/filter guides and LFO, scale, arpeggiator and slicer widgets. This is an
original Salt implementation, with no firmware/DSP changes or upstream source
import. See [STUDIO_VISUAL_PANELS.md](STUDIO_VISUAL_PANELS.md) for behavior,
validation and visualization limits. It has not been selected for upstream
submission or published; the upstream baseline and candidate status above remain.

Follow-up `fix/studio-piano-shortcuts` corrects the typing-key pattern for the
FM-1's F-start range. Home-row keys now consistently play white notes and the
upper row plays black notes (R = A-sharp, F = B; T is unused). The earlier
C-start pattern had assigned F to A-sharp and T to B. Labels and key events share
the corrected table. Keyboard regression tests verify each row's note colors;
`web/test_keyboard_layout.mjs` additionally verifies all seventeen displayed
shortcuts and their note-on/off behavior in Chrome against the simulated device.
The note range, MIDI handling and firmware remain unchanged. The panel feature
is [fork PR #49](https://github.com/ChanceTheMaker/Felucca/pull/49), dependent on
[RC1 branding PR #48](https://github.com/ChanceTheMaker/Felucca/pull/48); stable
branding is separately [PR #47](https://github.com/ChanceTheMaker/Felucca/pull/47).

Studio review follow-up `fix/studio-widget-polish` corrects graph handle placement
and pressed-state movement, adds parameter reset, separates arpeggiator note
bars, enlarges graph controls and labels the modulation matrix in all eight
languages. It builds on keyboard [PR #50](https://github.com/ChanceTheMaker/Felucca/pull/50).
The existing web suite, graph-model checks and expanded browser checks cover the
changes; see [STUDIO_VISUAL_PANELS.md](STUDIO_VISUAL_PANELS.md). No firmware or
upstream baseline changes, merge or publication are included.

The review fixes are [PR #51](https://github.com/ChanceTheMaker/Felucca/pull/51).
Follow-up `feat/studio-select-wheel` adds hover-only wheel selection inside open
Studio dropdowns, using native customizable select pickers with fallback to
normal menus in unsupported browsers. Wheel and keyboard behavior, live graph
updates and the full Studio layout/audio matrix pass against isolated built-site
assets. Firmware and publication remain unchanged.

Wheel selection is [PR #52](https://github.com/ChanceTheMaker/Felucca/pull/52).
Its follow-up [PR #53](https://github.com/ChanceTheMaker/Felucca/pull/53),
`feat/studio-hardware-fillers`, fills unused Sound-card space with decorative
speaker-grille holes. After visual review, patch plates and circular drivers
were removed in favor of unframed perforations. Dedicated browser checks pass
48 combinations, including unchanged card/control geometry and no overlap or
interaction. The preceding full Studio regression passed 192 layout/theme
combinations, thirteen engines, eight languages and browser audio. See
[STUDIO_VISUAL_PANELS.md](STUDIO_VISUAL_PANELS.md). This remains a fork-only
presentation change, with no upstream import, firmware change or publication.

No new upstream PR has been authorized or submitted during this round. Preparing
and documenting candidates does not imply upstream acceptance. For each future
contribution, append the following information:

| Feature | Upstream base SHA | Fork branch or PR | Upstream issue or PR | State and next action | Evidence |
| --- | --- | --- | --- | --- | --- |
| Content-driven TRS receive ring | Recheck current main | `fix/1.0-trs-ring`, fork PR 17 | None submitted in this round | Candidate for a focused proposal | Host parser/ring tests; combined hardware validation pending |

After upstream merges a contribution, record the merge SHA and the Salt PR that
removes or reconciles the duplicate implementation.
