#ifndef OPENTMATE2_OPENTMATE2_H
#define OPENTMATE2_OPENTMATE2_H

/*
 * OpenTMate2Lib — platform-agnostic C99 library for the
 * ELAD / WoodBoxRadio TMate 2 SDR controller (VID 0x1721, PID 0x0614).
 *
 * Segment IDs and naming follow LCD_SEGMENT_DEF.pas from the original
 * Delphi reference application (Flex-TMate2).  The byte/bit positions were
 * captured from the Windows DLL (TMATE2_DLL.dll) by calling
 * TMate2SetSegment() for every ID and logging the resulting LCDVector via
 * USBPcap (session_20260605_051711, file segments_20260605_051745.csv).
 * Number encoding was cross-validated against numbers_20260605_051711.csv.
 *
 * Protocol overview:
 *   IN  endpoint 0x81 — 64-byte interrupt report; first 9 bytes used.
 *   OUT endpoint 0x01 — 64-byte interrupt report; first 44 bytes are the
 *                       LCDVector, bytes 44..63 are zero padding.
 *
 * LCDVector layout (44 bytes):
 *   [0..31]  LCD segment / display bytes (scrambled bit layout — see map below)
 *   [32]     LED status   (active-high: bit0=USB-connected, bit1=lock)
 *   [33]     Backlight R  (0..255)
 *   [34]     Backlight G  (0..255)
 *   [35]     Backlight B  (0..255)
 *   [36]     Contrast     (0..255; 0x28 = 40 works well)
 *   [37]     LCD refresh time (× 10 ms; 0x28 = 400 ms observed default)
 *   [38]     Encoder speed increment 1
 *   [39]     Encoder speed increment 2
 *   [40]     Encoder speed increment 3
 *   [41]     Speed threshold 1→2
 *   [42]     Speed threshold 2→3
 *   [43]     Speed evaluation time
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Device identity ───────────────────────────────────────────────────── */

#define OPENTMATE2_VENDOR_ID  0x1721u
#define OPENTMATE2_PRODUCT_ID 0x0614u

#define OPENTMATE2_IN_ENDPOINT  0x81u
#define OPENTMATE2_OUT_ENDPOINT 0x01u

#define OPENTMATE2_REPORT_SIZE     64u
#define OPENTMATE2_INPUT_MIN_SIZE   9u
#define OPENTMATE2_LCD_VECTOR_SIZE 44u

/* ── Input key masks (active-low: bit clear = pressed) ─────────────────── */

#define OPENTMATE2_KEY_F1           0x0001u
#define OPENTMATE2_KEY_F2           0x0002u
#define OPENTMATE2_KEY_F3           0x0004u
#define OPENTMATE2_KEY_F4           0x0008u
#define OPENTMATE2_KEY_F5           0x0010u
#define OPENTMATE2_KEY_F6           0x0020u
#define OPENTMATE2_KEY_MAIN_ENCODER 0x0040u  /* main tuning encoder push */
#define OPENTMATE2_KEY_ENCODER2     0x0080u  /* TX-power encoder push */
#define OPENTMATE2_KEY_ENCODER1     0x0100u  /* volume encoder push */
#define OPENTMATE2_KEY_IDLE_MASK    0x01FFu  /* all bits set = no key pressed */

/* ── LCDVector byte offsets (bytes 32..43) ─────────────────────────────── */

#define OPENTMATE2_LCD_LED_STATUS  32u
#define OPENTMATE2_LCD_BACKLIGHT_R 33u
#define OPENTMATE2_LCD_BACKLIGHT_G 34u
#define OPENTMATE2_LCD_BACKLIGHT_B 35u
#define OPENTMATE2_LCD_CONTRAST    36u
#define OPENTMATE2_LCD_REFRESH     37u  /* × 10 ms */
#define OPENTMATE2_LCD_SPEED1      38u
#define OPENTMATE2_LCD_SPEED2      39u
#define OPENTMATE2_LCD_SPEED3      40u
#define OPENTMATE2_LCD_THR_12      41u
#define OPENTMATE2_LCD_THR_23      42u
#define OPENTMATE2_LCD_EVAL_TIME   43u

