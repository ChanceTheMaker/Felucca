# Browser sound preview

Select **Browser** in the **FM-1 / Browser** switch to start audio automatically.
Both modes remain visible; switching back to FM-1 stops browser playback. If opening
`?audio=1#sound` directly, click **Browser** or **Enable audio** to unlock audio playback.
The live oscilloscope shows browser output above the Studio tabs.
This runs the existing GPL-3.0 Felucca C DSP in an AudioWorklet through WebAssembly.
It does not open Web MIDI, send firmware, or require an FM-1.

Supported: all thirteen sound engines, their 69 factory presets, Sound controls, built-in
samples, patch file loading/saving, the on-screen/computer keyboard, sustain, chords,
and a separate browser volume. Four tracks, mixer controls, step patterns and
motion events are synchronized to the browser DSP. Play from start and Stop use
the audio engine's sample clock, and the transport reports actual step positions.
The keyboard plays the selected track. Mute retains firmware semantics (blocks
new notes; existing releases/effects can finish). Browser settings expose tempo,
swing and tuning; external clock and physical MIDI routing are omitted.
Library patches use the existing IndexedDB library. Current tracks, four project
slots, FM6 banks and favorites persist locally in the browser workspace. The
save-status label reports saving or failure. Projects backup export/restore uses
a validated browser-specific format, distinct from FM-1 device backups. Workspace
files currently exclude sample data. Song chains play using the audio clock.
Patch files referencing user sample slots cannot reproduce those samples here.
FM6 factory patches and imported patches embedded in sound files are carried into
the audio engine. The 6-OP FM editor supports live editing, import/export and a
persistent browser patch bank. Sending a patch selects the FM6 engine automatically.
Export patches or a workspace backup to keep a separate copy.
The retired DIGITAL engine ID is hidden; imported DIGITAL sounds use the editor's
upstream DIGITAL-to-FM6 conversion.
Browser storage is local to this browser/site and can be cleared; exported files
provide independent copies. Playback never starts automatically after a reload.

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
