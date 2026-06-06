# Delphi Usage

The Delphi binding is in `bindings/delphi/OpenTMate2.pas`.

It is a pure protocol unit. It does not load `TMATE2_DLL.dll` and it does not open USB devices, so it is safe to use from existing Delphi applications.

For Windows, a native USB HID transport that replaces `TMATE2_DLL.dll`'s open/read/write surface (SetupAPI + `hid.dll`, no third-party hidapi) is provided in `bindings/delphi/OpenTMate2HID.pas` — see the [HID Transport](#hid-transport-windows) section below. On other platforms, feed the protocol unit 64-byte reports from your own transport (`hidraw`, `hidapi`, …).

## Add The Unit

Add the binding directory (`bindings\delphi`, relative to the repository root)
to the project search path.

Then use:

```pascal
uses
  OpenTMate2;
```

## Parse Input

```pascal
var
  Report: TBytes;
  Input: TOpenTMate2Input;
begin
  if OpenTMate2ParseInputReport(Report, Input) then
  begin
    Writeln(Input.Enc1);
    Writeln(Input.Enc2);
    Writeln(Input.Enc3);
  end;
end;
```

## Encoder Delta

```pascal
Delta := OpenTMate2EncoderDelta(Current.Enc1, Previous.Enc1);
```

The function handles wrap-around, for example `65535 -> 0` and `0 -> 65535`.

## Key Detection

```pascal
if OpenTMate2KeyIsPressed(Input.Keys, OPENTMATE2_KEY_F1) then
  Writeln('F1 pressed');
```

Keys are active-low.

## Error Contract

The Delphi binding uses a Boolean return contract for protocol helpers that validate caller-supplied byte arrays.

- `False` means the input length was invalid.
- No exception is raised for invalid report or vector lengths.

## Drive The Display

The binding ports the full LCD display layer, so you build frames with named
helpers instead of poking raw bytes. All display helpers take a `var TBytes`
LCDVector and return `False` if it is shorter than `OPENTMATE2_LCD_VECTOR_SIZE`.

```pascal
var
  LCDVector: TBytes;
  Report: TBytes;
begin
  OpenTMate2LcdInit(LCDVector);                 // alloc 44 bytes + timing defaults
  OpenTMate2SetBacklight(LCDVector, $20, $80, $20);
  OpenTMate2SetContrast(LCDVector, $28);
  OpenTMate2SetStatus(LCDVector, OPENTMATE2_LED_USB);
  OpenTMate2ToggleClick(LCDVector);             // request click on next write

  OpenTMate2WriteMainDisplay(LCDVector, 14200000);   // 9-digit frequency (Hz)
  OpenTMate2WriteSmallDisplay(LCDVector, 59);        // 3-digit S-meter / power

  OpenTMate2SetSegment(LCDVector, OPENTMATE2_SEG_USB, True);   // mode indicator
  OpenTMate2SetSegment(LCDVector, OPENTMATE2_SEG_RX, True);
  OpenTMate2SetSegment(LCDVector, OPENTMATE2_SMETER_BAR9, True);

  if OpenTMate2BuildOutputReport(LCDVector, Report) then
    ; // write the resulting 64-byte Report through the USB transport
end;
```

`OpenTMate2WriteMainDisplay` / `OpenTMate2WriteSmallDisplay` touch only the
seven segment bits of each digit, so indicator segments sharing those bytes
(RIT, underlines, mode flags) survive a redraw. Call
`OpenTMate2LcdClearDisplay` to blank the 32 segment bytes while keeping
backlight, contrast, and timing. Segment IDs are the `OPENTMATE2_SEG_*` and
`OPENTMATE2_SMETER_BAR*` constants (0..175).

## HID Transport (Windows)

`bindings/delphi/OpenTMate2HID.pas` is an optional, Windows-only USB HID
transport built on SetupAPI + `hid.dll` — no third-party hidapi, no
`TMATE2_DLL.dll`. It pairs the protocol unit above with the actual device:
discovery by VID/PID, open/close, overlapped read with timeout, and write.

```pascal
uses
  OpenTMate2, OpenTMate2HID;

var
  Dev: TOpenTMate2HID;
  Inp: TOpenTMate2Input;
  Lcd: TBytes;
begin
  Dev := TOpenTMate2HID.Create;
  try
    if not Dev.Open then
      Exit;  // no TMate 2 connected

    // Build and push a display frame.
    OpenTMate2LcdInit(Lcd);
    OpenTMate2SetBacklight(Lcd, 0, 50, 255);
    OpenTMate2ToggleClick(Lcd);
    OpenTMate2WriteMainDisplay(Lcd, 14200000);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_USB, True);
    Dev.WriteLcd(Lcd);

    // Poll input (returns False on timeout).
    if Dev.ReadInput(Inp, 50) then
      ; // Inp.Enc1..Enc3, Inp.Keys
  finally
    Dev.Free;  // closes the device
  end;
end;
```

The Windows HID stack frames reports with a leading report-ID byte; the unit
prepends it on write and drops it on read, so callers work in the device's own
report bytes. See `examples/delphi/TMate2HidSmoke.dpr` for a runnable end-to-end
smoke test (it degrades gracefully when no device is present).

## Console Example

Build:

```powershell
cd examples\delphi
dcc32 TMate2ConsoleDemo.dpr
```

Run:

```powershell
.\TMate2ConsoleDemo.exe
```