/* ── LED status bits (byte 32) ─────────────────────────────────────────── */

#define OPENTMATE2_LED_USB  0x01u   /* USB / radio connected */
#define OPENTMATE2_LED_LOCK 0x02u   /* VFO locked */

/* ── Segment IDs (0..175) ──────────────────────────────────────────────── */
/*
 * These IDs are the argument to opentmate2_set_segment().
 * Naming and numbering match LCD_SEGMENT_DEF.pas exactly.
 *
 * Display layout:
 *
 *   Main 9-digit 7-segment display (frequency in Hz, right-aligned):
 *     Digit 9 (leftmost, 100 MHz) … Digit 1 (rightmost, 1 Hz)
 *     Each digit has segments A..G  (SEG_MAIN_<digit><seg>).
 *
 *   Small 3-digit 7-segment display (S-meter reading or TX power):
 *     Digit 3 (leftmost, hundreds) … Digit 1 (rightmost, units)
 *     Each digit has segments A..G  (SEG_SMETER_<digit><seg>).
 *
 * 7-segment layout:
 *        AAA
 *       F   B
 *       F   B
 *        GGG
 *       E   C
 *       E   C
 *        DDD
 */

/* Indicator segments (IDs 0..76) */
#define OPENTMATE2_SEG_SMETER_LINE     0
#define OPENTMATE2_SEG_SMETER_DB_MINUS 1
#define OPENTMATE2_SEG_CW_PLUS         2
#define OPENTMATE2_SEG_CW_MINUS        3
#define OPENTMATE2_SEG_DIG_PLUS        4
#define OPENTMATE2_SEG_DIG_MINUS       5
#define OPENTMATE2_SEG_UNDERLINE_9     6   /* underline below main digit 9 */
#define OPENTMATE2_SEG_UNDERLINE_8     7
#define OPENTMATE2_SEG_UNDERLINE_7     8
#define OPENTMATE2_SEG_UNDERLINE_6     9
#define OPENTMATE2_SEG_UNDERLINE_5    10
#define OPENTMATE2_SEG_UNDERLINE_4    11
#define OPENTMATE2_SEG_UNDERLINE_3    12
#define OPENTMATE2_SEG_UNDERLINE_2    13
#define OPENTMATE2_SEG_UNDERLINE_1    14
#define OPENTMATE2_SEG_DOT1           15   /* decimal point after digit 3 */
#define OPENTMATE2_SEG_DOT2           16   /* decimal point after digit 6 */
#define OPENTMATE2_SEG_HZ             17
#define OPENTMATE2_SEG_E1             18
#define OPENTMATE2_SEG_ERR            19
#define OPENTMATE2_SEG_E2             20
#define OPENTMATE2_SEG_LP             21
#define OPENTMATE2_SEG_ATT            22
#define OPENTMATE2_SEG_S              23
#define OPENTMATE2_SEG_VFO            24
#define OPENTMATE2_SEG_NR             25
#define OPENTMATE2_SEG_NB             26
#define OPENTMATE2_SEG_SMETER_1       27   /* "S1" marker on S-meter scale */
#define OPENTMATE2_SEG_A              28   /* VFO-A label */
#define OPENTMATE2_SEG_B              29   /* VFO-B label */
#define OPENTMATE2_SEG_VOL            30
#define OPENTMATE2_SEG_SMETER_9       31
#define OPENTMATE2_SEG_SMETER_7       32
#define OPENTMATE2_SEG_SMETER_5       33
#define OPENTMATE2_SEG_SMETER_3       34
#define OPENTMATE2_SEG_AN             35
#define OPENTMATE2_SEG_RFG            36
#define OPENTMATE2_SEG_SQL            37
#define OPENTMATE2_SEG_DRV            38
#define OPENTMATE2_SEG_SHIFT          39
#define OPENTMATE2_SEG_LOW            40
#define OPENTMATE2_SEG_HIGH           41
#define OPENTMATE2_SEG_DSB            42
#define OPENTMATE2_SEG_FM             43
#define OPENTMATE2_SEG_USB            44
#define OPENTMATE2_SEG_SAM            45
#define OPENTMATE2_SEG_DRM            46
#define OPENTMATE2_SEG_DIG            47
#define OPENTMATE2_SEG_STEREO         48
#define OPENTMATE2_SEG_DBM            49
#define OPENTMATE2_SEG_CW             50
#define OPENTMATE2_SEG_LSB            51
#define OPENTMATE2_SEG_AM             52
#define OPENTMATE2_SEG_SMETER_PLUS20  53
#define OPENTMATE2_SEG_SMETER_PLUS40  54
#define OPENTMATE2_SEG_SMETER_PLUS60  55
#define OPENTMATE2_SEG_SMETER_10      56
#define OPENTMATE2_SEG_SMETER_20      57
#define OPENTMATE2_SEG_SMETER_40      58
#define OPENTMATE2_SEG_SMETER_60      59
#define OPENTMATE2_SEG_RX             60
#define OPENTMATE2_SEG_TX             61
#define OPENTMATE2_SEG_ATT_1          62
#define OPENTMATE2_SEG_ATT_2          63
#define OPENTMATE2_SEG_PRE            64
#define OPENTMATE2_SEG_PRE_1          65
#define OPENTMATE2_SEG_PRE_2          66
#define OPENTMATE2_SEG_MW_W           67   /* mW/W indicator */
#define OPENTMATE2_SEG_MW_M           68   /* milli-prefix of mW */
#define OPENTMATE2_SEG_W              69
#define OPENTMATE2_SEG_K              70
#define OPENTMATE2_SEG_RIT            71
#define OPENTMATE2_SEG_XIT            72
#define OPENTMATE2_SEG_W_FM           73
#define OPENTMATE2_SEG_NR2            74
#define OPENTMATE2_SEG_NB2            75
#define OPENTMATE2_SEG_AN2            76

