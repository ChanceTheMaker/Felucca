# Upstream integration status

Last reviewed: **2026-10-05**. Update this record when importing or offering a
feature; follow [UPSTREAM_WORKFLOW.md](UPSTREAM_WORKFLOW.md).

Installer follow-up on `feat/installer-retry`: failed discovery, denied MIDI
permission, interrupted updates and stopped resumes show Try Again below the
output. It starts a fresh connection attempt, clears stale actions and becomes
unavailable during the update. Success replaces it with Edit in Studio. Labels
are translated in all eight languages; desktop/mobile simulated-device checks
cover failure and successful retry. The firmware package remains Salt14. This
feature branch is prepared for review; it has not been published.

## Website follow-up

The owner authorized merging the completed UI round into the fork's main on
2026-10-05. Install opens a collapsed Install Progress accordion, scrolls to its
output, and shows Edit in Studio only after confirmed success. The UI is ported
independently from the RC1 branch; main retains the Salt14 installer protocol.
Simulated-updater checks cover desktop/mobile, resume, failure, retry and all
eight languages. No physical device is accessed by those checks.
Website deployment and the draft RC1 firmware remain unchanged in this round.
The header logo also links home from both pages, with its typography preserved
and visible keyboard focus. This is a separate feature PR in the same UI round.
The hero Studio preview is 320px wide and stays outside the entire action row,
including wrapped buttons. Its image contracts to fit short windows. The
full-width Studio button keeps its original preview size.
Installer section links and Install Progress now animate their scrolling, like
Back to Top. Reduced-motion preferences retain immediate navigation.

## Baseline and release state

- Upstream: [hugelton/Felucca v1.0](https://github.com/hugelton/Felucca/tree/v1.0),
  commit `727f272015da26eb2d0291bd652eba28ff57cb37`.
- Earlier upstream reference: `1e838e1` (0.9 plus white-key scale changes).
- Published Salt firmware: **0.9-salt14**. Current website updates retain it.
- Integration candidate: **1.0-salt1rc1**, hardware validation pending.
- Foundation: [PR 16](https://github.com/ChanceTheMaker/Felucca/pull/16), a pinned
  source import, not a merge of upstream Git ancestry. The baseline above is
  therefore the authoritative comparison point for the next upstream sync.

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
