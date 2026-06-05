unit OpenTMate2;

{
  OpenTMate2 — Delphi binding for the ELAD / WoodBoxRadio TMate 2 SDR
  controller (USB VID $1721 / PID $0614).

  Pure-Pascal port of the OpenTMate2Lib C core: input report parsing, encoder
  deltas, key helpers, output framing, and the full LCD display layer (segment
  map, 7-segment digit encoders, status / backlight / contrast). No USB or OS
  dependency — feed raw HID reports in, get LCDVector bytes out.

  Byte/bit positions and digit encodings match src/opentmate2.c exactly and are
  validated against the same hardware captures.
}

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

  { Input key masks (active-low: bit clear = pressed) }
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

  { LCDVector byte offsets (bytes 32..43) }
  OPENTMATE2_LCD_LED_STATUS = 32;
  OPENTMATE2_LCD_BACKLIGHT_R = 33;
  OPENTMATE2_LCD_BACKLIGHT_G = 34;
  OPENTMATE2_LCD_BACKLIGHT_B = 35;
  OPENTMATE2_LCD_CONTRAST = 36;
  OPENTMATE2_LCD_REFRESH = 37;   { x 10 ms }
  OPENTMATE2_LCD_SPEED1 = 38;
  OPENTMATE2_LCD_SPEED2 = 39;
  OPENTMATE2_LCD_SPEED3 = 40;
  OPENTMATE2_LCD_THR_12 = 41;
  OPENTMATE2_LCD_THR_23 = 42;
  OPENTMATE2_LCD_EVAL_TIME = 43;

  { LED status bits (byte 32) }
  OPENTMATE2_LED_USB = $01;   { USB / radio connected }
  OPENTMATE2_LED_LOCK = $02;  { VFO locked }

  { Indicator segment IDs (0..76) — argument to OpenTMate2SetSegment }
  OPENTMATE2_SEG_SMETER_LINE = 0;
  OPENTMATE2_SEG_SMETER_DB_MINUS = 1;
  OPENTMATE2_SEG_CW_PLUS = 2;
  OPENTMATE2_SEG_CW_MINUS = 3;
  OPENTMATE2_SEG_DIG_PLUS = 4;
  OPENTMATE2_SEG_DIG_MINUS = 5;
  OPENTMATE2_SEG_UNDERLINE_9 = 6;
  OPENTMATE2_SEG_UNDERLINE_8 = 7;
  OPENTMATE2_SEG_UNDERLINE_7 = 8;
  OPENTMATE2_SEG_UNDERLINE_6 = 9;
  OPENTMATE2_SEG_UNDERLINE_5 = 10;
  OPENTMATE2_SEG_UNDERLINE_4 = 11;
  OPENTMATE2_SEG_UNDERLINE_3 = 12;
  OPENTMATE2_SEG_UNDERLINE_2 = 13;
  OPENTMATE2_SEG_UNDERLINE_1 = 14;
  OPENTMATE2_SEG_DOT1 = 15;   { decimal point after digit 3 }
  OPENTMATE2_SEG_DOT2 = 16;   { decimal point after digit 6 }
  OPENTMATE2_SEG_HZ = 17;
  OPENTMATE2_SEG_E1 = 18;
  OPENTMATE2_SEG_ERR = 19;
  OPENTMATE2_SEG_E2 = 20;
  OPENTMATE2_SEG_LP = 21;
  OPENTMATE2_SEG_ATT = 22;
  OPENTMATE2_SEG_S = 23;
  OPENTMATE2_SEG_VFO = 24;
  OPENTMATE2_SEG_NR = 25;
  OPENTMATE2_SEG_NB = 26;
  OPENTMATE2_SEG_SMETER_1 = 27;
  OPENTMATE2_SEG_A = 28;
  OPENTMATE2_SEG_B = 29;
  OPENTMATE2_SEG_VOL = 30;
  OPENTMATE2_SEG_SMETER_9 = 31;
  OPENTMATE2_SEG_SMETER_7 = 32;
  OPENTMATE2_SEG_SMETER_5 = 33;
  OPENTMATE2_SEG_SMETER_3 = 34;
  OPENTMATE2_SEG_AN = 35;
  OPENTMATE2_SEG_RFG = 36;
  OPENTMATE2_SEG_SQL = 37;
  OPENTMATE2_SEG_DRV = 38;
  OPENTMATE2_SEG_SHIFT = 39;
  OPENTMATE2_SEG_LOW = 40;
  OPENTMATE2_SEG_HIGH = 41;
  OPENTMATE2_SEG_DSB = 42;
  OPENTMATE2_SEG_FM = 43;
  OPENTMATE2_SEG_USB = 44;
  OPENTMATE2_SEG_SAM = 45;
  OPENTMATE2_SEG_DRM = 46;
  OPENTMATE2_SEG_DIG = 47;
  OPENTMATE2_SEG_STEREO = 48;
  OPENTMATE2_SEG_DBM = 49;
  OPENTMATE2_SEG_CW = 50;
  OPENTMATE2_SEG_LSB = 51;
  OPENTMATE2_SEG_AM = 52;
  OPENTMATE2_SEG_SMETER_PLUS20 = 53;
  OPENTMATE2_SEG_SMETER_PLUS40 = 54;
  OPENTMATE2_SEG_SMETER_PLUS60 = 55;
  OPENTMATE2_SEG_SMETER_10 = 56;
  OPENTMATE2_SEG_SMETER_20 = 57;
  OPENTMATE2_SEG_SMETER_40 = 58;
  OPENTMATE2_SEG_SMETER_60 = 59;
  OPENTMATE2_SEG_RX = 60;
  OPENTMATE2_SEG_TX = 61;
  OPENTMATE2_SEG_ATT_1 = 62;
  OPENTMATE2_SEG_ATT_2 = 63;
  OPENTMATE2_SEG_PRE = 64;
  OPENTMATE2_SEG_PRE_1 = 65;
  OPENTMATE2_SEG_PRE_2 = 66;
  OPENTMATE2_SEG_MW_W = 67;
  OPENTMATE2_SEG_MW_M = 68;
  OPENTMATE2_SEG_W = 69;
  OPENTMATE2_SEG_K = 70;
  OPENTMATE2_SEG_RIT = 71;
  OPENTMATE2_SEG_XIT = 72;
  OPENTMATE2_SEG_W_FM = 73;
  OPENTMATE2_SEG_NR2 = 74;
  OPENTMATE2_SEG_NB2 = 75;
  OPENTMATE2_SEG_AN2 = 76;

  { S-meter bargraph (IDs 77..91), bar 1 = weakest }
  OPENTMATE2_SMETER_BAR1 = 77;
  OPENTMATE2_SMETER_BAR2 = 78;
  OPENTMATE2_SMETER_BAR3 = 79;
  OPENTMATE2_SMETER_BAR4 = 80;
  OPENTMATE2_SMETER_BAR5 = 81;
  OPENTMATE2_SMETER_BAR6 = 82;
  OPENTMATE2_SMETER_BAR7 = 83;
  OPENTMATE2_SMETER_BAR8 = 84;
  OPENTMATE2_SMETER_BAR9 = 85;
  OPENTMATE2_SMETER_BAR10 = 86;
  OPENTMATE2_SMETER_BAR11 = 87;
  OPENTMATE2_SMETER_BAR12 = 88;
  OPENTMATE2_SMETER_BAR13 = 89;
  OPENTMATE2_SMETER_BAR14 = 90;
  OPENTMATE2_SMETER_BAR15 = 91;

  { Main 9-digit display segments (IDs 92..154) — digit 9 (leftmost) to 1 }
  OPENTMATE2_SEG_MAIN_9A = 92;
  OPENTMATE2_SEG_MAIN_9B = 93;
  OPENTMATE2_SEG_MAIN_9C = 94;
  OPENTMATE2_SEG_MAIN_9D = 95;
  OPENTMATE2_SEG_MAIN_9E = 96;
  OPENTMATE2_SEG_MAIN_9F = 97;
  OPENTMATE2_SEG_MAIN_9G = 98;
  OPENTMATE2_SEG_MAIN_8A = 99;
  OPENTMATE2_SEG_MAIN_8B = 100;
  OPENTMATE2_SEG_MAIN_8C = 101;
  OPENTMATE2_SEG_MAIN_8D = 102;
  OPENTMATE2_SEG_MAIN_8E = 103;
  OPENTMATE2_SEG_MAIN_8F = 104;
  OPENTMATE2_SEG_MAIN_8G = 105;
  OPENTMATE2_SEG_MAIN_7A = 106;
  OPENTMATE2_SEG_MAIN_7B = 107;
  OPENTMATE2_SEG_MAIN_7C = 108;
  OPENTMATE2_SEG_MAIN_7D = 109;
  OPENTMATE2_SEG_MAIN_7E = 110;
  OPENTMATE2_SEG_MAIN_7F = 111;
  OPENTMATE2_SEG_MAIN_7G = 112;
  OPENTMATE2_SEG_MAIN_6A = 113;
  OPENTMATE2_SEG_MAIN_6B = 114;
  OPENTMATE2_SEG_MAIN_6C = 115;
  OPENTMATE2_SEG_MAIN_6D = 116;
  OPENTMATE2_SEG_MAIN_6E = 117;
  OPENTMATE2_SEG_MAIN_6F = 118;
  OPENTMATE2_SEG_MAIN_6G = 119;
  OPENTMATE2_SEG_MAIN_5A = 120;
  OPENTMATE2_SEG_MAIN_5B = 121;
  OPENTMATE2_SEG_MAIN_5C = 122;
  OPENTMATE2_SEG_MAIN_5D = 123;
  OPENTMATE2_SEG_MAIN_5E = 124;
  OPENTMATE2_SEG_MAIN_5F = 125;
  OPENTMATE2_SEG_MAIN_5G = 126;
  OPENTMATE2_SEG_MAIN_4A = 127;
  OPENTMATE2_SEG_MAIN_4B = 128;
  OPENTMATE2_SEG_MAIN_4C = 129;
  OPENTMATE2_SEG_MAIN_4D = 130;
  OPENTMATE2_SEG_MAIN_4E = 131;
  OPENTMATE2_SEG_MAIN_4F = 132;
  OPENTMATE2_SEG_MAIN_4G = 133;
  OPENTMATE2_SEG_MAIN_3A = 134;
  OPENTMATE2_SEG_MAIN_3B = 135;
  OPENTMATE2_SEG_MAIN_3C = 136;
  OPENTMATE2_SEG_MAIN_3D = 137;
  OPENTMATE2_SEG_MAIN_3E = 138;
  OPENTMATE2_SEG_MAIN_3F = 139;
  OPENTMATE2_SEG_MAIN_3G = 140;
  OPENTMATE2_SEG_MAIN_2A = 141;
  OPENTMATE2_SEG_MAIN_2B = 142;
  OPENTMATE2_SEG_MAIN_2C = 143;
  OPENTMATE2_SEG_MAIN_2D = 144;
  OPENTMATE2_SEG_MAIN_2E = 145;
  OPENTMATE2_SEG_MAIN_2F = 146;
  OPENTMATE2_SEG_MAIN_2G = 147;
  OPENTMATE2_SEG_MAIN_1A = 148;
  OPENTMATE2_SEG_MAIN_1B = 149;
  OPENTMATE2_SEG_MAIN_1C = 150;
  OPENTMATE2_SEG_MAIN_1D = 151;
  OPENTMATE2_SEG_MAIN_1E = 152;
  OPENTMATE2_SEG_MAIN_1F = 153;
  OPENTMATE2_SEG_MAIN_1G = 154;

  { Small (S-meter) 3-digit display segments (IDs 155..175) — digit 3 to 1 }
  OPENTMATE2_SEG_SMETER_3A = 155;
  OPENTMATE2_SEG_SMETER_3B = 156;
  OPENTMATE2_SEG_SMETER_3C = 157;
  OPENTMATE2_SEG_SMETER_3D = 158;
  OPENTMATE2_SEG_SMETER_3E = 159;
  OPENTMATE2_SEG_SMETER_3F = 160;
  OPENTMATE2_SEG_SMETER_3G = 161;
  OPENTMATE2_SEG_SMETER_2A = 162;
  OPENTMATE2_SEG_SMETER_2B = 163;
  OPENTMATE2_SEG_SMETER_2C = 164;
  OPENTMATE2_SEG_SMETER_2D = 165;
  OPENTMATE2_SEG_SMETER_2E = 166;
  OPENTMATE2_SEG_SMETER_2F = 167;
  OPENTMATE2_SEG_SMETER_2G = 168;
  OPENTMATE2_SEG_SMETER_1A = 169;
  OPENTMATE2_SEG_SMETER_1B = 170;
  OPENTMATE2_SEG_SMETER_1C = 171;
  OPENTMATE2_SEG_SMETER_1D = 172;
  OPENTMATE2_SEG_SMETER_1E = 173;
  OPENTMATE2_SEG_SMETER_1F = 174;
  OPENTMATE2_SEG_SMETER_1G = 175;

  OPENTMATE2_SEGMENT_COUNT = 176;