/* S-meter bargraph (IDs 77..91) */
#define OPENTMATE2_SMETER_BAR1        77
#define OPENTMATE2_SMETER_BAR2        78
#define OPENTMATE2_SMETER_BAR3        79
#define OPENTMATE2_SMETER_BAR4        80
#define OPENTMATE2_SMETER_BAR5        81
#define OPENTMATE2_SMETER_BAR6        82
#define OPENTMATE2_SMETER_BAR7        83
#define OPENTMATE2_SMETER_BAR8        84
#define OPENTMATE2_SMETER_BAR9        85
#define OPENTMATE2_SMETER_BAR10       86
#define OPENTMATE2_SMETER_BAR11       87
#define OPENTMATE2_SMETER_BAR12       88
#define OPENTMATE2_SMETER_BAR13       89
#define OPENTMATE2_SMETER_BAR14       90
#define OPENTMATE2_SMETER_BAR15       91

/* Main display segments (IDs 92..154) — digit 9 (leftmost) to digit 1 (rightmost) */
#define OPENTMATE2_SEG_MAIN_9A        92
#define OPENTMATE2_SEG_MAIN_9B        93
#define OPENTMATE2_SEG_MAIN_9C        94
#define OPENTMATE2_SEG_MAIN_9D        95
#define OPENTMATE2_SEG_MAIN_9E        96
#define OPENTMATE2_SEG_MAIN_9F        97
#define OPENTMATE2_SEG_MAIN_9G        98
#define OPENTMATE2_SEG_MAIN_8A        99
#define OPENTMATE2_SEG_MAIN_8B       100
#define OPENTMATE2_SEG_MAIN_8C       101
#define OPENTMATE2_SEG_MAIN_8D       102
#define OPENTMATE2_SEG_MAIN_8E       103
#define OPENTMATE2_SEG_MAIN_8F       104
#define OPENTMATE2_SEG_MAIN_8G       105
#define OPENTMATE2_SEG_MAIN_7A       106
#define OPENTMATE2_SEG_MAIN_7B       107
#define OPENTMATE2_SEG_MAIN_7C       108
#define OPENTMATE2_SEG_MAIN_7D       109
#define OPENTMATE2_SEG_MAIN_7E       110
#define OPENTMATE2_SEG_MAIN_7F       111
#define OPENTMATE2_SEG_MAIN_7G       112
#define OPENTMATE2_SEG_MAIN_6A       113
#define OPENTMATE2_SEG_MAIN_6B       114
#define OPENTMATE2_SEG_MAIN_6C       115
#define OPENTMATE2_SEG_MAIN_6D       116
#define OPENTMATE2_SEG_MAIN_6E       117
#define OPENTMATE2_SEG_MAIN_6F       118
#define OPENTMATE2_SEG_MAIN_6G       119
#define OPENTMATE2_SEG_MAIN_5A       120
#define OPENTMATE2_SEG_MAIN_5B       121
#define OPENTMATE2_SEG_MAIN_5C       122
#define OPENTMATE2_SEG_MAIN_5D       123
#define OPENTMATE2_SEG_MAIN_5E       124
#define OPENTMATE2_SEG_MAIN_5F       125
#define OPENTMATE2_SEG_MAIN_5G       126
#define OPENTMATE2_SEG_MAIN_4A       127
#define OPENTMATE2_SEG_MAIN_4B       128
#define OPENTMATE2_SEG_MAIN_4C       129
#define OPENTMATE2_SEG_MAIN_4D       130
#define OPENTMATE2_SEG_MAIN_4E       131
#define OPENTMATE2_SEG_MAIN_4F       132
#define OPENTMATE2_SEG_MAIN_4G       133
#define OPENTMATE2_SEG_MAIN_3A       134
#define OPENTMATE2_SEG_MAIN_3B       135
#define OPENTMATE2_SEG_MAIN_3C       136
#define OPENTMATE2_SEG_MAIN_3D       137
#define OPENTMATE2_SEG_MAIN_3E       138
#define OPENTMATE2_SEG_MAIN_3F       139
#define OPENTMATE2_SEG_MAIN_3G       140
#define OPENTMATE2_SEG_MAIN_2A       141
#define OPENTMATE2_SEG_MAIN_2B       142
#define OPENTMATE2_SEG_MAIN_2C       143
#define OPENTMATE2_SEG_MAIN_2D       144
#define OPENTMATE2_SEG_MAIN_2E       145
#define OPENTMATE2_SEG_MAIN_2F       146
#define OPENTMATE2_SEG_MAIN_2G       147
#define OPENTMATE2_SEG_MAIN_1A       148
#define OPENTMATE2_SEG_MAIN_1B       149
#define OPENTMATE2_SEG_MAIN_1C       150
#define OPENTMATE2_SEG_MAIN_1D       151
#define OPENTMATE2_SEG_MAIN_1E       152
#define OPENTMATE2_SEG_MAIN_1F       153
#define OPENTMATE2_SEG_MAIN_1G       154

