# Developer Guide

OpenTMate2Lib separates protocol parsing from USB transport.

That split is important: the report format is stable across operating systems, while device access differs between Windows HID, Linux `hidraw`, `hidapi`, and `libusb`.

## Architecture

```text
Application
  |
  | uses controls, events, SDR mapping
  v
OpenTMate2Lib protocol core
  |
  | parses IN reports and builds OUT reports
  v
Transport layer
  |
  | reads/writes 64-byte reports
  v
TMate 2 USB device
```

The current library is the protocol core.

It provides:

- Constants for VID/PID, endpoints, report sizes, and key masks.
- `opentmate2_parse_input_report` to decode input reports.
- `opentmate2_build_output_report` to build display reports.
- `opentmate2_encoder_delta` to handle 16-bit wrap-around.
- `opentmate2_key_is_pressed` for active-low keys.

## Transport Requirements

A transport layer must do two things:

- Read interrupt IN reports from endpoint `0x81`.
- Write interrupt OUT reports to endpoint `0x01`.

Both directions use 64-byte payloads in the captures.

### Linux Options

Recommended practical order:

| Option | Pros | Notes |
| --- | --- | --- |
| `hidraw` | Direct, no libusb driver detaching | Best if the kernel exposes the device as HID |
| `hidapi` | Portable API | Report ID handling can differ by backend |
| `libusb` | Full endpoint control | May require detaching kernel HID driver |

### hidapi Caution

USBPcap showed 64-byte interrupt endpoint payloads. Some hidapi backends expect a leading report ID byte for writes, making the application buffer 65 bytes even when the wire payload is 64 bytes. Verify this against the target platform.

If `hid_write` fails or shifts the display vector by one byte, try a 65-byte buffer where byte `0` is the report ID and bytes `1..64` hold the OpenTMate2Lib report.

## Event Loop

A typical application loop:

1. Open the TMate 2 device by VID/PID.
2. Initialize an output `LCDVector` and send a 64-byte report.
3. Repeatedly read 64-byte input reports.
4. Parse each report with `opentmate2_parse_input_report`.
5. Compute encoder deltas against the previous report.
6. Detect key transitions with `opentmate2_key_is_pressed`.
7. Map controls to SDR actions.
8. Send display updates as needed.

## Encoder State

Store previous encoder values per encoder:

```c
int32_t d1 = opentmate2_encoder_delta(input.enc1, previous.enc1);
int32_t d2 = opentmate2_encoder_delta(input.enc2, previous.enc2);
int32_t d3 = opentmate2_encoder_delta(input.enc3, previous.enc3);
```

The C API returns `int32_t` for deltas so the subtraction is safe even on platforms where plain `int` is only 16 bits.

For the first report, initialize previous values and treat all deltas as zero.

## Key State

Keys are active-low, so pressed means `(keys & mask) == 0`.

For edge detection:

```c
int was_down = opentmate2_key_is_pressed(previous.keys, OPENTMATE2_KEY_F1);
int is_down = opentmate2_key_is_pressed(current.keys, OPENTMATE2_KEY_F1);

if (!was_down && is_down) {
    /* F1 pressed */
}
```

## Display Output

The library currently treats display bytes as a raw 44-byte vector because segment-to-byte packing still comes from the original Delphi definitions. A higher-level display builder can be added later once all segment mapping helpers are ported.

Current output contract:

- The caller supplies exactly 44 bytes.
- OpenTMate2Lib appends zero padding to create a 64-byte USB report.
- Transport writes that report to endpoint `0x01`.

## Naming

The project name `OpenTMate2Lib` is intentionally explicit:

- `Open`: no dependency on the original closed Windows DLL.
- `TMate2`: matches the device name people will search for.
- `Lib`: usable from multiple SDR applications instead of being tied to one host.
