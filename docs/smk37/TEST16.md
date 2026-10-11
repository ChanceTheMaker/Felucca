# SMK Test16 ? boot branding and installer presentation

Based on Test15 and the unchanged upstream 1.5.1 pin
`a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.

## Firmware

The one-time startup screen reads `Felucca SMK`, horizontally centered in the
240-pixel display and vertically centered using the actual medium font height.
The real build version uses the small font, centered 10 pixels above the bottom.
No deliberate boot delay, protocol, synthesis, storage or update-window changes.
The USB identity remains `SMK37 TEST16 USB` / `SMK-37_916` for existing tooling.

## Website

The local installer reuses the live FM-1 site's hero buttons, theme gallery and
Studio hover preview behavior. It retains the existing SMK updater and safety gates.
Its illustration is original SVG geometry, referenced against official manufacturer
Pro and Elite product photos, with additional Pro finish references from the
product lineup at InterCyprus. White, red, pink, charcoal, all-black Pro and
slate/orange Elite finishes shuffle automatically every five seconds with a 1.5-second
crossfade between two separate painted layers, without buttons. Each finish appears once per shuffled cycle; changes pause in hidden
tabs and for reduced motion. Color changes never select a firmware package or
access hardware.
A brief wireframe-to-product reveal respects reduced-motion preferences.
The headline and tagline match the live site. Installation instructions distinguish
normal USB updates from the first-time recovery-adapter bootstrap.
Twelve clickable themes and matching hover previews use 24 fresh SMK Studio
captures, one per theme and light/dark mode. Screenshots use Browser audio only.

References:
- https://www.m-vave.com/product?id=smk-37-pro
- https://www.m-vave.com/product?id=smk-37-elite
- Additional Pro lineup: https://www.intercyprus.com/products/m-vave-smk-37-pro-midi-controller-keyboard-16-rgb-pads-8-knobs-software-compatible-for-music-production
- Live FM-1 presentation: fork gh-pages commit `79019a6`.

## Validation

- Target build, protected ranges and call-free RAM flash checks passed.
- Combined SMK application host suite passed, including all drawing pages,
  palettes and fonts; synthesis, controls, storage, editor and migration checks.
- Web updater tests passed: integrity, wrong-model isolation, bootstrap,
  success, disconnect and port cleanup.
- All eight locale catalogs passed; shared translation keys are unchanged.
- Desktop 1440px and mobile 390px: no horizontal overflow; all six finishes,
  37-key/16-pad geometry, twelve galleries, preview image loading, Escape dismissal,
  and disabled/enabled installer acknowledgement gates checked without MIDI access.
- Actual C text renderer used for the offline startup preview.
- This exact firmware still needs its device boot/display/playback check.

## Private artifacts

`build/smk37-webupdate-test16/` (ignored, unit-specific; never publish).

Application SHA-256: `44f39ad8bbced57759df7b7d778aa3b43aab0708f65338fa8da931c8a1bdccad`

Package: `smk37-test16-private.smkupdate` (627712 bytes)

Package SHA-256: `76a18289a0ab2207a0c5b917c59f761ae117e204dc7763ce4609c1aa451eaf77`

Full image SHA-256: `5e7185bb4c2f2033bf8fbd14fe89b2463edbf5184552a096b9e5fadbbce0e3b3`

Served Test16 metadata and all 627712 package bytes match the local SHA-256.

Local preview: http://localhost:8770/
Test15 artifacts remain intact. No device flash, Git push or public deployment.

## Illustrated manual follow-up

Ported the live FM-1 panel-guide structure, shared styles and group-switching
behavior, adapting six control groups to the SMK implementation in smk37_ui.c
and smk37_synth.c. A static SMK drawing highlights the relevant controls.
The manual is before the theme gallery, with a header Manual link. Installation
instructions are immediately under the hero; first-time bootstrap details remain
available in a disclosure. No protocol or firmware changes in this follow-up.
The user supplied the marketing claim "The first custom firmware for SMK-37
Pro & Elite" and requested "revolutionary" launch wording; these are product copy,
not a finding of an independent historical survey of other firmware projects.

## Browser audio follow-up

Reproduced `short reply` with the previous 91-parameter WASM and current
111-parameter editor. Revisioned loader/WASM/worklet URLs now prevent reuse of
the old unversioned cache entry. A forced incompatible engine displays a clear
localized version-mismatch message. Real browser playback, scope, stop/restart,
engine switching and MIDI isolation passed; both cache regressions passed.
These website changes do not alter the Test16 package above.

## Next firmware: physical shortcuts (source prepared)

PATCH retains Home/Mix and preset browsing. PARA opens Projects without issuing
a storage request; knob 5 selects the slot and > explicitly saves. BT opens the
parameter editor and its steady light follows editing pages. PARA lights on
Projects. Bluetooth remains disabled; future radio blinking is deferred.
The real-synth button regression passes with AddressSanitizer, including no
implicit save on repeated PARA presses and saving to the selected slot.
The full suite wrapper could not regenerate fonts in the default WSL Python
(missing Pillow); the targeted test used existing generated headers.
These shortcuts are not in the staged Test16 binary. Installer help continues
to describe that binary until a new candidate is built. No flash or publication.
