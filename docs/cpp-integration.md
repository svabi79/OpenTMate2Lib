# C++ Integration

The core library exposes a C ABI so that C++ applications can consume it without ABI surprises.

## Minimal Decode Example

```cpp
#include "opentmate2/opentmate2.h"

opentmate2_input_t input {};
if (opentmate2_parse_input_report(report, report_len, &input) == OPENTMATE2_OK) {
    int32_t delta = opentmate2_encoder_delta(input.enc1, previous.enc1);
}
```

## Build With RAD Studio / C++Builder

RAD Studio 11 / compiler version 35.0 provided these compilers on the development machine:

| Compiler | Target |
| --- | --- |
| `bcc64` | Win64 C/C++ |
| `bcc32c` | Win32 modern Clang-enhanced C/C++ |
| `bcc32` | Win32 classic C++Builder compiler |

Verified commands:

```powershell
cd D:\Code\OpenTMate2Lib
bcc64 -Iinclude -o examples\cpp\tmate2_decode_demo_bcc64.exe src\opentmate2.c examples\cpp\tmate2_decode_demo.cpp
bcc32c -Iinclude -o examples\cpp\tmate2_decode_demo_bcc32c.exe src\opentmate2.c examples\cpp\tmate2_decode_demo.cpp
```

## Aether Notes

I could not verify from the local source trees which exact Aether project is meant. Many SDR applications are C or C++ based, so the safest integration path is:

- Keep OpenTMate2Lib as a C ABI.
- Add a small C++ adapter class in the host application.
- Let the host application own the USB/HID event loop.
- Convert TMate 2 events into the host application's existing tuning, mode, volume, and button commands.

## Suggested C++ Adapter

```cpp
class TMate2Controller {
public:
    void onReport(const uint8_t* data, size_t size)
    {
        opentmate2_input_t current {};
        if (opentmate2_parse_input_report(data, size, &current) != OPENTMATE2_OK) {
            return;
        }

        if (hasPrevious_) {
            const int32_t mainDelta = opentmate2_encoder_delta(current.enc3, previous_.enc3);
            if (mainDelta != 0) {
                // map to SDR tuning
            }
        }

        previous_ = current;
        hasPrevious_ = true;
    }

private:
    opentmate2_input_t previous_ {};
    bool hasPrevious_ = false;
};
```

## hidapi Skeleton

See `examples/cpp/tmate2_hidapi_example.cpp`.

That file is intentionally not part of the default build because hidapi packaging and report-ID handling vary across systems.

## Integration Checklist

- Confirm the device opens by VID/PID `0x1721:0x0614`.
- Confirm input reads produce 64-byte reports beginning with `0x01`.
- Confirm idle keys are `0x01FF`.
- Confirm each encoder delta direction matches the host application's expected direction.
- Confirm output writes do not shift the LCD vector by one byte.
- Add user-configurable mappings for F1..F6 and encoder buttons.
