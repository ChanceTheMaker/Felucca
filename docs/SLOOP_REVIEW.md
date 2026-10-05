# Sloop comparison and firmware switching

Sloop is a substantial Felucca fork focused on live groovebox performance. A
Salt/Sloop installer selector is the recommended first switching option, with
separate backups and the matching editor for each firmware. It would reflash
the FM-1; it would not switch instantly while music is playing.

Reviewed October 5, 2026: [Sloop 2.2][sloop], commit
`f2b44c219b8a4ac00bc06dca756cdae8a259dd1a`, compared with Salt RC1
`7b1164abd2322c279faf9bb57ae22283466b9f53`, incorporating Felucca v1.0.1
`20c275e39f75fa820978032efaceddfc5283c8cb`. Published Salt14 is a separate
compatibility target. Sloop's 2.2 number does not indicate that it includes
Felucca 1.0.1. Its public Git history does not establish an exact Felucca fork
point, so do not invent shared merge ancestry.

## What Sloop changes

Sloop credits Felucca for its engines, sequencer, editor and installer. Its
source implements musical and workflow changes beyond branding:

| Area | Sloop 2.2 | Salt RC1 |
| --- | --- | --- |
| Tracks | Three synth tracks and one dedicated drum track | Four flexible synth tracks |
| Engines | Nine by default, including four-operator FM; optional SLICE disabled | Includes FM6, PHYS, NOISE and SLICE; 13 selectable engines |
| Drum performance | 16 lanes, per-hit levels and ratchets, note repeat, live erase | Drum engines and step grid, with different data and control behavior |
| Recording | Free take derives loop length/tempo; live section-order recording | Song chains and motion recording in the integrated Felucca implementation |
| Performance controls | Lockable layers, live sections, DUST, DUCK and DJ filter | Felucca quick layers plus Salt display, monitor and preference features |
| Sounds and effects | Reworked drum synthesis/presets, level trims, revised stereo chorus/reverb | Candidate Felucca DSP and retained Salt integration |

The [Sloop overview][readme] advertises 68 sounds and 37 kit choices (32
synthesized plus five treatments of the sampled acoustic kit). Ratchets and
the stereo effect changes are present in `seq.c` and `fx.c`; the
[engine table][engines] confirms the different engine set. These are source
findings, not an audio-quality ranking or real-device reliability assessment.

## Switching options

| Option | User experience | Scope |
| --- | --- | --- |
| Installer selector | Choose firmware, back up, install, reconnect to its editor | Recommended first implementation after compatibility tests |
| Performance mode within Salt | Select a different workflow in one running firmware | Selective ports of Sloop features, preserving Salt protocols and storage |
| Dual boot | Select one of two installed applications at startup | Unproven flash budget and bootloader design; a separate research project |

Both projects use the FM-1 FWSC package family and Felucca loader marker. That
supports investigating an installer selector, but does not establish that
every release, interrupted update, downgrade or restore works in both
directions. A website choice cannot provide dual boot by itself.

## Data and editor compatibility

The [Sloop storage map][storage] uses the same project, settings, sample and
preset regions as Felucca. Its autosave occupies `0x9F000` and `0xFE000`, the
sectors Salt RC1 uses for the FM6 patch bank. Running and saving in Sloop can
overwrite data needed when returning to Salt, even if flashing preserves it.

Sloop writes FUN4 projects with level/ratchet step fields. Salt RC1 writes
FUN8 and interprets older FUN4 through Felucca's different format definition.
A shared magic value is not evidence that project files can be interchanged.
Preserve raw backups separately; any musical project conversion needs its own
tested, version-aware mapping. Do not silently import foreign projects or
reuse preset banks by assuming parameter counts or engine IDs match.

Both editor protocols start with the same `FL` SysEx header. In Sloop,
**command 33 is DRUM_STEP**; in Salt RC1 it is **SONG**. Sloop's INFO response
still starts with `FELUCCA` and has its own trailer interpretation. Detect the
firmware family and capabilities before issuing feature commands. A brand
substring or an unqualified protocol-version number is insufficient.
[Sloop protocol reference][protocol]

Sloop does not expose the same complete backup commands as RC1. Before calling
the selector seamless, establish a supported Sloop export/restore path or add
one as a distinct feature. A Salt backup alone cannot preserve work created
after switching to Sloop.

## Proposed separate feature PRs

1. Detect Salt, upstream Felucca and Sloop without sending commands with
   conflicting meanings; reject unsupported variants explicitly. Test reconnects
   and malformed/ambiguous INFO responses.
2. Define firmware-specific backup files and restore rules. Include all data
   needed for return trips, especially FM6 banks, projects, samples and settings.
3. Add an installer catalog of pinned packages with hashes, source links,
   firmware identity, supported hardware and the appropriate editor destination.
   Keep RC1 promotion separate from the choice of website UI.
4. Validate Salt14/Sloop and RC1/Sloop round trips on real FM-1 hardware,
   including edits/saves on each side, reconnects and interrupted installation.
5. Separately assess bounded feature ports: ratchets, free-take recording,
   performance layers, drum kits, or stereo effects. Preserve upstream command
   allocation, measure CPU/RAM/flash, and compare audio before choosing imports.

The user requested research and documentation. Switching implementation and
Sloop feature imports are queued, not implemented or published. No Sloop
firmware has been flashed or built in this review. No source has been imported
into Salt, no hardware switching has been validated, and no upstream PR or
message has been sent. This FM-1 review does not establish SMK compatibility.

## Credits and licensing

Sloop declares GPL-3.0-only code and retains Felucca attribution. Preserve author
credits and corresponding source when distributing derived code or packages.
Its [licensing file][license] distinguishes code from assets, including separate
font/sample terms and retained Felucca asset notices. Review each imported asset;
do not treat the repository's code license as blanket permission for all media.

[sloop]: https://github.com/isod89/sloop-fm1/tree/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a
[readme]: https://github.com/isod89/sloop-fm1/blob/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a/README.md
[engines]: https://github.com/isod89/sloop-fm1/blob/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a/firmware/src/engines.c
[storage]: https://github.com/isod89/sloop-fm1/blob/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a/firmware/src/storage.c
[protocol]: https://github.com/isod89/sloop-fm1/blob/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a/web/EDITOR_PROTOCOL.md
[license]: https://github.com/isod89/sloop-fm1/blob/f2b44c219b8a4ac00bc06dca756cdae8a259dd1a/LICENSING.md