/* Small (S-meter) display segments (IDs 155..175) — digit 3 (hundreds) to 1 (units) */
#define OPENTMATE2_SEG_SMETER_3A     155
#define OPENTMATE2_SEG_SMETER_3B     156
#define OPENTMATE2_SEG_SMETER_3C     157
#define OPENTMATE2_SEG_SMETER_3D     158
#define OPENTMATE2_SEG_SMETER_3E     159
#define OPENTMATE2_SEG_SMETER_3F     160
#define OPENTMATE2_SEG_SMETER_3G     161
#define OPENTMATE2_SEG_SMETER_2A     162
#define OPENTMATE2_SEG_SMETER_2B     163
#define OPENTMATE2_SEG_SMETER_2C     164
#define OPENTMATE2_SEG_SMETER_2D     165
#define OPENTMATE2_SEG_SMETER_2E     166
#define OPENTMATE2_SEG_SMETER_2F     167
#define OPENTMATE2_SEG_SMETER_2G     168
#define OPENTMATE2_SEG_SMETER_1A     169
#define OPENTMATE2_SEG_SMETER_1B     170
#define OPENTMATE2_SEG_SMETER_1C     171
#define OPENTMATE2_SEG_SMETER_1D     172
#define OPENTMATE2_SEG_SMETER_1E     173
#define OPENTMATE2_SEG_SMETER_1F     174
#define OPENTMATE2_SEG_SMETER_1G     175

