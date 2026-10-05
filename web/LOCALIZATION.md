# Website localization

Installer and Studio share `i18n.js` and eight catalogs in `locales/`: English,
Japanese, Spanish, French, German, Russian, Simplified Chinese, and Brazilian
Portuguese. Clicking the current language's flag opens the language picker;
selection, Escape, or clicking outside closes it. A saved
choice takes priority over the browser language and persists between pages.

Edit JSON catalogs, then run `python web/build_locales.py`. The site builder
also rebuilds the bundled `locales.js`; no translation network request is needed.
Keep English keys and `{placeholder}` names in every catalog. Run
`node web/test_locales.mjs` to check coverage and placeholders.

Static text uses `data-i18n="namespace.key"`. Attributes use
`data-i18n-aria-label`, `data-i18n-title`, `data-i18n-alt`, or
`data-i18n-placeholder`. Dynamic text uses `FeluccaI18n.text(node, key, values)`
so a language change updates it, or `FeluccaI18n.t(key, values)` during rendering.
The existing editor and installer `data-t` keys are namespaced in their catalogs.
Use `onChange` to redraw interactive text. Do not put translated text in MIDI
messages, preset files, storage identifiers, or analytics event names.

Brand names, factory/user preset names, firmware-supplied parameter identifiers,
musical notation, and raw diagnostic details retain their original values so
they correspond to the hardware and saved files. Translate the surrounding UI.
