program test_binding;
{$APPTYPE CONSOLE}
uses
  System.SysUtils,
  OpenTMate2 in 'OpenTMate2.pas';

var
  Failures: Integer = 0;

procedure Check(Cond: Boolean; const Msg: string);
begin
  if not Cond then
  begin
    Writeln('FAIL: ', Msg);
    Inc(Failures);
  end;
end;

var
  Lcd: TBytes;
  Rep: TBytes;
  Inp: TOpenTMate2Input;
  RawIn: TBytes;
begin
  { ── Input parse + encoder wrap ── }
  RawIn := TBytes.Create($01, $02, $01, $04, $03, $FE, $FF, $FF, $01);
  Check(OpenTMate2ParseInputReport(RawIn, Inp), 'parse input');
  Check(Inp.Enc1 = $0102, 'enc1');
  Check(Inp.Enc3 = $FFFE, 'enc3');
  Check(Inp.Keys = $01FF, 'keys idle');
  Check(OpenTMate2EncoderDelta($0002, $FFFF) = 3, 'encoder wrap +');
  Check(OpenTMate2EncoderDelta($FFFF, $0002) = -3, 'encoder wrap -');
  Check(OpenTMate2KeyIsPressed($01FF and not OPENTMATE2_KEY_F1, OPENTMATE2_KEY_F1), 'F1 pressed');
  Check(OpenTMate2KeyName(OPENTMATE2_KEY_F1) = 'F1', 'key name');

  { ── LCD init defaults ── }
  OpenTMate2LcdInit(Lcd);
  Check(Length(Lcd) = OPENTMATE2_LCD_VECTOR_SIZE, 'lcd length');
  Check(Lcd[OPENTMATE2_LCD_CONTRAST] = $28, 'contrast default');
  Check(Lcd[OPENTMATE2_LCD_THR_23] = $19, 'thr23 default');

  { ── Output framing ── }
  Check(OpenTMate2BuildOutputReport(Lcd, Rep), 'build report');
  Check(Length(Rep) = OPENTMATE2_REPORT_SIZE, 'report size 64');

  { ── Segment set/clear ── }
  OpenTMate2LcdInit(Lcd);
  Check(OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_RX, True), 'set RX');
  Check((Lcd[0] and $04) <> 0, 'RX bit set (byte 0, 0x04)');
  Check(OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_RX, False), 'clear RX');
  Check((Lcd[0] and $04) = 0, 'RX bit cleared');
  Check(not OpenTMate2SetSegment(Lcd, OPENTMATE2_SEGMENT_COUNT, True), 'segment range guard');

  { ── Main display + indicator preservation ── }
  OpenTMate2LcdInit(Lcd);
  OpenTMate2LcdClearDisplay(Lcd);
  Check(OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_RIT, True), 'set RIT');
  Check((Lcd[13] and $10) <> 0, 'RIT bit pre');
  Check(OpenTMate2WriteMainDisplay(Lcd, 14200000), 'write main 14.2M');
  Check((Lcd[13] and $10) <> 0, 'RIT preserved after main write');
  { value 0 → digit 1 shows 0: high byte 20 = 0x07, low byte 19 = 0x0D }
  OpenTMate2LcdClearDisplay(Lcd);
  OpenTMate2WriteMainDisplay(Lcd, 0);
  Check((Lcd[20] and $07) = $07, 'digit1 high = 0x07 for ''0''');
  Check((Lcd[19] and $0F) = $0D, 'digit1 low = 0x0D for ''0''');
  Check((Lcd[18] and $07) = $00, 'digit2 blank');

  { ── Small display + HZ preservation ── }
  OpenTMate2LcdClearDisplay(Lcd);
  OpenTMate2SetSegment(Lcd, OPENTMATE2_SEG_HZ, True);
  Check((Lcd[23] and $01) <> 0, 'HZ pre');
  Check(OpenTMate2WriteSmallDisplay(Lcd, 59), 'write small 59');
  Check((Lcd[23] and $01) <> 0, 'HZ preserved after small write');
  OpenTMate2LcdClearDisplay(Lcd);
  OpenTMate2WriteSmallDisplay(Lcd, 0);
  Check((Lcd[23] and $F0) = $F0, 'small digit1 high = 0xF0 for ''0''');
  Check((Lcd[24] and $E0) = $A0, 'small digit1 low = 0xA0 for ''0''');

  { ── Status / backlight / contrast ── }
  OpenTMate2LcdInit(Lcd);
  OpenTMate2SetStatus(Lcd, OPENTMATE2_LED_USB or OPENTMATE2_LED_LOCK);
  Check(Lcd[OPENTMATE2_LCD_LED_STATUS] = $03, 'status byte');
  Check(OpenTMate2SetClick(Lcd, True), 'set click');
  Check(Lcd[OPENTMATE2_LCD_LED_STATUS] = $07, 'click bit set');
  Check(OpenTMate2SetClick(Lcd, False), 'clear click');
  Check(Lcd[OPENTMATE2_LCD_LED_STATUS] = $03, 'click bit cleared');
  Check(OpenTMate2ToggleClick(Lcd), 'toggle click on');
  Check(Lcd[OPENTMATE2_LCD_LED_STATUS] = $07, 'click bit toggled on');
  Check(OpenTMate2ToggleClick(Lcd), 'toggle click off');
  Check(Lcd[OPENTMATE2_LCD_LED_STATUS] = $03, 'click bit toggled off');
  OpenTMate2SetBacklight(Lcd, 10, 20, 30);
  Check((Lcd[33] = 10) and (Lcd[34] = 20) and (Lcd[35] = 30), 'backlight rgb');
  OpenTMate2SetContrast(Lcd, $28);
  Check(Lcd[OPENTMATE2_LCD_CONTRAST] = $28, 'contrast set');

  if Failures = 0 then
  begin
    Writeln('All OpenTMate2 Delphi binding tests passed.');
    ExitCode := 0;
  end
  else
  begin
    Writeln(Failures, ' check(s) failed.');
    ExitCode := 1;
  end;
end.