type
  TOpenTMate2Input = record
    ReportId: Byte;
    Enc1: Word;
    Enc2: Word;
    Enc3: Word;
    Keys: Word;
  end;

{ ── Input ─────────────────────────────────────────────────────────────── }
function OpenTMate2ParseInputReport(const Report: TBytes; out Value: TOpenTMate2Input): Boolean;
function OpenTMate2EncoderDelta(Current, Previous: Word): Integer;
function OpenTMate2KeyIsPressed(Keys, Mask: Word): Boolean;
function OpenTMate2KeyName(Mask: Word): string;

{ ── Output framing ────────────────────────────────────────────────────── }
function OpenTMate2BuildOutputReport(const LCDVector: TBytes; out Report: TBytes): Boolean;

{ ── LCDVector lifecycle ───────────────────────────────────────────────── }
{ Allocates LCDVector to 44 bytes, zeroes it, and applies the captured timing
  defaults (contrast/refresh/speed/thresholds). }
procedure OpenTMate2LcdInit(var LCDVector: TBytes);
{ Zeroes only the 32 segment/display bytes, leaving LED/backlight/timing. }
function OpenTMate2LcdClearDisplay(var LCDVector: TBytes): Boolean;

{ ── Segment + display ─────────────────────────────────────────────────── }
function OpenTMate2SetSegment(var LCDVector: TBytes; SegmentId: Integer; Enabled: Boolean): Boolean;
function OpenTMate2WriteMainDisplay(var LCDVector: TBytes; ValueHz: Cardinal): Boolean;
function OpenTMate2WriteSmallDisplay(var LCDVector: TBytes; Value: Cardinal): Boolean;

