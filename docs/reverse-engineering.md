# Reverse Engineering Notes

These notes document how the current protocol facts were discovered. They are included so future developers can reproduce and extend the mapping.

## Source Windows Application

The starting point was the existing Delphi Windows reference application
(Flex-TMate2). Important files in it:

| File | Purpose |
| --- | --- |
| `TMate2DLL.pas` | Delphi declarations for `TMATE2_DLL.dll` |
| `main.pas` | Application logic calling `TMate2ReadValue`, `TMate2SetLcd`, and related functions |
| `LCD_SEGMENT_DEF.pas` | Segment constants and key masks |

The DLL is a 32-bit Windows DLL. OpenTMate2Lib does not use it; it was only used as a reference while correlating semantic calls with USB reports.

## Capture Tooling

A helper tool (`TMate2Probe`) drove the reference DLL while USB traffic was
captured in parallel. The mapping in this library was derived from two paired
capture sets, each a USBPcap/TShark recording alongside the matching semantic
DLL session log:

| Capture session | Semantic DLL session |
| --- | --- |
| `pcap_orchestrated_20260605_050759` | `session_20260605_050801` |
| `pcap_orchestrated_20260605_051709` | `session_20260605_051711` |

The session timestamps are referenced in code comments (e.g. the segment map
in `src/opentmate2.c`) so each mapped fact is traceable to the capture it came
from.

## Method

1. Call the Windows DLL from a controlled Delphi console program.
2. Log semantic DLL inputs and outputs.
3. Capture USB traffic with USBPcap/TShark at the same time.
4. Extract interrupt IN/OUT reports.
5. Correlate report bytes with DLL-level values.
6. Exercise each encoder and key separately.
7. Confirm active-low key polarity and encoder counter wrap.

## Important Capture Detail

USBPcap devices did not show correctly until after a reboot following installation. After reboot, interface `\\.\USBPcap1` produced valid captures for this TMate 2.

## Remaining Work

- Fully map input bytes `9..63`.

Transports implemented and validated against real hardware:

- **Cross-platform (Linux / macOS / Windows)** — `src/opentmate2_hid.c`, an
  optional hidapi-based C transport (`-DOPENTMATE2_WITH_HIDAPI=ON`).
- **Windows, DLL-free Delphi** — `bindings/delphi/OpenTMate2HID.pas`
  (SetupAPI + `hid.dll`), a native drop-in replacement for `TMATE2_DLL.dll`.

The Delphi binding also carries the full LCD display layer (segment map +
digit encoders), so it reaches parity with the C core.

