# SMK physical MIDI evidence

Test19 leaves the physical UART disabled. FM-1's `fm1_uart.h` is an input driver
using PH8; SMK's scanner uses that signal, so enabling the FM-1 driver is wrong.
The locally pinned SMK hardware reference documents **TRS MIDI OUT, Type A**.
Do not present the SMK jack as a MIDI input without separate hardware evidence.

New static leads from the mapped Pro016 stock image:

- `0x02006160..0x0200616c`: installs UART1 interrupt vector 20 pointing at
  `0x0202ddc8`.
- `0x02006170..0x0200617e`: updates IOMAP register `0x51028`, clearing bit 7
  and bits 20..23, then setting bit 20.
- `0x02006182..0x0200619e`: GPIO helper calls for pin number 39. The matching
  SDK numbers this PC7. Helpers manipulate pull-up, pull-down, output/direction
  and digital-input registers. This is an association to investigate, not proof
  that PC7 is the TRS data output.
- `0x020061a2..0x0200620c`: UART1 base `0x12100`, baud computation with 31,250,
  then enables the UART. The interrupt handler reads UART RX/count flags; this
  needs tracing to distinguish internal MIDI/controller traffic from the jack.

The SDK's UART1 fixed TX/RX pairs include PH6/PH7 and PC3/PC4; PC7 is not one
of those fixed pairs. Decode the remap/output-channel setup and trace transmit
buffer writes before defining an SMK pin mapping. Copying the numerical GPIO
lead into a TX driver now would skip this distinction.

Local evidence: `build/research/startup/pro016-mapped.disasm.txt`,
`build/research/smk-docs/README.md` (pinned revision recorded in preparation
notes), SDK `include_lib/driver/cpu/wl82/asm/{gpio,uart}.h`. No vendor binary or
full disassembly is redistributed here.

Next validation: identify the UART TX setup/call path and board connection,
then an isolated output test through a Type-A TRS MIDI adapter with a receiving
interface. Verify note-on/off, sustain, pitch/mod, transport/clock and recovery
after a full output queue. USB MIDI remains functional while this is unresolved.

## Test20 transmit-path evidence

The stock routine at `0x0202daf4` writes buffer address `0x01c34700` to
`0x12114` and its byte count to `0x12118`. The matching SDK register layout
identifies those as UART1 TXADR/TXCNT. Its callers at `0x0202db9c` and
`0x0202dbe6` take data from the adjacent MIDI-message queue routine
`0x0202da0e`. The UART1 ISR at `0x0202de54..0x0202de6c` handles the TX pending
flag and posts a completion event. This establishes a stock transmit path,
in addition to the earlier RX evidence. It still does not independently prove
which board net reaches the TRS tip/ring or resolve the PC7 remap interpretation.

`firmware/hal/smk37_midi_tx.h` now supplies a bounded, host-tested output queue
for channel notes/controllers, pitch, program/pressure, song position and
clock/transport. It excludes editor/update SysEx. Overflow finishes the current
message and sends sustain-off, all-sound-off and all-notes-off on all channels
before accepting new notes. It does not enable a UART, configure any GPIO or
connect itself to the running instrument. The physical adapter remains pending
the mux/board verification; no guessed routing is in Test20.

## Test21 isolated output candidate

Test21 adds an opt-in UART driver reproducing the stock `0x2006170` mux and PC7
output setup. The GPIO helper at `0x200436e` writes OUT then clears DIR; its
PC7 call uses output level zero. This is not the FM-1's PH8 receive setup.
The driver selects PLL48M / 31,250 baud, uses TXADR/TXCNT and polls TX pending
bit 15, clearing it with bit 13. TX interrupt enable is omitted. Other mux/GPIO
bits are preserved. PC7 does not overlap the current SMK LCD CS (PC8), light
gate (PC10), audio gate (PC6), or analog input masks.

TRS remains OFF on startup; only the MIDI-page K7 toggle activates the driver.
The shared local-output events feed the bounded queue independently of USB
configuration. The existing USB timer services it at 2 kHz, below audio priority.
Register tests and the target build pass; actual pin wiring/polarity and reception
still require the Type-A adapter/receiver test. Reproducing the stock setup is
evidence for a test candidate, not proof that the board output works.

See [TEST21.md](TEST21.md) for routing limits, overflow/disable behavior and the
exact package. Subsequent owner testing on 2026-10-10 confirmed basic Test21
TRS output into an FM-1 with audible playback ("Totally works"). Chords, sustain
and pitch bend subsequently passed; mod wheel was inconclusive. Switching TRS OUT
OFF while holding a note with the cable connected left the FM-1 sounding: the
shutdown test FAILED. Root cause is unresolved, with repair deferred at the
owner's request. See the hardware follow-up in TEST21.md.