{ ── Status + appearance ───────────────────────────────────────────────── }
function OpenTMate2SetStatus(var LCDVector: TBytes; LedByte: Byte): Boolean;
function OpenTMate2SetBacklight(var LCDVector: TBytes; R, G, B: Byte): Boolean;
function OpenTMate2SetContrast(var LCDVector: TBytes; Contrast: Byte): Boolean;

implementation

type
  TOpenTMate2SegBit = record
    ByteIdx: Byte;
    Mask: Byte;
  end;

const
  { Segment lookup table (176 entries) — byte/bit per segment ID.
    Mirrors kSegmentMap in src/opentmate2.c. }
  SegmentMap: array[0..OPENTMATE2_SEGMENT_COUNT - 1] of TOpenTMate2SegBit = (
    (ByteIdx: 2;  Mask: $01),  { 0  SMETER_LINE }
    (ByteIdx: 28; Mask: $10),  { 1  SMETER_DB_MINUS }
    (ByteIdx: 21; Mask: $01),  { 2  CW_PLUS }
    (ByteIdx: 21; Mask: $02),  { 3  CW_MINUS }
    (ByteIdx: 21; Mask: $04),  { 4  DIG_PLUS }
    (ByteIdx: 21; Mask: $08),  { 5  DIG_MINUS }
    (ByteIdx: 4;  Mask: $08),  { 6  UNDERLINE_9 }
    (ByteIdx: 6;  Mask: $08),  { 7  UNDERLINE_8 }
    (ByteIdx: 8;  Mask: $08),  { 8  UNDERLINE_7 }
    (ByteIdx: 10; Mask: $08),  { 9  UNDERLINE_6 }
    (ByteIdx: 12; Mask: $08),  { 10 UNDERLINE_5 }
    (ByteIdx: 14; Mask: $08),  { 11 UNDERLINE_4 }
    (ByteIdx: 16; Mask: $08),  { 12 UNDERLINE_3 }
    (ByteIdx: 18; Mask: $08),  { 13 UNDERLINE_2 }
    (ByteIdx: 20; Mask: $08),  { 14 UNDERLINE_1 }
    (ByteIdx: 9;  Mask: $10),  { 15 DOT1 }
    (ByteIdx: 15; Mask: $10),  { 16 DOT2 }
    (ByteIdx: 23; Mask: $01),  { 17 HZ }
    (ByteIdx: 0;  Mask: $80),  { 18 E1 }
    (ByteIdx: 19; Mask: $10),  { 19 ERR }
    (ByteIdx: 8;  Mask: $10),  { 20 E2 }
    (ByteIdx: 0;  Mask: $02),  { 21 LP }
    (ByteIdx: 1;  Mask: $01),  { 22 ATT }
    (ByteIdx: 0;  Mask: $10),  { 23 S }
    (ByteIdx: 0;  Mask: $20),  { 24 VFO }
    (ByteIdx: 0;  Mask: $40),  { 25 NR }
    (ByteIdx: 1;  Mask: $40),  { 26 NB }
    (ByteIdx: 1;  Mask: $10),  { 27 SMETER_1 }
    (ByteIdx: 1;  Mask: $20),  { 28 A }
    (ByteIdx: 2;  Mask: $20),  { 29 B }
    (ByteIdx: 1;  Mask: $80),  { 30 VOL }
    (ByteIdx: 2;  Mask: $02),  { 31 SMETER_9 }
    (ByteIdx: 2;  Mask: $04),  { 32 SMETER_7 }
    (ByteIdx: 2;  Mask: $08),  { 33 SMETER_5 }
    (ByteIdx: 2;  Mask: $10),  { 34 SMETER_3 }
    (ByteIdx: 2;  Mask: $40),  { 35 AN }
    (ByteIdx: 2;  Mask: $80),  { 36 RFG }
    (ByteIdx: 3;  Mask: $10),  { 37 SQL }
    (ByteIdx: 4;  Mask: $10),  { 38 DRV }
    (ByteIdx: 12; Mask: $10),  { 39 SHIFT }
    (ByteIdx: 11; Mask: $10),  { 40 LOW }
    (ByteIdx: 10; Mask: $10),  { 41 HIGH }
    (ByteIdx: 21; Mask: $10),  { 42 DSB }
    (ByteIdx: 21; Mask: $20),  { 43 FM }
    (ByteIdx: 21; Mask: $40),  { 44 USB }
    (ByteIdx: 21; Mask: $80),  { 45 SAM }
    (ByteIdx: 22; Mask: $01),  { 46 DRM }
    (ByteIdx: 22; Mask: $02),  { 47 DIG }
    (ByteIdx: 22; Mask: $04),  { 48 STEREO }
    (ByteIdx: 22; Mask: $10),  { 49 DBM }
    (ByteIdx: 22; Mask: $20),  { 50 CW }
    (ByteIdx: 22; Mask: $40),  { 51 LSB }
    (ByteIdx: 22; Mask: $80),  { 52 AM }
    (ByteIdx: 9;  Mask: $20),  { 53 SMETER_PLUS20 }
    (ByteIdx: 15; Mask: $20),  { 54 SMETER_PLUS40 }
    (ByteIdx: 18; Mask: $20),  { 55 SMETER_PLUS60 }
    (ByteIdx: 8;  Mask: $20),  { 56 SMETER_10 }
    (ByteIdx: 10; Mask: $20),  { 57 SMETER_20 }
    (ByteIdx: 16; Mask: $20),  { 58 SMETER_40 }
    (ByteIdx: 19; Mask: $20),  { 59 SMETER_60 }
    (ByteIdx: 0;  Mask: $04),  { 60 RX }
    (ByteIdx: 0;  Mask: $08),  { 61 TX }
    (ByteIdx: 31; Mask: $04),  { 62 ATT_1 }
    (ByteIdx: 31; Mask: $01),  { 63 ATT_2 }
    (ByteIdx: 31; Mask: $02),  { 64 PRE }
    (ByteIdx: 30; Mask: $01),  { 65 PRE_1 }
    (ByteIdx: 30; Mask: $02),  { 66 PRE_2 }
    (ByteIdx: 27; Mask: $01),  { 67 MW_W }
    (ByteIdx: 28; Mask: $01),  { 68 MW_M }
    (ByteIdx: 20; Mask: $20),  { 69 W }
    (ByteIdx: 25; Mask: $01),  { 70 K }
    (ByteIdx: 13; Mask: $10),  { 71 RIT }
    (ByteIdx: 14; Mask: $10),  { 72 XIT }
    (ByteIdx: 20; Mask: $10),  { 73 W_FM }
    (ByteIdx: 5;  Mask: $10),  { 74 NR2 }
    (ByteIdx: 6;  Mask: $10),  { 75 NB2 }
    (ByteIdx: 7;  Mask: $10),  { 76 AN2 }

    { S-meter bargraph (77..91), bar 1 = weakest }
    (ByteIdx: 1;  Mask: $08),  { 77 BAR1 }
    (ByteIdx: 1;  Mask: $04),  { 78 BAR2 }
    (ByteIdx: 1;  Mask: $02),  { 79 BAR3 }
    (ByteIdx: 31; Mask: $80),  { 80 BAR4 }
    (ByteIdx: 31; Mask: $40),  { 81 BAR5 }
    (ByteIdx: 31; Mask: $20),  { 82 BAR6 }
    (ByteIdx: 31; Mask: $10),  { 83 BAR7 }
    (ByteIdx: 30; Mask: $10),  { 84 BAR8 }
    (ByteIdx: 30; Mask: $20),  { 85 BAR9 }
    (ByteIdx: 30; Mask: $40),  { 86 BAR10 }
    (ByteIdx: 30; Mask: $80),  { 87 BAR11 }
    (ByteIdx: 29; Mask: $80),  { 88 BAR12 }
    (ByteIdx: 29; Mask: $40),  { 89 BAR13 }
    (ByteIdx: 29; Mask: $20),  { 90 BAR14 }
    (ByteIdx: 29; Mask: $10),  { 91 BAR15 }

    { Main display digits 9..1, segments A..G }
    (ByteIdx: 4;  Mask: $01),  { 92  MAIN_9A }
    (ByteIdx: 4;  Mask: $02),  { 93  MAIN_9B }
    (ByteIdx: 4;  Mask: $04),  { 94  MAIN_9C }
    (ByteIdx: 3;  Mask: $08),  { 95  MAIN_9D }
    (ByteIdx: 3;  Mask: $04),  { 96  MAIN_9E }
    (ByteIdx: 3;  Mask: $01),  { 97  MAIN_9F }
    (ByteIdx: 3;  Mask: $02),  { 98  MAIN_9G }
    (ByteIdx: 6;  Mask: $01),  { 99  MAIN_8A }
    (ByteIdx: 6;  Mask: $02),  { 100 MAIN_8B }
    (ByteIdx: 6;  Mask: $04),  { 101 MAIN_8C }
    (ByteIdx: 5;  Mask: $08),  { 102 MAIN_8D }
    (ByteIdx: 5;  Mask: $04),  { 103 MAIN_8E }
    (ByteIdx: 5;  Mask: $01),  { 104 MAIN_8F }
    (ByteIdx: 5;  Mask: $02),  { 105 MAIN_8G }
    (ByteIdx: 8;  Mask: $01),  { 106 MAIN_7A }
    (ByteIdx: 8;  Mask: $02),  { 107 MAIN_7B }
    (ByteIdx: 8;  Mask: $04),  { 108 MAIN_7C }
    (ByteIdx: 7;  Mask: $08),  { 109 MAIN_7D }
    (ByteIdx: 7;  Mask: $04),  { 110 MAIN_7E }
    (ByteIdx: 7;  Mask: $01),  { 111 MAIN_7F }
    (ByteIdx: 7;  Mask: $02),  { 112 MAIN_7G }
    (ByteIdx: 10; Mask: $01),  { 113 MAIN_6A }
    (ByteIdx: 10; Mask: $02),  { 114 MAIN_6B }
    (ByteIdx: 10; Mask: $04),  { 115 MAIN_6C }
    (ByteIdx: 9;  Mask: $08),  { 116 MAIN_6D }
    (ByteIdx: 9;  Mask: $04),  { 117 MAIN_6E }
    (ByteIdx: 9;  Mask: $01),  { 118 MAIN_6F }
    (ByteIdx: 9;  Mask: $02),  { 119 MAIN_6G }
    (ByteIdx: 12; Mask: $01),  { 120 MAIN_5A }
    (ByteIdx: 12; Mask: $02),  { 121 MAIN_5B }
    (ByteIdx: 12; Mask: $04),  { 122 MAIN_5C }
    (ByteIdx: 11; Mask: $08),  { 123 MAIN_5D }
    (ByteIdx: 11; Mask: $04),  { 124 MAIN_5E }
    (ByteIdx: 11; Mask: $01),  { 125 MAIN_5F }
    (ByteIdx: 11; Mask: $02),  { 126 MAIN_5G }
    (ByteIdx: 14; Mask: $01),  { 127 MAIN_4A }
    (ByteIdx: 14; Mask: $02),  { 128 MAIN_4B }
    (ByteIdx: 14; Mask: $04),  { 129 MAIN_4C }
    (ByteIdx: 13; Mask: $08),  { 130 MAIN_4D }
    (ByteIdx: 13; Mask: $04),  { 131 MAIN_4E }
    (ByteIdx: 13; Mask: $01),  { 132 MAIN_4F }
    (ByteIdx: 13; Mask: $02),  { 133 MAIN_4G }
    (ByteIdx: 16; Mask: $01),  { 134 MAIN_3A }
    (ByteIdx: 16; Mask: $02),  { 135 MAIN_3B }
    (ByteIdx: 16; Mask: $04),  { 136 MAIN_3C }
    (ByteIdx: 15; Mask: $08),  { 137 MAIN_3D }
    (ByteIdx: 15; Mask: $04),  { 138 MAIN_3E }
    (ByteIdx: 15; Mask: $01),  { 139 MAIN_3F }
    (ByteIdx: 15; Mask: $02),  { 140 MAIN_3G }
    (ByteIdx: 18; Mask: $01),  { 141 MAIN_2A }
    (ByteIdx: 18; Mask: $02),  { 142 MAIN_2B }
    (ByteIdx: 18; Mask: $04),  { 143 MAIN_2C }
    (ByteIdx: 17; Mask: $08),  { 144 MAIN_2D }
    (ByteIdx: 17; Mask: $04),  { 145 MAIN_2E }
    (ByteIdx: 17; Mask: $01),  { 146 MAIN_2F }
    (ByteIdx: 17; Mask: $02),  { 147 MAIN_2G }
    (ByteIdx: 20; Mask: $01),  { 148 MAIN_1A }
    (ByteIdx: 20; Mask: $02),  { 149 MAIN_1B }
    (ByteIdx: 20; Mask: $04),  { 150 MAIN_1C }
    (ByteIdx: 19; Mask: $08),  { 151 MAIN_1D }
    (ByteIdx: 19; Mask: $04),  { 152 MAIN_1E }
    (ByteIdx: 19; Mask: $01),  { 153 MAIN_1F }
    (ByteIdx: 19; Mask: $02),  { 154 MAIN_1G }

    { Small display digits 3..1, segments A..G (reversed bit layout) }
    (ByteIdx: 27; Mask: $80),  { 155 SMETER_3A }
    (ByteIdx: 27; Mask: $40),  { 156 SMETER_3B }
    (ByteIdx: 27; Mask: $20),  { 157 SMETER_3C }
    (ByteIdx: 27; Mask: $10),  { 158 SMETER_3D }
    (ByteIdx: 28; Mask: $20),  { 159 SMETER_3E }
    (ByteIdx: 28; Mask: $80),  { 160 SMETER_3F }
    (ByteIdx: 28; Mask: $40),  { 161 SMETER_3G }
    (ByteIdx: 25; Mask: $80),  { 162 SMETER_2A }
    (ByteIdx: 25; Mask: $40),  { 163 SMETER_2B }
    (ByteIdx: 25; Mask: $20),  { 164 SMETER_2C }
    (ByteIdx: 25; Mask: $10),  { 165 SMETER_2D }
    (ByteIdx: 26; Mask: $20),  { 166 SMETER_2E }
    (ByteIdx: 26; Mask: $80),  { 167 SMETER_2F }
    (ByteIdx: 26; Mask: $40),  { 168 SMETER_2G }
    (ByteIdx: 23; Mask: $80),  { 169 SMETER_1A }
    (ByteIdx: 23; Mask: $40),  { 170 SMETER_1B }
    (ByteIdx: 23; Mask: $20),  { 171 SMETER_1C }
    (ByteIdx: 23; Mask: $10),  { 172 SMETER_1D }
    (ByteIdx: 24; Mask: $20),  { 173 SMETER_1E }
    (ByteIdx: 24; Mask: $80),  { 174 SMETER_1F }
    (ByteIdx: 24; Mask: $40)   { 175 SMETER_1G }
  );

  { Main display: high byte bits A=$01 B=$02 C=$04, low byte F=$01 G=$02 E=$04 D=$08 }
  MainDigitHigh: array[0..9] of Byte = ($07, $06, $03, $07, $06, $05, $05, $07, $07, $07);
  MainDigitLow:  array[0..9] of Byte = ($0D, $00, $0E, $0A, $03, $0B, $0F, $00, $0F, $0B);

  { Small display: high byte A=$80 B=$40 C=$20 D=$10, low byte E=$20 F=$80 G=$40 }
  SmallDigitHigh: array[0..9] of Byte = ($F0, $60, $D0, $F0, $60, $B0, $B0, $E0, $F0, $F0);
  SmallDigitLow:  array[0..9] of Byte = ($A0, $00, $60, $40, $C0, $C0, $E0, $00, $E0, $C0);

