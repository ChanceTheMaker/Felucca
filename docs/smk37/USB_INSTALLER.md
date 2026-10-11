# SMK-37 direct USB updater candidate

Status: Test12 normal-USB installation, restart and saved-project retention
confirmed by owner on the test unit. Test11 adapter bootstrap and same-version
USB flow previously passed. Source checkpoint for the owner's fork; no public-site deployment.

## Local front page

http://localhost:8770/ serves the themed SMK landing page and private installer.
Studio stays at /editor.html. /webapp/installer/ and /webapp/editor/ redirect to
those local pages. tools/serve_smk37.py binds only 127.0.0.1 and serves the private
package through two explicit routes; directory listings are disabled. Run:

    python tools/serve_smk37.py --port 8770 --candidate build/smk37-webupdate-test12/web-candidate.json

No firmware package, private stock dump, or unit calibration was added to web/.
The FM-1 updater and generated public site remain separate.

## Firmware and installation

Build with tools/build_smk37.py --enable-saves --enable-usb-update plus the two
validated stock backup paths and Pro016 package. Outputs are isolated under
build/smk37-webupdate; Test10 artifacts remain unchanged. Package via
smk37_update_package.py with the same evidence and the new manifest.

Test10 has no updater, so first install Test11 through the XIAO adapter using
the private smk37-test11-flash-kit. smk37_bootstrap.py defaults to dry run,
requires exact Test10/Test11 application identity, pinned unit data and adapter
caps, backs up the full current image, writes application sectors only and
verifies complete readback. The entire current tail, including saved projects,
is preserved. Its restore mode rolls back to Test10 while preserving saves;
it is not a stock restore or a repair tool for interrupted application images.

After bootstrap, normal USB supports the local browser installer. The browser
accepts only the pinned candidate SHA256, then checks header and sector CRCs.
Exact SMK model filters prevent upgrade commands being sent to an FM-1. The
SMK stops audio/scanner/ADC and interrupts, disconnects USB, copies its embedded
loader to RAM 0x1c0a800 and jumps there. The RAM loader re-enumerates as
Felucca Update, protocol identity ota-SMK-37_911. Normal identity SMK-37_911.

Loader checks JEDEC 856014, unit boot-head and trim CRCs, all package sectors
before any erase, then checks each sector again on the programming pass.
Only 0x4000..0x9d000 is writable. Bootloader, calibration and saves are excluded.
Per-sector and whole-application readback precede completion/reboot. A failed
transfer leaves the RAM loader running for explicit retry. It has NO persistent
update record: power loss during programming requires the recovery adapter.
The FM-1 staging/record cleanup routines are not linked into this loader.

This private package contains the owner's stock application-container data;
it is NOT a redistributable SMK release. A public package/layout strategy and
wider hardware update validation remain release gates.

## Validation completed

- Jieli app (373728 bytes) and loader (7912 bytes) compile and link.
- Loader code/data/BSS constrained to RAM; no FM-1 staging routine linked.
- Existing application RAM flash routines pass the call-free audit.
- Portable loader ASAN/UBSAN: valid/no-op update, incorrect model/bounds/unit/
  chip, corrupt last sector, changed second-pass data, transport/program
  failure and explicit retry; protected ranges remain unchanged.
- Browser parser and simulated transport: pinned SHA, CRC, wrong-model
  isolation, handoff, success, failure and MIDI handler cleanup.
- Five bootstrap tests: populated saves preserved, exact rollback, foreign
  protected bytes/unknown applications rejected, invalid sector plan rejected.
- Exact candidate bootstrap/rollback simulation with populated save sectors.
- Existing editor/keyboard/OTA-disabled SysEx regressions pass. Optional FM-1
  package tests skip because this worktree has no built FM-1 package.
- Desktop/mobile visual checks, Studio navigation, package fetch/parser and
  high-contrast button colors pass on the live local port. A transient local
  connection reset occurred during earlier browser checks; final run passed.

## Original hardware validation plan (completed below)

1. Adapter dry-run, then approved Test11 bootstrap; full readback and normal boot.
2. Confirm existing saved projects and sustain still work.
3. Close Studio and use normal USB installer. Same-version reinstall verifies
   handoff, transport, package preflight and readback; identical sectors skip.
4. A distinct second reviewed build is needed to exercise actual USB erase/
   program. Then confirm normal boot and saved project preservation.
5. Keep the adapter and fresh pre-update backup available throughout testing.

## Test12 hardware result

Test12 direct USB installation: owner explicitly confirms final Test12 installed status, normal device restart and saved project retention after instructions to install the distinct candidate. Four application sectors differed from Test11, so this exercises actual USB erase/program rather than a same-version no-op. This is owner-reported hardware evidence; no independent browser log or adapter readback captured in this turn. Basic private normal-USB installation is now validated on this unit. Public redistribution, stock first-install, power-loss recovery and wider-device validation remain separate gates. No public firmware or website publication.
Package SHA256: 99d7c3d75d39706abb0d32463defba0e5c46ff73041dd5282ed982a6fe60fcec.
The earlier numbered hardware-check list records the original validation plan;
steps1-4 have now passed through the Test11/Test12 owner checks. The power-loss
limitation and unit-specific packaging restriction still apply.
