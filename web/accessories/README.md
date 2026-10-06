# FM-1 accessories illustration

`fm1-case-knob-extenders.png` is a transparent 1448 × 1086 product illustration
created with the built-in imagegen tool on 2026-10-05 from the owner's photos.
It depicts the empty custom case and eight hollow knob extender rings, without
the synthesizer. The installer card links to the owner's listing:
https://www.ebay.com/itm/128108187687

Reference photos: IMG_3144.JPEG (case and eight rings), IMG_3143 - Copy.JPEG
(rings), IMG_3147.JPEG (installed rings). Originals remain in the owner's
`Desktop/FM-1/FM-1 Accessories 4Sale` folder; they are not shipped with the site.

Generation prompt:

The rotating card also includes red, blue, glow, white and silver variants
created with the built-in imagegen tool from the same illustration. Colors follow
the owner's listing screenshot (black, white, red, blue, glow, silver).
White and silver show a black FM-1 inside, using IMG_3148.JPEG as the fitted-device
reference; the other views show the empty case. Exact variant prompts are in
`color-prompts.json`. The first purple-device draft was discarded.
The white and silver illustrations were corrected against IMG_3145.JPEG so the
device top sits flush with the tray rim, with no deep recess or hidden keys.
Their final edit prompts are recorded in `fit-correction-prompts.json`.

`rotate.js` crossfades every four seconds while the card is in view. It loads
the next image before transitioning, preserves the current image on load failure,
and stops when offscreen, the tab is hidden, or reduced motion is requested.

Original illustration prompt:

> Use case: product-mockup. Asset type: tasteful product illustration for an FM-1 accessories website card. Use the supplied photographs as accurate product references: image 1 shows the empty custom protective tray case and all eight knob extender rings; image 2 is the detailed ring shape; image 3 shows how the rings fit the FM-1 knobs, for understanding only. Create one polished, clean industrial-design illustration of ONLY the empty black custom case and exactly EIGHT black hollow knob extender rings. Preserve the case's wide shallow rectangular proportions, rounded corners, raised corner/side walls, lowered center rim on front and rear, small inner retaining lip, and subtle 3D-printed layered texture. Preserve the rings as short hollow collars with round center holes and softly faceted outer sides, NOT solid topped conventional knobs. Composition: elevated front three-quarter view, empty case filling the upper two-thirds, eight rings in one evenly spaced row in front below it, all products fully within the frame. Style: refined technical product illustration with crisp silhouettes, controlled soft charcoal shading and fine pale edge highlights, enough surface detail to show construction without gritty photographic noise. Remain black/charcoal, no added accent colors. Real transparent background including the ring holes. No synth, no keyboard, no electronics, no props, no text, no labels, no arrows, no logo, no watermark. Clearly illustrated rather than raw photo, tasteful and highly legible at a small website size.
