# Website analytics

The Salt installer and Studio use GA4 measurement ID `G-JVF09MZEGD`.
Only `chancethemaker.github.io/Felucca/` sends analytics. Local previews,
other forks, and upstream sites do not. The Google script loads automatically
with analytics storage denied, sending cookieless measurements. The preferences
modal, opened from the hamburger menu, offers Without cookies, Allow analytics cookies, and Turn analytics off.
Existing declines remain full opt-outs. All choices leave every feature available.
Preferences persist in local storage and can be changed at any time.

Cookieless pings support Google's aggregate measurement and modeling; they are
not equivalent to identified visitors or sessions in standard reports. Modeling
depends on Google's eligibility thresholds and is not guaranteed for a small site.
Cookieless does not mean no data is sent to Google or establish legal compliance.

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
from Google's automatic download events. Verify consented visits in Realtime;
verify cookieless pings in browser network tools. Local unit tests never contact Google.

Run `node web/test_analytics.mjs` for consent, environment, and filtering checks.