#define OPENTMATE2_SEGMENT_COUNT     176u

/* ── Return codes ──────────────────────────────────────────────────────── */

typedef enum opentmate2_result {
    OPENTMATE2_OK          =  0,
    OPENTMATE2_ERROR_NULL  = -1,
    OPENTMATE2_ERROR_SIZE  = -2,
    OPENTMATE2_ERROR_RANGE = -3   /* segment ID out of range */
} opentmate2_result_t;

/* ── Input report struct ───────────────────────────────────────────────── */

typedef struct opentmate2_input {
    uint8_t  report_id;  /* always 0x01 */
    uint16_t enc1;       /* main tuning encoder, absolute wrapping uint16 */
    uint16_t enc2;       /* TX-power encoder */
    uint16_t enc3;       /* volume encoder */
    uint16_t keys;       /* active-low bitmask; idle = 0x01FF */
} opentmate2_input_t;

/* ── Input parsing ─────────────────────────────────────────────────────── */

/*
 * Parse a raw 64-byte (or longer) HID input report into opentmate2_input_t.
 * Returns OPENTMATE2_OK on success.
 */
int opentmate2_parse_input_report(
    const uint8_t *report,
    size_t         report_len,
    opentmate2_input_t *out_input);

/*
 * Compute a wrap-corrected encoder delta: current − previous, adjusted so
 * the result is in [−32768, +32767].  Use this on successive enc1/enc2/enc3
 * readings to get a signed step count per poll.
 */
int32_t opentmate2_encoder_delta(uint16_t current, uint16_t previous);

/*
 * Returns non-zero when the key identified by mask is pressed.
 * TMate 2 keys are active-low: a cleared bit indicates pressed.
 */
int opentmate2_key_is_pressed(uint16_t keys, uint16_t mask);

/* Returns a human-readable name for a key mask, e.g. "F1" or "MAIN_ENCODER". */
const char *opentmate2_key_name(uint16_t mask);

/* ── Output report building ────────────────────────────────────────────── */

/*
 * Copy lcd_vector (44 bytes) into out_report (64 bytes), zeroing the 20
 * bytes of padding.  out_report is ready to hand to hid_write() or
 * equivalent.  Returns OPENTMATE2_OK on success.
 */
int opentmate2_build_output_report(
    const uint8_t *lcd_vector,
    size_t         lcd_vector_len,
    uint8_t       *out_report,
    size_t         out_report_len);

/* ── LCDVector initialisation ──────────────────────────────────────────── */

/*
 * Zero the entire 44-byte lcd_vector and fill bytes 36..43 with the timing
 * defaults observed during hardware capture:
 *
 *   contrast      = 0x28 (40)
 *   refresh time  = 0x28 → 400 ms
 *   speed1..3     = 1, 5, 10
 *   threshold 1→2 = 15
 *   threshold 2→3 = 25
 *   eval time     = 10
 *
 * Backlight and LED status are left at 0 — call opentmate2_set_backlight()
 * and opentmate2_set_status() to configure them.
 */
void opentmate2_lcd_init(uint8_t *lcd_vector);

/*
 * Zero only the 32 segment/display bytes (bytes 0..31), leaving bytes
 * 32..43 (LED, backlight, contrast, timing) unchanged.  Use this to blank
 * the display before writing a fresh frame.
 */