function HasLcdVector(const LCDVector: TBytes): Boolean;
begin
  Result := Length(LCDVector) >= OPENTMATE2_LCD_VECTOR_SIZE;
end;

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

procedure OpenTMate2LcdInit(var LCDVector: TBytes);
begin
  SetLength(LCDVector, OPENTMATE2_LCD_VECTOR_SIZE);
  FillChar(LCDVector[0], OPENTMATE2_LCD_VECTOR_SIZE, 0);

  { Timing defaults as captured from hardware. }
  LCDVector[OPENTMATE2_LCD_CONTRAST]  := $28; { 40 — good all-round value }
  LCDVector[OPENTMATE2_LCD_REFRESH]   := $28; { 40 x 10 ms = 400 ms }
  LCDVector[OPENTMATE2_LCD_SPEED1]    := $01;
  LCDVector[OPENTMATE2_LCD_SPEED2]    := $05;
  LCDVector[OPENTMATE2_LCD_SPEED3]    := $0A;
  LCDVector[OPENTMATE2_LCD_THR_12]    := $0F;
  LCDVector[OPENTMATE2_LCD_THR_23]    := $19;
  LCDVector[OPENTMATE2_LCD_EVAL_TIME] := $0A;
end;

function OpenTMate2LcdClearDisplay(var LCDVector: TBytes): Boolean;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;
  FillChar(LCDVector[0], 32, 0); { segment area only; bytes 32..43 unchanged }
