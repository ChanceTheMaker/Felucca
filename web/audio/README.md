# Browser sound preview

Select **Browser** in the **FM-1 / Browser** switch to start audio automatically.
Both modes remain visible; switching back to FM-1 stops browser playback. If opening
`?audio=1#sound` directly, click **Browser** or **Enable audio** to unlock audio playback.
The live oscilloscope shows browser output in place of the redundant single Sound tab.
This runs the existing GPL-3.0 Felucca C DSP in an AudioWorklet through WebAssembly.
It does not open Web MIDI, send firmware, or require an FM-1.

Supported: all thirteen sound engines, their 69 factory presets, Sound controls, built-in
samples, patch file loading/saving, the on-screen/computer keyboard, sustain, chords,
and a separate browser volume. The preview plays one sound; the MIDI channel selector,
device storage, sample uploads, sequencer, and other hardware tabs are hidden.
Patch files referencing user sample slots cannot reproduce those samples here.
FM6 factory patches and imported patches embedded in sound files are carried into
the audio engine. The full FM6 patch editor, banks and sequencing remain device workflows.
The retired DIGITAL engine ID is hidden; imported DIGITAL sounds use the editor's
upstream DIGITAL-to-FM6 conversion.
Browser edits are temporary unless exported with the Sound page's save-file button.

DSP renders at its native 44,100 Hz in 32-frame blocks. The worklet linearly resamples
when the audio device uses a different rate. This is a preview, not a measured claim
of exact hardware sound or latency. Modern Chrome/Edge on localhost or HTTPS are the
tested targets; other browsers/devices still need listening and performance checks.

## Build

From the repository root, with Python and Zig 0.13.0:

```
python -m pip install --target build/wasm-tools ziglang==0.13.0
python tools/build_browser_audio.py
node web/test_audio.mjs
```

Use `--zig /path/to/zig` on macOS/Linux. Generated sample/table headers remain in
`build/browser-audio/gen`; `web/audio/engine.wasm` is the deployable runtime asset.
`web/make_site.py` copies this directory into the editor site. Rebuild the WASM when
changing firmware DSP or sample assets. The adapter excludes USB, flash and radio drivers.

Browser integration: `node web/test_browser_audio.mjs` with Playwright/Chrome and the
site served on port 8768 (`PLAYWRIGHT_MODULE` can select a local Playwright installation).
The test rejects any Web MIDI access and checks actual audio samples from the keyboard.

Original engine code: Leo Kuroshita (@kurogedelic), Hügelton Instruments, GPL-3.0-only.
Included sample sources and notices: `assets/samples-cc0` and the repository sample
documentation. The browser adapter and JavaScript are GPL-3.0-only as well.
