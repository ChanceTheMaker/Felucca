# Felucca Salt work queue

Deferred requests for a future round. Listing an item here does not add it to
the current implementation or publish.

## Firmware choices

- [ ] **Evaluate Salt and Sloop switching on FM-1.** The
  [Sloop review](SLOOP_REVIEW.md) records the pinned comparison, overlapping
  storage and command 33 conflict. Proposed separate features: firmware
  detection, independent backup/restore, installer choice and hardware round
  trips. Dual boot remains research; selective Sloop feature ports need their
  own scope. Documentation is complete; implementation is not authorized by
  this queue entry.

## Website

- [x] **Make the header logo link to the homepage.** Implemented locally after
  the owner brought it into scope on 2026-10-05. The “Felucca [Salt]” logo on both
  pages links to the installer homepage, with its size/styling preserved and
  visible keyboard focus. The owner authorized merging into the fork's main;
  live deployment is separate.
