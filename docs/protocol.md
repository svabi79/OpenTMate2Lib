# TMate 2 Protocol

This document describes the currently known protocol for the ELAD / WoodBoxRadio TMate 2 SDR controller.

The information comes from correlated Windows DLL calls and USBPcap captures made on 2026-06-05.

## Device

| Field | Value |
| --- | --- |
| Vendor ID | `0x1721` |
| Product ID | `0x0614` |
| IN endpoint | `0x81` |
| OUT endpoint | `0x01` |
| Report size | `64` bytes |

The traffic behaves like HID-style interrupt traffic. The original Windows software talks to the device through `TMATE2_DLL.dll`; OpenTMate2Lib avoids that DLL and works from the decoded report format.

## Input Reports

The device sends 64-byte interrupt IN reports.

Known layout:

| Offset | Size | Meaning |
| --- | --- | --- |
| `0` | 1 | Report ID, observed as `0x01` |
| `1` | 2 | Encoder 1, little-endian unsigned 16-bit |
| `3` | 2 | Encoder 2, little-endian unsigned 16-bit |
| `5` | 2 | Encoder 3, little-endian unsigned 16-bit |
| `7` | 2 | Key bitmask, little-endian unsigned 16-bit |
| `9` | 55 | Additional state, not fully mapped |

Observed idle example:

```text
01 4A 00 0B 00 FE FF FF 01 ...
```

Decoded:

| Field | Value |
| --- | --- |
| report_id | `0x01` |
| enc1 | `0x004A` / `74` |
| enc2 | `0x000B` / `11` |
| enc3 | `0xFFFE` / `65534` |
| keys | `0x01FF` |

## Encoder Counters

The three encoders are unsigned 16-bit counters. Direction is represented by increment or decrement. The counters wrap naturally at `0` and `65535`.

Use wrap-corrected delta logic:

```c
int32_t diff = (int32_t)current - (int32_t)previous;
if (diff > 32767) {
    diff -= 65536;
} else if (diff < -32768) {
    diff += 65536;
}
```

Examples seen in captures:

| Encoder | Observed movement |
| --- | --- |
| enc1 | `79 -> 97 -> 81` |
| enc2 | `9 -> 0 -> 65535 -> 65520 -> 11` |
| enc3 | `1 -> 30 -> 7` |

## Keys

Keys are active-low.

| State | Meaning |
| --- | --- |
| Bit set to `1` | Not pressed |
| Bit cleared to `0` | Pressed |
| Idle mask | `0x01FF` |

Known masks:

| Control | Mask |
| --- | --- |
| F1 | `0x0001` |
| F2 | `0x0002` |
| F3 | `0x0004` |
| F4 | `0x0008` |
| F5 | `0x0010` |
| F6 | `0x0020` |
| Main encoder push | `0x0040` |
| Encoder 2 push | `0x0080` |
| Encoder 1 push | `0x0100` |

Confirmed transitions:

| Keys value | Meaning |
| --- | --- |
| `0x00FF` | Encoder 1 push pressed |
| `0x017F` | Encoder 2 push pressed |
| `0x01BF` | Main encoder push pressed |
| `0x01FE` | F1 pressed |
| `0x01FD` | F2 pressed |
| `0x01FB` | F3 pressed |
| `0x01F7` | F4 pressed |
| `0x01EF` | F5 pressed |
| `0x01DF` | F6 pressed |

## Output Reports

The host sends 64-byte interrupt OUT reports to endpoint `0x01`.

The first 44 bytes are the same `LCDVector` used by the original Delphi code and Windows DLL. Bytes `44..63` are zero padding.

Known `LCDVector` layout:

