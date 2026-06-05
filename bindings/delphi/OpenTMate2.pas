unit OpenTMate2;

interface

uses
  System.SysUtils;

const
  OPENTMATE2_VENDOR_ID = $1721;
  OPENTMATE2_PRODUCT_ID = $0614;

  OPENTMATE2_IN_ENDPOINT = $81;
  OPENTMATE2_OUT_ENDPOINT = $01;

  OPENTMATE2_REPORT_SIZE = 64;
  OPENTMATE2_INPUT_MIN_SIZE = 9;
  OPENTMATE2_LCD_VECTOR_SIZE = 44;

  OPENTMATE2_KEY_F1 = $0001;
  OPENTMATE2_KEY_F2 = $0002;
  OPENTMATE2_KEY_F3 = $0004;
  OPENTMATE2_KEY_F4 = $0008;
  OPENTMATE2_KEY_F5 = $0010;
  OPENTMATE2_KEY_F6 = $0020;
  OPENTMATE2_KEY_MAIN_ENCODER = $0040;
  OPENTMATE2_KEY_ENCODER2 = $0080;
  OPENTMATE2_KEY_ENCODER1 = $0100;
  OPENTMATE2_KEY_IDLE_MASK = $01FF;

type
  TOpenTMate2Input = record
    ReportId: Byte;
    Enc1: Word;
    Enc2: Word;
    Enc3: Word;
    Keys: Word;
  end;

function OpenTMate2ParseInputReport(const Report: TBytes; out Value: TOpenTMate2Input): Boolean;
function OpenTMate2BuildOutputReport(const LCDVector: TBytes; out Report: TBytes): Boolean;
function OpenTMate2EncoderDelta(Current, Previous: Word): Integer;
function OpenTMate2KeyIsPressed(Keys, Mask: Word): Boolean;
function OpenTMate2KeyName(Mask: Word): string;

implementation

function ReadUInt16LE(const Data: TBytes; Offset: Integer): Word;
begin
  Result := Word(Data[Offset]) or (Word(Data[Offset + 1]) shl 8);
end;

function OpenTMate2ParseInputReport(const Report: TBytes; out Value: TOpenTMate2Input): Boolean;
begin
  Result := Length(Report) >= OPENTMATE2_INPUT_MIN_SIZE;
  if not Result then
    Exit;

  Value.ReportId := Report[0];
  Value.Enc1 := ReadUInt16LE(Report, 1);
  Value.Enc2 := ReadUInt16LE(Report, 3);
  Value.Enc3 := ReadUInt16LE(Report, 5);
  Value.Keys := ReadUInt16LE(Report, 7);
end;

function OpenTMate2BuildOutputReport(const LCDVector: TBytes; out Report: TBytes): Boolean;
begin
  SetLength(Report, 0);

  Result := Length(LCDVector) = OPENTMATE2_LCD_VECTOR_SIZE;
  if not Result then
    Exit;

  SetLength(Report, OPENTMATE2_REPORT_SIZE);
  Move(LCDVector[0], Report[0], OPENTMATE2_LCD_VECTOR_SIZE);
end;

function OpenTMate2EncoderDelta(Current, Previous: Word): Integer;
begin
  Result := Integer(Current) - Integer(Previous);

  if Result > 32767 then
    Dec(Result, 65536)
  else if Result < -32768 then
    Inc(Result, 65536);
end;

function OpenTMate2KeyIsPressed(Keys, Mask: Word): Boolean;
begin
  Result := (Keys and Mask) = 0;
end;

function OpenTMate2KeyName(Mask: Word): string;
begin
  case Mask of
    OPENTMATE2_KEY_F1:
      Result := 'F1';
    OPENTMATE2_KEY_F2:
      Result := 'F2';
    OPENTMATE2_KEY_F3:
      Result := 'F3';
    OPENTMATE2_KEY_F4:
      Result := 'F4';
    OPENTMATE2_KEY_F5:
      Result := 'F5';
    OPENTMATE2_KEY_F6:
      Result := 'F6';
    OPENTMATE2_KEY_MAIN_ENCODER:
      Result := 'MAIN_ENCODER';
    OPENTMATE2_KEY_ENCODER2:
      Result := 'ENCODER2';
    OPENTMATE2_KEY_ENCODER1:
      Result := 'ENCODER1';
  else
    Result := 'UNKNOWN';
  end;
end;

end.
