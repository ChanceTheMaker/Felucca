# Browser Studio expansion

The owner authorized enabling the existing Studio workflows in browser mode on
2026-10-05. Deliver as focused fork PRs above the Studio UI stack, without a
firmware release or website publication.

## Implementation order

1. FM6: expose the six-operator editor, send/live-edit patches into browser DSP,
   preserve import/export and factory/bank workflows.
2. Tracks and sequencer: synchronize all four tracks with the WASM engine,
   including mixer controls, patterns, transport and sample-clock timing.
3. Library and projects: persistent local browser workspace, explicit export,
   restore and storage-failure feedback. Browser storage is not a backup.
4. Samples: retain the existing FM-1-compatible sample pipeline initially,
   connect uploads to browser DSP and local storage, and preserve recording's
   explicit microphone permission flow.
5. Settings: expose meaningful audio/global controls, omit physical-device-only
   options, and verify navigation, reconnect and hardware/browser isolation.

## Decisions to revisit with the owner

- Larger browser-only samples versus FM-1-compatible limits. Start with hardware
  limits; no larger format or changed firmware format has been approved.
- Whether project exports should embed samples or reference shared sample slots.
- Microphone recording and browser-output recording are distinct. Existing sample
  microphone capture can be reused; an output recorder is a separate decision.
- Local browser storage versus any cloud/account synchronization. Only local
  storage is in this implementation's scope.

Earlier browser documentation explicitly describes one-sound playback and
unsupported sequencing, projects and uploads. Do not mark a tab supported until
its controls affect actual DSP/state and the corresponding tests pass.
