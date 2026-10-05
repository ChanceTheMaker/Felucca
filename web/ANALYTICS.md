# Website analytics

The Salt installer and Studio use GA4 measurement ID `G-JVF09MZEGD`.
Only `chancethemaker.github.io/Felucca/` sends analytics. Local previews,
other forks, and upstream sites do not. The Google script loads only after
the visitor allows analytics using the preferences section at the bottom
of the page. Declining leaves all features available. Preferences persist
in local storage and can be changed in that section at any time.

Advertising consent and Google signals remain disabled. Page URLs omit
query strings; referrers are reduced to their origin. Custom event fields
are restricted to firmware version, stage, fixed error code, and file type.
Do not add MIDI messages, device identifiers, preset names, or raw errors.

| Event | Meaning |
| --- | --- |
| `install_attempt` | Install clicked, before MIDI permission |
| `install_write_started` | First write progress callback, once per attempt |
| `install_success` | Normal install returned and confirmed expected device identity |
| `install_failed` | Permission, discovery, transfer, or verification failure |
| `install_resume_complete` | Recovery write finished; reboot identity is not verified by the resume path |
| `browser_audio_started` | Browser synth start resolved |
| `download_click` | Click on a ZIP or FWSC link; not proof of completed download |

Automatic package fetching is not a download click. A retry is a new attempt.
Counts are approximate: consent, blockers, lost connections, and closed tabs
can prevent reporting. No analytics call is awaited by an installation.

In GA4, mark `install_success` as a key event if desired. Register
`firmware_version`, `stage`, `error_code`, and `file_type` as event-scoped
custom dimensions to break down these events in reports. Review enhanced
measurement in the web stream; these explicit custom events are separate
from Google's automatic download events. Verify receipt in Realtime after
deployment and consent; local tests deliberately never contact Google.

Run `node web/test_analytics.mjs` for consent, environment, and filtering checks.
