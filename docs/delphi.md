# Delphi Usage

The Delphi binding is in `bindings/delphi/OpenTMate2.pas`.

It is a pure protocol unit. It does not load `TMATE2_DLL.dll` and it does not open USB devices. This makes it safe to use from existing Delphi applications while a native USB transport is added separately.

## Add The Unit

Add the binding path to the project search path:

```text
D:\Code\OpenTMate2Lib\bindings\delphi
```

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

## Build Output

```pascal
var
  LCDVector: TBytes;
  Report: TBytes;
begin
  SetLength(LCDVector, OPENTMATE2_LCD_VECTOR_SIZE);
  LCDVector[33] := $20; // red
  LCDVector[34] := $80; // green
  LCDVector[35] := $20; // blue
  LCDVector[36] := $28; // contrast

  if OpenTMate2BuildOutputReport(LCDVector, Report) then
    Writeln(Length(Report));
end;
```

Write the resulting 64-byte `Report` through the USB transport.

## Console Example

Build:

```powershell
cd D:\Code\OpenTMate2Lib\examples\delphi
dcc32 TMate2ConsoleDemo.dpr
```

Run:

```powershell
.\TMate2ConsoleDemo.exe
```