void opentmate2_lcd_clear_display(uint8_t *lcd_vector);

/* ── Segment control ───────────────────────────────────────────────────── */

/*
 * Set or clear one segment in lcd_vector.
 *
 *   segment_id : one of the OPENTMATE2_SEG_* / OPENTMATE2_SMETER_BAR*
 *                constants (0..175).
 *   on         : non-zero = turn segment on, 0 = turn segment off.
 *
 * Returns OPENTMATE2_OK, OPENTMATE2_ERROR_NULL, or OPENTMATE2_ERROR_RANGE.
 *
 * Note: opentmate2_write_main_display() and opentmate2_write_small_display()
 * also modify segment bits.  To avoid conflicts, call those first, then
 * layer indicator segments on top.
 */
int opentmate2_set_segment(uint8_t *lcd_vector, int segment_id, int on);

/* ── Number display ────────────────────────────────────────────────────── */

/*
 * Write a frequency (in Hz) to the 9-digit main display.
 *
 * value_hz is clamped to [0, 999 999 999].  Digits are right-aligned;
 * leading positions are blanked (not shown as '0').  Only the seven
 * segment bits (A–G) of each digit pair are modified — adjacent indicator
 * bits (underlines, DRV, etc.) are preserved.
 *
 * Main display byte pairs by digit position (digit 1 = units, digit 9 =
 * 100 MHz):
 *
 *   Digit  low_byte  high_byte   layout inside low_byte   layout inside high_byte
 *     1       19        20       F=b0 G=b1 E=b2 D=b3      A=b0 B=b1 C=b2
 *     2       17        18
 *     3       15        16
 *     4       13        14
 *     5       11        12
 *     6        9        10
 *     7        7         8
 *     8        5         6
 *     9        3         4
 *
 * Example: 14 200 000 Hz  →  "14200000" right-aligned, digit 9 blank.
 *
 * Returns OPENTMATE2_OK or OPENTMATE2_ERROR_NULL.
 */
int opentmate2_write_main_display(uint8_t *lcd_vector, uint32_t value_hz);

/*
 * Write a value (0..999) to the 3-digit small (S-meter / power) display.
 *
 * Values ≥ 1000 are displayed as value % 1000.  Digits are right-aligned;
 * leading positions are blanked.
 *
 * Small display byte pairs by digit position (digit 1 = units, digit 3 =
 * hundreds):
 *
 *   Digit  high_byte  low_byte   layout inside high_byte    layout inside low_byte
 *     1       23        24       A=b7 B=b6 C=b5 D=b4        E=b5 F=b7 G=b6
 *     2       25        26
 *     3       27        28
 *
 * The bit layout of the small display differs from the main display
 * (bits are reversed / shifted) — this is a hardware quirk of the TMate 2.
 *
 * Returns OPENTMATE2_OK or OPENTMATE2_ERROR_NULL.
 */
int opentmate2_write_small_display(uint8_t *lcd_vector, uint32_t value);

/* ── Status and appearance ─────────────────────────────────────────────── */

/*
 * Set the LED status byte (byte 32).
 *   led_byte : bitmask of OPENTMATE2_LED_* flags.
 * Returns OPENTMATE2_OK or OPENTMATE2_ERROR_NULL.
 */
int opentmate2_set_status(uint8_t *lcd_vector, uint8_t led_byte);

/*
 * Set the RGB backlight (bytes 33..35).  Values are 0..255 per channel.
 * Returns OPENTMATE2_OK or OPENTMATE2_ERROR_NULL.
 */
int opentmate2_set_backlight(uint8_t *lcd_vector, uint8_t r, uint8_t g, uint8_t b);

/*
 * Set the display contrast (byte 36).  0x28 (40) is the recommended default.
 * Returns OPENTMATE2_OK or OPENTMATE2_ERROR_NULL.
 */
int opentmate2_set_contrast(uint8_t *lcd_vector, uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif /* OPENTMATE2_OPENTMATE2_H */
