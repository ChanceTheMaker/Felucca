# Salt feature inventory and upstream contribution choices

Reviewed on 2026-10-05. This inventory records the features developed for Felucca
[Salt], what current upstream already includes, and which remaining differences
could become focused contributions. The numbered entries are feature groups,
not a claim that each should become a separate PR. Recommendations below are
proposals for the owner's decision; no new upstream submission is authorized
by this document.

Comparison points:

- Published Salt source: fork main `5874362969e021b3e16e63697a9de926972b848f`.
- Published website: `gh-pages` deployment `f370b0c`, retaining **0.9-salt14** firmware.
- Candidate: **1.0.1-salt1rc1**, based on upstream v1.0.1
  `20c275e39f75fa820978032efaceddfc5283c8cb`; hardware validation pending.
- Latest upstream inspected: **v1.0.1**, commit
  `20c275e39f75fa820978032efaceddfc5283c8cb`. Imported into the updated RC1 in focused feature PRs.
- RC1-local UI refinements are also represented by the independently merged
  website PRs #30-33 on main.

Upstream credits ChanceTheMaker for TRS MIDI, bend, sustain and clock, palettes,
favorites, and editor display settings, citing upstream PRs #8, #10, #11 and
#12. That establishes adoption or adaptation of functionality; it does not mean
every original PR was merged unchanged. Credit other contributors when their
work was integrated into ours. See [upstream credits](https://github.com/hugelton/Felucca/blob/20c275e39f75fa820978032efaceddfc5283c8cb/README.md#credits).

## Firmware features

| ID | Feature and behavior | Current upstream position and proposed action |
| --- | --- | --- |
| 1 | **TRS MIDI input.** Enabled TRS reception in normal builds and integrated it with shared USB/TRS track routing. | Adopted. Do not resubmit the original feature. |
| 2 | **Expressive MIDI controls.** Pitch bend, runtime bend sensitivity through RPN, mod-wheel behavior, sustain, All Notes Off, All Sound Off and Reset All Controllers. Handles repeated notes, sustained notes, overlapping sources and track-selection changes. | The foundations are adopted/adapted. Original Salt expression behavior is not necessarily identical to current upstream modulation. Offer only a demonstrated remaining fix. |
| 3 | **Clock-source controls.** Studio selection of internal, USB or TRS clock and integration of clock/transport behavior. | Present upstream. This work incorporates contributions from others, including keremimo; preserve attribution. |
| 4 | **TRS stuck-note fix.** Integrates keremimo's receive-ring fix: drains actual DMA contents rather than treating the reported byte count as the normal read limit. Addresses a delayed note-off that waits for another incoming byte. Our RC1 port retains upstream timestamp and overflow handling. | **First proposed contribution.** Preserve keremimo's authorship and check for an existing upstream submission. Current 1.0.1 still uses the count-driven approach. Reproduce and validate on current hardware/firmware; include focused regression tests. |
| 5 | **Expanded screen palettes.** Salt14 has 20 palettes. RC1 preserves upstream's first eight IDs and supplies 21 palettes in total, adding dark, light, two-color and high-contrast choices, with legacy-palette migration. | Basic palette support is adopted. Offer the extra palette set and its migration behavior separately. |
| 6 | **Regular or bold screen text.** Salt14 selects font weights. RC1 restores a heavier rendering option while keeping upstream's new glyph dimensions and layout. | Current upstream has one weight. Good accessibility proposal; include menu, settings persistence and editor support. |
| 7 | **Track and status colors.** Distinct tracks, playback, recording, armed state, USB and TRS activity, with labels/shapes retained and readable light-theme accents. | Upstream's redesigned UI overlaps much of this. Compare individual remaining differences before proposing a patch. |
| 8 | **On-device MIDI monitor.** OFF, EVENTS and NOTES modes. Shows USB/TRS source and events or the selected track's held/sustained note names. Displays up to four pitches plus an additional-note count. Clock events do not immediately hide other useful events. | Strong separate proposal. Upstream reserves a monitor capability but lacks our implementation. Note display shows chord tones, not named chord recognition. |
| 9 | **Persistent preset favorites.** Stars factory sounds and user slots; All/Favorites browsing; remembered filter; sensible behavior for renamed, overwritten and deleted slots; no repeated entries in short lists. | Adopted. Submit only new fixes with a reproduction. |
| 10 | **Persistent display preferences.** Saves palette, font weight and monitor mode. RC1's tagged storage preserves the upstream HOLD preference alongside Salt extensions. | Implementation support for #5, #6 and #8. Review migrations and backup validation carefully when rebasing onto 1.0.1's new LED setting. |

Evidence: [Salt14 MIDI behavior](MIDI-EXPRESSION.txt), original contributions
[#8](https://github.com/hugelton/Felucca/pull/8),
[#10](https://github.com/hugelton/Felucca/pull/10),
[#11](https://github.com/hugelton/Felucca/pull/11),
[#12](https://github.com/hugelton/Felucca/pull/12), and candidate fork
[TRS PR #17](https://github.com/ChanceTheMaker/Felucca/pull/17) and
[display PR #18](https://github.com/ChanceTheMaker/Felucca/pull/18).
The candidate's firmware delta against `727f272` is the source of the remaining
TRS, font, monitor and palette distinctions above.

## Studio and browser audio features

| ID | Feature and behavior | Proposed action |
| --- | --- | --- |
| 11 | **Device display controls.** Studio changes supported device palettes, font weight and MIDI-monitor mode; controls follow advertised firmware capabilities. | Palette/preferences framework already adopted. Pair remaining font/monitor support with #6 and #8. |
| 12 | **Synchronized device favorites.** Stars/filter reflect device-side changes and used/deleted user slots. | Adopted. Do not offer a duplicate implementation. |
| 13 | **Playable on-screen keyboard.** Mouse, multitouch and computer-key playing; chords, octave, MIDI channel, velocity, sustain and Stop All Notes. Works with a connected FM-1 without a new firmware feature. | Strong independent proposal, initially using upstream's existing visual style. |
| 14 | **Keyboard note cleanup.** Releases notes on key/pointer release, pointer cancellation, focus loss, page hiding, channel/octave changes and disconnect. Typing in text inputs does not trigger notes. | Ship with #13 and its regression tests. Physical-disconnect cleanup is best effort. |
| 15 | **Docked responsive keyboard.** Collapsible tray, the FM-1's F3-G5 range, and additional shaded keys when the viewport permits. | Optional presentation layer for #13. |
| 16 | **Felucca Synth in the browser.** Adapts Felucca's C DSP to WebAssembly and an AudioWorklet. Plays without hardware. Published version has nine engines and 54 presets; RC1 supports thirteen engines and 69 presets, plus embedded FM6 patches in imported sound files. | Major standalone proposal. Our contribution is the browser adapter/runtime and integration; credit the upstream engine and sample authors. |
| 17 | **FM-1 and Browser mode switch.** Starts browser audio through user interaction, stops browser playback when returning to hardware, and hides hardware workflows unavailable in browser mode. | Include with #16. |
| 18 | **Browser sound editing and file workflow.** Sound controls, built-in samples, sound-file loading/export, chords/sustain and separate browser volume. Audio output adapts to the browser's sample rate. | Include with #16. |
| 19 | **Live oscilloscope.** Displays the browser engine's actual audio output. | Optional component of #16. |
| 20 | **Customizable control cards.** Knobs/sliders globally or per Sound card, fine knob adjustments, card reordering and saved layout preferences. | Separate usability proposal, independent of Salt branding and themes. |
| 21 | **Responsive and full-width Studio.** Saved width preference, responsive panels, appropriate side/top margins and keyboard-aware layout. | Offer selected improvements after comparing with upstream's preferred layout. |
| 22 | **Twelve website themes.** Stage Red, Matrix, Vintage DX7, Model D Walnut, Chocolate Factory, Vaporwave, Midnight Studio, Space Mission, Bauhaus, Ocean Lab, Arcade '84 and High Contrast. Includes light/dark modes, hardware-inspired materials and locally hosted licensed fonts. | Discuss an optional theme layer first; a replacement design is a larger product decision. |

Browser playback is a **one-sound preview**. Device sequencing, track mixing,
projects, sample uploads and the full FM6 patch/bank editor are not implemented
as browser playback workflows. User-sample slots referenced by files are not
automatically reproduced. Sound edits must be exported to persist them as files.
See [browser audio details](../web/audio/README.md) for the version represented
by the checked-out branch; the 1.0 candidate description is in
[fork PR #26](https://github.com/ChanceTheMaker/Felucca/pull/26).

Implementation pointers: `web/editor.html`, `web/skin.js`, `web/interface.css`,
`web/audio/`, `tools/build_browser_audio.py`, `web/test_keyboard.mjs`,
`web/test_audio.mjs` and `web/test_browser_audio.mjs`.

## Localization and accessibility features

| ID | Feature and behavior | Proposed action |
| --- | --- | --- |
| 23 | **Shared localization framework.** Installer/Studio catalogs, dynamic text and translated attributes, interpolation, browser-language detection, saved choice and cross-page/tab consistency. | Strong contribution. Framework and translation batches can be separate PRs. |
| 24 | **Six added languages.** Spanish, French, German, Russian, Simplified Chinese and Brazilian Portuguese, alongside English and Japanese. | Offer after native-speaker review where possible; automated coverage checks do not establish translation quality. |
| 25 | **Language picker.** Current-language flag, click-to-open selection, close after selection/Escape/outside click, persistent preference. | Useful interaction; let upstream choose flag or language-name presentation. |
| 26 | **Accessibility and reduced motion.** High-contrast choices, keyboard focus, translated accessible labels, and motion preferences for scrolling/animations. | Include the relevant pieces with each feature rather than one unrelated omnibus patch. |

Evidence: [localization documentation](../web/LOCALIZATION.md), `web/i18n.js`,
`web/locales/`, `web/test_locales.mjs` and the theme/navigation CSS and scripts.
Technical names, sound names, raw diagnostics and musical notation retain their
original meanings rather than being translated indiscriminately.

## Installer and website features

| ID | Feature and behavior | Proposed action |
| --- | --- | --- |
| 27 | **Install Progress accordion.** Initially collapsed; Install opens it and smoothly scrolls to output. Bounds long logs and follows new entries when already at the bottom. | Good small installer PR. |
| 28 | **Post-install Studio action.** Edit in Studio appears after a successful installer result, and hides during a new attempt or after failure/stopped recovery. | Pair with #27. Preserve the distinction between a completed resume write and a verified reboot; the stable resume path does not verify reboot identity. |
| 29 | **Shared navigation.** Install, Manual, Studio, Themes and Features; plain clickable homepage logo; smooth section scrolling; installer Back to Top. | Offer a focused usability package if wanted. |
| 30 | **Interactive illustrated controls guide.** Compact FM-1 vector illustration, red indicator lines, selectable groups and localized explanations of the 1.0 controls. | Documentation proposal; upstream should review wording and presentation. |
| 31 | **Animated hero illustration.** Wireframe transitions through product colors, colorful first, with reduced-motion support. | Optional website asset; discuss first. |
| 32 | **Theme gallery and direct launch.** Thumbnails for all themes; opens Studio in the selected theme; in-place hover enlargement with Click to Play/Edit. | Depends on accepting #22. |
| 33 | **Studio previews and launch controls.** Theme-matched screenshots, 320px hero preview kept clear of both buttons, full-width launch CTA, clearer hardware/browser wording and USB icon. | Presentation choices to offer selectively. |
| 34 | **Readability and layout polish.** Larger description text, relevant feature icons, compact header, spacing corrections, responsive gallery and removal of redundant links. | Keep Salt-specific presentation local; offer reusable fixes individually. |

Evidence: website fork PRs #14, #15, #19-22 and the latest independent PRs
[#30](https://github.com/ChanceTheMaker/Felucca/pull/30),
[#31](https://github.com/ChanceTheMaker/Felucca/pull/31),
[#32](https://github.com/ChanceTheMaker/Felucca/pull/32),
[#33](https://github.com/ChanceTheMaker/Felucca/pull/33).
The four latest PRs are merged and their website output was published in `f370b0c`.

## Analytics and development support

| ID | Feature and behavior | Proposed action |
| --- | --- | --- |
| 35 | **Analytics and preferences.** GA4 visits, theme selection, browser-audio starts, download clicks and installation events. Bottom notice, settings and remembered opt-out. Local previews and other forks are excluded. | Keep Salt-specific. Never transfer our measurement ID or assume upstream wants our analytics/cookie defaults. |
| 36 | **Regression coverage.** Tests for ring undercount, display migration, monitor, keyboard cleanup, browser audio, localization, preferences and installer flow. | Ship focused tests with their features. Much of the wider firmware suite originated upstream. |
| 37 | **Browser build tooling.** Reproducible WASM build adapter and website packaging for browser assets. | Include with #16. |
| 38 | **Release and contribution documentation.** Upstream boundaries, compatibility, hardware-validation checklist, source archives, checksums and feature PR discipline. | Keep fork policy local. Offer generally useful instructions separately. |
| 39 | **Consolidated local previews.** Stable and RC1 on port 8768 at separate paths, with local caching disabled. | Local convenience, low upstream priority. The helper currently lives in ignored `build/` tooling, not the tracked product. |
| 40 | **Experimental Bluetooth work.** BLE MIDI transport/output investigations, board initialization and probes. | Excluded from published Salt and RC1. Not ready for an upstream proposal. |

The analytics implementation is documented in [ANALYTICS.md](../web/ANALYTICS.md).
The local preview entry points are:

- Stable: `http://localhost:8768/webapp/installer/`
- RC1: `http://localhost:8768/rc1/webapp/installer/`
- Replace `installer` with `editor` for each Studio. Port 8871 is retired locally.

## Attribution boundaries

The following are upstream or other contributors' features we integrated, not
new Salt inventions to offer back: FM6 and the other 1.0 engines, songs, motion
sequencing, modulation matrix, quick layers, USB audio, sample recording/trim,
full backup/restore and official-V15 recovery. White-key scales also came from
another contributor. The [upstream README and credits](https://github.com/hugelton/Felucca/blob/20c275e39f75fa820978032efaceddfc5283c8cb/README.md)
are the attribution reference.

Protocol alignment is integration work: preserve SONG at 33 and preferences/
favorites at 34-38. Do not revive compatibility for the unshipped experimental
Salt command-33 layout. A browser adaptation of upstream DSP does not transfer
authorship of the DSP to Salt.

## Proposed contribution order

1. **#4 TRS receive-ring fix**, with a current-upstream reproduction and hardware check.
2. **#8 MIDI monitor**, including its controls, persistence and focused tests.
3. **#6 bold text**, independently reviewable with its settings and editor behavior.
4. **#13-14 on-screen keyboard and cleanup**, with upstream's existing appearance first.
5. **#27-28 installer progress and Studio handoff**, preserving accurate success semantics.
6. **#23-24 localization framework and language batches**.
7. **#16-19 and #37 browser playback**, after agreeing its scope with upstream.

Additional palettes and control-card customization are reasonable next proposals.
Themes and website design need a product discussion. This order is a recommendation,
not an agreed submission queue. Record the owner's selections and actual PR state
in [UPSTREAM_STATUS.md](UPSTREAM_STATUS.md).

Before any proposal, check current upstream and open issues/PRs again, start from
a clean upstream base, and include only the selected feature. Follow
[UPSTREAM_WORKFLOW.md](UPSTREAM_WORKFLOW.md). The new LED/menu work in 1.0.1
overlaps Salt's display extension files; see the [1.0.1 review](UPSTREAM_1.0.1_REVIEW.md).

This inventory is based on source/history inspection and validation records.
The accompanying 1.0.1 candidate update rebuilds and tests the combined software;
see [candidate notes](SALT-1.0-CANDIDATE.md). Hardware validation remains pending.
