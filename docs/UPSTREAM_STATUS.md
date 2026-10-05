# Upstream integration status

Last reviewed: **2026-10-05**. Update this record when importing or offering a
feature; follow [UPSTREAM_WORKFLOW.md](UPSTREAM_WORKFLOW.md).

Local website follow-up: Install now scrolls to its progress output and confirmed
success reveals an Edit in Studio link. Desktop/mobile, resumed success, failure,
retry and translations are tested with a simulated updater. Publication is on
hold at the owner's request; the published site and draft firmware assets are unchanged.

## Baseline and release state

- Upstream: [hugelton/Felucca v1.0](https://github.com/hugelton/Felucca/tree/v1.0),
  commit `727f272015da26eb2d0291bd652eba28ff57cb37`.
- Earlier upstream reference: `1e838e1` (0.9 plus white-key scale changes).
- Published Salt firmware: **0.9-salt14**. Current website updates retain it.
- Integration candidate: **1.0-salt1rc1**, hardware validation pending.
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
| Browser synth | Rebuild from 1.0 DSP; 13 selectable engines, 69 presets, imported FM6 sounds | Fork feature; a contribution needs an independent upstream proposal |
| Website, themes, branding, analytics | Maintain independently in the fork | Not selected for upstream submission |
| Bluetooth experiment | Excluded from integration and release | Not selected for upstream submission |

The complete firmware host suite passes on the candidate, including 87 golden
renders and target disassembly budgets. Browser DSP and editor/backup tests pass.
Real-device installation and MIDI validation remain pending; optional DaisySP
reference checks and actual vendor V15 restoration were not run. Detailed build
evidence accompanies the candidate PRs and release artifacts.

## Contribution ledger

No new upstream PR has been authorized or submitted during this round. Preparing
and documenting candidates does not imply upstream acceptance. For each future
contribution, append the following information:

| Feature | Upstream base SHA | Fork branch or PR | Upstream issue or PR | State and next action | Evidence |
| --- | --- | --- | --- | --- | --- |
| Content-driven TRS receive ring | Recheck current main | `fix/1.0-trs-ring`, fork PR 17 | None submitted in this round | Candidate for a focused proposal | Host parser/ring tests; combined hardware validation pending |

After upstream merges a contribution, record the merge SHA and the Salt PR that
removes or reconciles the duplicate implementation.