| Offset | Size | Meaning |
| --- | --- | --- |
| `0..31` | 32 | LCD segment and display bytes |
| `32` | 1 | Status byte: USB LED, lock LED, click control |
| `33` | 1 | Backlight red |
| `34` | 1 | Backlight green |
| `35` | 1 | Backlight blue |
| `36` | 1 | Contrast |
| `37` | 1 | LCD refresh time |
| `38` | 1 | Encoder speed increment 1 |
| `39` | 1 | Encoder speed increment 2 |
| `40` | 1 | Encoder speed increment 3 |
| `41` | 1 | Speed threshold 1 to 2 |
| `42` | 1 | Speed threshold 2 to 3 |
| `43` | 1 | Speed evaluation time |
| `44..63` | 20 | Zero padding in USB report |

Known status byte bits:

| Bit | Mask | Meaning |
| --- | --- | --- |
| `0` | `0x01` | USB / radio connected LED |
| `1` | `0x02` | VFO locked LED |
| `2` | `0x04` | Click control; hardware appears to click when this bit changes value |

The click bit was cross-checked against the independent `microenh/Tmate2_C`
implementation, where it is modeled as a bit in the same byte as the two LED
flags. OpenTMate2Lib exposes this as `opentmate2_set_click()` and
`opentmate2_toggle_click()`.

Example 44-byte vector captured from a `number_0` display update:

```text
00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
00 00 00 0D 07 00 00 F0 A0 00 00 00 00 00 00 00
00 00 32 FF 00 28 01 05 0A 0F 19 0A
```

To send this over USB, append 20 zero bytes to create the 64-byte report.

## Segment map

Bytes `0..31` of the LCDVector hold 176 individually addressable segments.
The layout is highly scrambled — segment IDs are not sequential in memory.

Each segment occupies exactly one bit.  The table was derived from a USB
capture made on 2026-06-05 (`session_20260605_051711`,
`segments_20260605_051745.csv`): the Windows DLL function
`TMate2SetSegment(buf, id, 1)` was called for every ID and the resulting
LCDVector was recorded via USBPcap.

### Indicator segments (IDs 0–76)