end;

function OpenTMate2SetSegment(var LCDVector: TBytes; SegmentId: Integer; Enabled: Boolean): Boolean;
var
  Entry: TOpenTMate2SegBit;
begin
  Result := HasLcdVector(LCDVector)
    and (SegmentId >= 0) and (SegmentId < OPENTMATE2_SEGMENT_COUNT);
  if not Result then
    Exit;

  Entry := SegmentMap[SegmentId];
  if Enabled then
    LCDVector[Entry.ByteIdx] := LCDVector[Entry.ByteIdx] or Entry.Mask
  else
    LCDVector[Entry.ByteIdx] := LCDVector[Entry.ByteIdx] and Byte(not Entry.Mask);
end;

function OpenTMate2WriteMainDisplay(var LCDVector: TBytes; ValueHz: Cardinal): Boolean;
var
  D: Integer;
  Hi, Lo, Digit: Integer;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;

  if ValueHz > 999999999 then
    ValueHz := 999999999;

  { Digit 1 (units) .. digit 9 (100 MHz). Only segment bits are touched; the
    indicator bits sharing these bytes (underlines, mode flags) are preserved. }
  for D := 1 to 9 do
  begin
    Hi := 22 - 2 * D;  { holds A B C in bits 0-2 }
    Lo := 21 - 2 * D;  { holds F G E D in bits 0-3 }

    LCDVector[Hi] := LCDVector[Hi] and $F8;
    LCDVector[Lo] := LCDVector[Lo] and $F0;

    if (ValueHz = 0) and (D > 1) then
      Continue; { blank — already cleared above }

    Digit := ValueHz mod 10;
    LCDVector[Hi] := LCDVector[Hi] or MainDigitHigh[Digit];
    LCDVector[Lo] := LCDVector[Lo] or MainDigitLow[Digit];
    ValueHz := ValueHz div 10;
  end;
