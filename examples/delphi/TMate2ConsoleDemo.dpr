program TMate2ConsoleDemo;

{$APPTYPE CONSOLE}

uses
  System.SysUtils,
  OpenTMate2 in '..\..\bindings\delphi\OpenTMate2.pas';

procedure PrintPressedKeys(Keys: Word);
const
  Masks: array[0..8] of Word = (
    OPENTMATE2_KEY_F1,
    OPENTMATE2_KEY_F2,
    OPENTMATE2_KEY_F3,
    OPENTMATE2_KEY_F4,
    OPENTMATE2_KEY_F5,
    OPENTMATE2_KEY_F6,
    OPENTMATE2_KEY_MAIN_ENCODER,
    OPENTMATE2_KEY_ENCODER2,
    OPENTMATE2_KEY_ENCODER1);
var
  I: Integer;
begin
  for I := Low(Masks) to High(Masks) do
    if OpenTMate2KeyIsPressed(Keys, Masks[I]) then
      Writeln('Pressed: ', OpenTMate2KeyName(Masks[I]));
end;

var
  Report: TBytes;
  LCDVector: TBytes;
  OutputReport: TBytes;
  Input: TOpenTMate2Input;
begin
  try
    Report := TBytes.Create(
      $01, $4A, $00, $0B, $00, $FE, $FF, $FF, $01);

    if OpenTMate2ParseInputReport(Report, Input) then
    begin
      Writeln('ReportId: ', Input.ReportId);
      Writeln('Enc1: ', Input.Enc1);
      Writeln('Enc2: ', Input.Enc2);
      Writeln('Enc3: ', Input.Enc3);
      Writeln(Format('Keys: 0x%.4x', [Input.Keys]));
      PrintPressedKeys(Input.Keys);
    end;

    SetLength(LCDVector, OPENTMATE2_LCD_VECTOR_SIZE);
    LCDVector[33] := $20;
    LCDVector[34] := $80;
    LCDVector[35] := $20;
    LCDVector[36] := $28;

    if OpenTMate2BuildOutputReport(LCDVector, OutputReport) then
      Writeln('Output report bytes: ', Length(OutputReport))
    else
      Writeln('Invalid LCD vector length');
  except
    on E: Exception do
    begin
      Writeln(E.ClassName, ': ', E.Message);
      Halt(1);
    end;
  end;
end.
