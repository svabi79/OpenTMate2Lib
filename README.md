# OpenTMate2Lib

OpenTMate2Lib is an open, transport-independent protocol library for the ELAD / WoodBoxRadio TMate 2 SDR hardware controller.

The first goal is deliberately modest and useful: decode TMate 2 input reports, build display output reports, and document the USB protocol well enough that SDR applications can integrate the controller without the original Windows DLL.

## Project Status

Reverse-engineering status as of 2026-06-05:

- Device VID/PID is known: `0x1721:0x0614`.
- Interrupt IN endpoint is known from USBPcap captures: `0x81`.
- Interrupt OUT endpoint is known from USBPcap captures: `0x01`.
- IN and OUT report size is `64` bytes.
- Input report bytes `0..8` are mapped with high confidence.
- Output report bytes `0..43` are mapped with high confidence.
- Button masks, active-low key polarity, and encoder wrap behavior are confirmed.
- Remaining input report bytes `9..63` are not fully mapped yet.

## Repository Layout

- `include/opentmate2/opentmate2.h`: public C ABI for the core protocol.
- `src/opentmate2.c`: pure C implementation, no OS or USB dependency.
- `bindings/delphi/OpenTMate2.pas`: Delphi protocol binding with the same constants and helpers.
- `examples/cpp`: C++ examples, including a decode demo and a hidapi integration skeleton.
- `examples/delphi`: Delphi console example.
- `docs`: protocol and developer documentation.

## Quick Start

The protocol core does not open USB devices itself. It expects callers to provide 64-byte reports from a HID/USB transport such as Linux `hidraw`, `hidapi`, `libusb`, or a platform-specific HID API.

```c
#include "opentmate2/opentmate2.h"

uint8_t report[OPENTMATE2_REPORT_SIZE] = {0};
opentmate2_input_t input;

if (opentmate2_parse_input_report(report, sizeof(report), &input) == OPENTMATE2_OK) {
    int pressed = opentmate2_key_is_pressed(input.keys, OPENTMATE2_KEY_F1);
}
```

## Build

### RAD Studio / C++Builder

RAD Studio installs Embarcadero C++ compilers next to the Delphi compiler. If the environment is initialized, the demo can be built directly:

```powershell
bcc64 -Iinclude -o examples\cpp\tmate2_decode_demo_bcc64.exe src\opentmate2.c examples\cpp\tmate2_decode_demo.cpp
```

For 32-bit:

```powershell
bcc32c -Iinclude -o examples\cpp\tmate2_decode_demo_bcc32c.exe src\opentmate2.c examples\cpp\tmate2_decode_demo.cpp
```

Delphi example:

```powershell
cd examples\delphi
dcc32 TMate2ConsoleDemo.dpr
```

### CMake

```powershell
cmake -S . -B build
cmake --build build
```

This repository is designed to compile as C99/C++ friendly code. During creation it was verified with Embarcadero `bcc64`, `bcc32c`, and Delphi `dcc32`. CMake still needs a CMake-capable generator/toolchain installed.

## Documentation

- `docs/protocol.md`: USB report format and decoded fields.
- `docs/developer-guide.md`: architecture, transports, and integration strategy.
- `docs/delphi.md`: Delphi usage.
- `docs/cpp-integration.md`: C++ and SDR application integration notes.
- `docs/reverse-engineering.md`: capture method and confidence levels.

## Legal / Interoperability

OpenTMate2Lib is an independent, clean-room interoperability implementation of
the TMate 2 USB protocol, produced by observing the device's own USB traffic.
It contains no code from the vendor's driver or application and does not link
against or redistribute any proprietary DLL — the reference application was used
only to correlate observed reports with their meaning (see
[`docs/reverse-engineering.md`](docs/reverse-engineering.md)). "ELAD",
"WoodBoxRadio", and "TMate 2" are trademarks of their respective owners; this
project is not affiliated with or endorsed by them.

## License

Released under the [MIT License](LICENSE) — © 2026 Jan Svabenik.