| ID | Name | Byte | Bit |
| -- | ---- | ---- | --- |
| 0  | SEG_SMETER_LINE      |  2 | 0x01 |
| 1  | SEG_SMETER_DB_MINUS  | 28 | 0x10 |
| 2  | SEG_CW_PLUS          | 21 | 0x01 |
| 3  | SEG_CW_MINUS         | 21 | 0x02 |
| 4  | SEG_DIG_PLUS         | 21 | 0x04 |
| 5  | SEG_DIG_MINUS        | 21 | 0x08 |
| 6  | SEG_UNDERLINE_9      |  4 | 0x08 |
| 7  | SEG_UNDERLINE_8      |  6 | 0x08 |
| 8  | SEG_UNDERLINE_7      |  8 | 0x08 |
| 9  | SEG_UNDERLINE_6      | 10 | 0x08 |
| 10 | SEG_UNDERLINE_5      | 12 | 0x08 |
| 11 | SEG_UNDERLINE_4      | 14 | 0x08 |
| 12 | SEG_UNDERLINE_3      | 16 | 0x08 |
| 13 | SEG_UNDERLINE_2      | 18 | 0x08 |
| 14 | SEG_UNDERLINE_1      | 20 | 0x08 |
| 15 | SEG_DOT1             |  9 | 0x10 |
| 16 | SEG_DOT2             | 15 | 0x10 |
| 17 | SEG_HZ               | 23 | 0x01 |
| 18 | SEG_E1               |  0 | 0x80 |
| 19 | SEG_ERR              | 19 | 0x10 |
| 20 | SEG_E2               |  8 | 0x10 |
| 21 | SEG_LP               |  0 | 0x02 |
| 22 | SEG_ATT              |  1 | 0x01 |
| 23 | SEG_S                |  0 | 0x10 |
| 24 | SEG_VFO              |  0 | 0x20 |
| 25 | SEG_NR               |  0 | 0x40 |
| 26 | SEG_NB               |  1 | 0x40 |
| 27 | SEG_SMETER_1         |  1 | 0x10 |
| 28 | SEG_A                |  1 | 0x20 |
| 29 | SEG_B                |  2 | 0x20 |
| 30 | SEG_VOL              |  1 | 0x80 |
| 31 | SEG_SMETER_9         |  2 | 0x02 |
| 32 | SEG_SMETER_7         |  2 | 0x04 |
| 33 | SEG_SMETER_5         |  2 | 0x08 |
| 34 | SEG_SMETER_3         |  2 | 0x10 |
| 35 | SEG_AN               |  2 | 0x40 |
| 36 | SEG_RFG              |  2 | 0x80 |
| 37 | SEG_SQL              |  3 | 0x10 |
| 38 | SEG_DRV              |  4 | 0x10 |
| 39 | SEG_SHIFT            | 12 | 0x10 |
| 40 | SEG_LOW              | 11 | 0x10 |
| 41 | SEG_HIGH             | 10 | 0x10 |
| 42 | SEG_DSB              | 21 | 0x10 |
| 43 | SEG_FM               | 21 | 0x20 |
| 44 | SEG_USB              | 21 | 0x40 |
| 45 | SEG_SAM              | 21 | 0x80 |
| 46 | SEG_DRM              | 22 | 0x01 |
| 47 | SEG_DIG              | 22 | 0x02 |
| 48 | SEG_STEREO           | 22 | 0x04 |
| 49 | SEG_DBM              | 22 | 0x10 |
| 50 | SEG_CW               | 22 | 0x20 |
| 51 | SEG_LSB              | 22 | 0x40 |
| 52 | SEG_AM               | 22 | 0x80 |
| 53 | SEG_SMETER_PLUS20    |  9 | 0x20 |
| 54 | SEG_SMETER_PLUS40    | 15 | 0x20 |
| 55 | SEG_SMETER_PLUS60    | 18 | 0x20 |
| 56 | SEG_SMETER_10        |  8 | 0x20 |
| 57 | SEG_SMETER_20        | 10 | 0x20 |
| 58 | SEG_SMETER_40        | 16 | 0x20 |
| 59 | SEG_SMETER_60        | 19 | 0x20 |
| 60 | SEG_RX               |  0 | 0x04 |
| 61 | SEG_TX               |  0 | 0x08 |
| 62 | SEG_ATT_1            | 31 | 0x04 |
| 63 | SEG_ATT_2            | 31 | 0x01 |
| 64 | SEG_PRE              | 31 | 0x02 |
| 65 | SEG_PRE_1            | 30 | 0x01 |
| 66 | SEG_PRE_2            | 30 | 0x02 |
| 67 | SEG_mW_W             | 27 | 0x01 |
| 68 | SEG_mW_m             | 28 | 0x01 |
| 69 | SEG_W                | 20 | 0x20 |
| 70 | SEG_K                | 25 | 0x01 |
| 71 | SEG_RIT              | 13 | 0x10 |
| 72 | SEG_XIT              | 14 | 0x10 |
| 73 | SEG_W_FM             | 20 | 0x10 |
| 74 | SEG_NR2              |  5 | 0x10 |
| 75 | SEG_NB2              |  6 | 0x10 |
| 76 | SEG_AN2              |  7 | 0x10 |

### S-meter bargraph (IDs 77–91)

| ID | Name        | Byte | Bit  |
| -- | ----------- | ---- | ---- |
| 77 | SMETER_BAR1 |  1 | 0x08 |
| 78 | SMETER_BAR2 |  1 | 0x04 |
| 79 | SMETER_BAR3 |  1 | 0x02 |
| 80 | SMETER_BAR4 | 31 | 0x80 |
| 81 | SMETER_BAR5 | 31 | 0x40 |
| 82 | SMETER_BAR6 | 31 | 0x20 |
| 83 | SMETER_BAR7 | 31 | 0x10 |
| 84 | SMETER_BAR8 | 30 | 0x10 |
| 85 | SMETER_BAR9 | 30 | 0x20 |
| 86 | SMETER_BAR10 | 30 | 0x40 |
| 87 | SMETER_BAR11 | 30 | 0x80 |
| 88 | SMETER_BAR12 | 29 | 0x80 |
| 89 | SMETER_BAR13 | 29 | 0x40 |
| 90 | SMETER_BAR14 | 29 | 0x20 |
| 91 | SMETER_BAR15 | 29 | 0x10 |

