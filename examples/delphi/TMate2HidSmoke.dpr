program TMate2HidSmoke;
{
  Smoke test / minimal end-to-end example for the native Windows HID transport.

  With a TMate 2 connected it opens the device, pushes one display frame
  (backlight + 14.200.000 Hz + USB/RX indicators), and polls a few input
  reports. With no device present it reports that and exits 0, so it is safe
  to run on any machine.

  Build (from this folder):  dcc32 TMate2HidSmoke.dpr   (or dcc64)
  The OpenTMate2 / OpenTMate2HID units live in ..\..\bindings\delphi.
}
{$APPTYPE CONSOLE}
uses
  System.SysUtils,
  OpenTMate2 in '..\..\bindings\delphi\OpenTMate2.pas',
  OpenTMate2HID in '..\..\bindings\delphi\OpenTMate2HID.pas';

procedure Run;
var
  Dev: TOpenTMate2HID;
  Lcd: TBytes;
  Inp: TOpenTMate2Input;
  I, Got: Integer;
begin
  Dev := TOpenTMate2HID.Create;
  try
    if not Dev.Open then
    begin
      Writeln('No TMate 2 found (not connected, or claimed by another app).');
      Writeln('Transport unit is built and linked; nothing to exercise.');
      ExitCode := 0;
      Exit;
    end;

    Writeln('TMate 2 opened.');

    OpenTMate2LcdInit(Lcd);
    OpenTMate2SetBacklight(Lcd, 0, 50, 255);
    OpenTMate2SetStatus(Lcd, OPENTMATE2_LED_USB);
    OpenTMate2WriteMainDisplay(Lcd, 14200000);
    OpenTMate2WriteSmallDisplay(Lcd, 9);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_USB, True);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_RX, True);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_HZ, True);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_DOT1, True);
    OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_DOT2, True);

    if Dev.WriteLcd(Lcd) then
      Writeln('Display frame sent: 14.200.000 Hz, USB, RX.')
    else
      Writeln('WriteLcd failed.');

    Writeln('Polling input for ~2 s (turn an encoder / press a key)...');
    Got := 0;
    for I := 1 to 40 do
      if Dev.ReadInput(Inp, 50) then
      begin
        Inc(Got);
        Writeln(Format('  enc1=%5d  enc2=%5d  enc3=%5d  keys=$%.4x',
          [Inp.Enc1, Inp.Enc2, Inp.Enc3, Inp.Keys]));
      end;
    Writeln(Format('Received %d input report(s).', [Got]));
    ExitCode := 0;
  finally
    Dev.Free;
  end;
end;

begin
  try
    Run;
  except
    on E: Exception do
    begin
      Writeln('ERROR: ', E.ClassName, ': ', E.Message);
      ExitCode := 1;
    end;
  end;
end.