end;

function OpenTMate2WriteSmallDisplay(var LCDVector: TBytes; Value: Cardinal): Boolean;
var
  D: Integer;
  Hi, Lo, Digit: Integer;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;

  Value := Value mod 1000; { 3-digit display; wrap larger values }

  { Digit 1 (units) .. digit 3 (hundreds). Reversed bit order vs main display.
    Low bits of the high/low bytes (SEG_HZ, SEG_K, SEG_MW_W) are preserved. }
  for D := 1 to 3 do
  begin
    Hi := 21 + 2 * D;  { holds A B C D in bits 4-7 }
    Lo := 22 + 2 * D;  { holds E F G in bits 5-7 }

    LCDVector[Hi] := LCDVector[Hi] and $0F;
    LCDVector[Lo] := LCDVector[Lo] and $1F;

    if (Value = 0) and (D > 1) then
      Continue;

    Digit := Value mod 10;
    LCDVector[Hi] := LCDVector[Hi] or SmallDigitHigh[Digit];
    LCDVector[Lo] := LCDVector[Lo] or SmallDigitLow[Digit];
    Value := Value div 10;
  end;
end;

function OpenTMate2SetStatus(var LCDVector: TBytes; LedByte: Byte): Boolean;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;
  LCDVector[OPENTMATE2_LCD_LED_STATUS] := LedByte;
end;

function OpenTMate2SetBacklight(var LCDVector: TBytes; R, G, B: Byte): Boolean;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;
  LCDVector[OPENTMATE2_LCD_BACKLIGHT_R] := R;
  LCDVector[OPENTMATE2_LCD_BACKLIGHT_G] := G;
  LCDVector[OPENTMATE2_LCD_BACKLIGHT_B] := B;
end;

function OpenTMate2SetContrast(var LCDVector: TBytes; Contrast: Byte): Boolean;
begin
  Result := HasLcdVector(LCDVector);
  if not Result then
    Exit;
  LCDVector[OPENTMATE2_LCD_CONTRAST] := Contrast;
end;

end.