### Main 9-digit display (IDs 92–154)

The main display shows frequency in Hz, right-aligned across 9 digits
(digit 9 = 100 MHz, digit 1 = 1 Hz).

Each digit occupies two bytes:

```
high_byte index = 22 − 2 × digit_pos    bits: A=0x01  B=0x02  C=0x04
low_byte  index = 21 − 2 × digit_pos    bits: F=0x01  G=0x02  E=0x04  D=0x08
```

Digit 7-segment encoding (main display):

| Digit | high (A B C) | low (F G E D) |
| ----- | ------------ | ------------- |
| 0     | 0x07         | 0x0D          |
| 1     | 0x06         | 0x00          |
| 2     | 0x03         | 0x0E          |
| 3     | 0x07         | 0x0A          |
| 4     | 0x06         | 0x03          |
| 5     | 0x05         | 0x0B          |
| 6     | 0x05         | 0x0F          |
| 7     | 0x07         | 0x00          |
| 8     | 0x07         | 0x0F          |
| 9     | 0x07         | 0x0B          |
| blank | 0x00         | 0x00          |

Individual segment IDs for digit N (columns A–G):

| Digit | A  | B  | C  | D  | E  | F  | G  |
| ----- | -- | -- | -- | -- | -- | -- | -- |
| 9     | 92 | 93 | 94 | 95 | 96 | 97 | 98 |
| 8     | 99 |100 |101 |102 |103 |104 |105 |
| 7     |106 |107 |108 |109 |110 |111 |112 |
| 6     |113 |114 |115 |116 |117 |118 |119 |
| 5     |120 |121 |122 |123 |124 |125 |126 |
| 4     |127 |128 |129 |130 |131 |132 |133 |
| 3     |134 |135 |136 |137 |138 |139 |140 |
| 2     |141 |142 |143 |144 |145 |146 |147 |
| 1     |148 |149 |150 |151 |152 |153 |154 |

### Small 3-digit display (IDs 155–175)

The small display shows S-meter reading or TX power (3 digits, right-aligned).

Each digit occupies two bytes with **reversed bit ordering** compared to the
main display (hardware quirk):

```
high_byte index = 21 + 2 × digit_pos    bits: A=0x80  B=0x40  C=0x20  D=0x10
low_byte  index = 22 + 2 × digit_pos    bits: E=0x20  F=0x80  G=0x40
```

Digit 7-segment encoding (small display):

| Digit | high (A B C D) | low (E F G) |
| ----- | -------------- | ----------- |
| 0     | 0xF0           | 0xA0        |
| 1     | 0x60           | 0x00        |
| 2     | 0xD0           | 0x60        |
| 3     | 0xF0           | 0x40        |
| 4     | 0x60           | 0xC0        |
| 5     | 0xB0           | 0xC0        |
| 6     | 0xB0           | 0xE0        |
| 7     | 0xE0           | 0x00        |
| 8     | 0xF0           | 0xE0        |
| 9     | 0xF0           | 0xC0        |
| blank | 0x00           | 0x00        |

Individual segment IDs for digit N (columns A–G):

| Digit | A   | B   | C   | D   | E   | F   | G   |
| ----- | --- | --- | --- | --- | --- | --- | --- |
| 3     | 155 | 156 | 157 | 158 | 159 | 160 | 161 |
| 2     | 162 | 163 | 164 | 165 | 166 | 167 | 168 |
| 1     | 169 | 170 | 171 | 172 | 173 | 174 | 175 |

## Confidence

| Area | Confidence |
| --- | --- |
| Device identity | High |
| Endpoint directions | High |
| 64-byte report size | High |
| Input first 9 bytes | High |
| Encoder wrap behavior | High |
| Button masks and active-low polarity | High |
| Output LCD vector structure | High |
| Remaining input bytes `9..63` | Low to medium |
