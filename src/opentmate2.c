#include "opentmate2/opentmate2.h"

#include <string.h>

/* ── Internal types ────────────────────────────────────────────────────── */

/* One entry in the segment lookup table: which byte and which bit. */
typedef struct { uint8_t byte_idx; uint8_t mask; } seg_bit_t;

/* ── Segment lookup table (176 entries) ────────────────────────────────── */
/*
 * Derived from a USB capture made on 2026-06-05 (session_20260605_051711,
 * segments_20260605_051745.csv).  For each segment ID the DLL was called
 * with TMate2SetSegment(lcdbuf, id, 1) and the resulting LCDVector was
 * recorded.  Exactly one bit in bytes 0..31 changed per call.
 *
 * The layout is highly scrambled — there is no sequential correspondence
 * between segment IDs and byte/bit positions.
 */
static const seg_bit_t kSegmentMap[OPENTMATE2_SEGMENT_COUNT] = {
    /* 0  SEG_SMETER_LINE     */ { 2, 0x01},
    /* 1  SEG_SMETER_DB_MINUS */ {28, 0x10},
    /* 2  SEG_CW_PLUS         */ {21, 0x01},
    /* 3  SEG_CW_MINUS        */ {21, 0x02},
    /* 4  SEG_DIG_PLUS        */ {21, 0x04},
    /* 5  SEG_DIG_MINUS       */ {21, 0x08},
    /* 6  SEG_UNDERLINE_9     */ { 4, 0x08},
    /* 7  SEG_UNDERLINE_8     */ { 6, 0x08},
    /* 8  SEG_UNDERLINE_7     */ { 8, 0x08},
    /* 9  SEG_UNDERLINE_6     */ {10, 0x08},
    /* 10 SEG_UNDERLINE_5     */ {12, 0x08},
    /* 11 SEG_UNDERLINE_4     */ {14, 0x08},
    /* 12 SEG_UNDERLINE_3     */ {16, 0x08},
    /* 13 SEG_UNDERLINE_2     */ {18, 0x08},
    /* 14 SEG_UNDERLINE_1     */ {20, 0x08},
    /* 15 SEG_DOT1            */ { 9, 0x10},
    /* 16 SEG_DOT2            */ {15, 0x10},
    /* 17 SEG_HZ              */ {23, 0x01},
    /* 18 SEG_E1              */ { 0, 0x80},
    /* 19 SEG_ERR             */ {19, 0x10},
    /* 20 SEG_E2              */ { 8, 0x10},
    /* 21 SEG_LP              */ { 0, 0x02},
    /* 22 SEG_ATT             */ { 1, 0x01},
    /* 23 SEG_S               */ { 0, 0x10},
    /* 24 SEG_VFO             */ { 0, 0x20},
    /* 25 SEG_NR              */ { 0, 0x40},
    /* 26 SEG_NB              */ { 1, 0x40},
    /* 27 SEG_SMETER_1        */ { 1, 0x10},
    /* 28 SEG_A               */ { 1, 0x20},
    /* 29 SEG_B               */ { 2, 0x20},
    /* 30 SEG_VOL             */ { 1, 0x80},
    /* 31 SEG_SMETER_9        */ { 2, 0x02},
    /* 32 SEG_SMETER_7        */ { 2, 0x04},
    /* 33 SEG_SMETER_5        */ { 2, 0x08},
    /* 34 SEG_SMETER_3        */ { 2, 0x10},
    /* 35 SEG_AN              */ { 2, 0x40},
    /* 36 SEG_RFG             */ { 2, 0x80},
    /* 37 SEG_SQL             */ { 3, 0x10},
    /* 38 SEG_DRV             */ { 4, 0x10},
    /* 39 SEG_SHIFT           */ {12, 0x10},
    /* 40 SEG_LOW             */ {11, 0x10},
    /* 41 SEG_HIGH            */ {10, 0x10},
    /* 42 SEG_DSB             */ {21, 0x10},
    /* 43 SEG_FM              */ {21, 0x20},
    /* 44 SEG_USB             */ {21, 0x40},
    /* 45 SEG_SAM             */ {21, 0x80},
    /* 46 SEG_DRM             */ {22, 0x01},
    /* 47 SEG_DIG             */ {22, 0x02},
    /* 48 SEG_STEREO          */ {22, 0x04},
    /* 49 SEG_DBM             */ {22, 0x10},
    /* 50 SEG_CW              */ {22, 0x20},
    /* 51 SEG_LSB             */ {22, 0x40},
    /* 52 SEG_AM              */ {22, 0x80},
    /* 53 SEG_SMETER_PLUS20   */ { 9, 0x20},
    /* 54 SEG_SMETER_PLUS40   */ {15, 0x20},
    /* 55 SEG_SMETER_PLUS60   */ {18, 0x20},
    /* 56 SEG_SMETER_10       */ { 8, 0x20},
    /* 57 SEG_SMETER_20       */ {10, 0x20},
    /* 58 SEG_SMETER_40       */ {16, 0x20},
    /* 59 SEG_SMETER_60       */ {19, 0x20},
    /* 60 SEG_RX              */ { 0, 0x04},
    /* 61 SEG_TX              */ { 0, 0x08},
    /* 62 SEG_ATT_1           */ {31, 0x04},
    /* 63 SEG_ATT_2           */ {31, 0x01},
    /* 64 SEG_PRE             */ {31, 0x02},
    /* 65 SEG_PRE_1           */ {30, 0x01},
    /* 66 SEG_PRE_2           */ {30, 0x02},
    /* 67 SEG_mW_W            */ {27, 0x01},
    /* 68 SEG_mW_m            */ {28, 0x01},
    /* 69 SEG_W               */ {20, 0x20},
    /* 70 SEG_K               */ {25, 0x01},
    /* 71 SEG_RIT             */ {13, 0x10},
    /* 72 SEG_XIT             */ {14, 0x10},
    /* 73 SEG_W_FM            */ {20, 0x10},
    /* 74 SEG_NR2             */ { 5, 0x10},
    /* 75 SEG_NB2             */ { 6, 0x10},
    /* 76 SEG_AN2             */ { 7, 0x10},

    /* S-meter bargraph — bar 1 is smallest (leftmost), bar 15 is largest */
    /* 77  SMETER_BAR1  */ { 1, 0x08},
    /* 78  SMETER_BAR2  */ { 1, 0x04},
    /* 79  SMETER_BAR3  */ { 1, 0x02},
    /* 80  SMETER_BAR4  */ {31, 0x80},
    /* 81  SMETER_BAR5  */ {31, 0x40},
    /* 82  SMETER_BAR6  */ {31, 0x20},
    /* 83  SMETER_BAR7  */ {31, 0x10},
    /* 84  SMETER_BAR8  */ {30, 0x10},
    /* 85  SMETER_BAR9  */ {30, 0x20},
    /* 86  SMETER_BAR10 */ {30, 0x40},
    /* 87  SMETER_BAR11 */ {30, 0x80},
    /* 88  SMETER_BAR12 */ {29, 0x80},
    /* 89  SMETER_BAR13 */ {29, 0x40},
    /* 90  SMETER_BAR14 */ {29, 0x20},
    /* 91  SMETER_BAR15 */ {29, 0x10},

    /*
     * Main 9-digit display, digit 9 (leftmost / 100 MHz) → digit 1 (units).
     * Each digit occupies two bytes:
     *   high_byte = 22 − 2×digit   bits A=0x01 B=0x02 C=0x04
     *   low_byte  = 21 − 2×digit   bits F=0x01 G=0x02 E=0x04 D=0x08
     * The remaining bits in those bytes belong to indicator segments
     * (underlines, DRV, NR2, etc.) and must not be disturbed.
     */
    /* 92  SEG_MAIN_9A */ { 4, 0x01},
    /* 93  SEG_MAIN_9B */ { 4, 0x02},
    /* 94  SEG_MAIN_9C */ { 4, 0x04},
    /* 95  SEG_MAIN_9D */ { 3, 0x08},
    /* 96  SEG_MAIN_9E */ { 3, 0x04},
    /* 97  SEG_MAIN_9F */ { 3, 0x01},
    /* 98  SEG_MAIN_9G */ { 3, 0x02},
    /* 99  SEG_MAIN_8A */ { 6, 0x01},
    /* 100 SEG_MAIN_8B */ { 6, 0x02},
    /* 101 SEG_MAIN_8C */ { 6, 0x04},
    /* 102 SEG_MAIN_8D */ { 5, 0x08},
    /* 103 SEG_MAIN_8E */ { 5, 0x04},
    /* 104 SEG_MAIN_8F */ { 5, 0x01},
    /* 105 SEG_MAIN_8G */ { 5, 0x02},
    /* 106 SEG_MAIN_7A */ { 8, 0x01},
    /* 107 SEG_MAIN_7B */ { 8, 0x02},
    /* 108 SEG_MAIN_7C */ { 8, 0x04},
    /* 109 SEG_MAIN_7D */ { 7, 0x08},
    /* 110 SEG_MAIN_7E */ { 7, 0x04},
    /* 111 SEG_MAIN_7F */ { 7, 0x01},
    /* 112 SEG_MAIN_7G */ { 7, 0x02},
    /* 113 SEG_MAIN_6A */ {10, 0x01},
    /* 114 SEG_MAIN_6B */ {10, 0x02},
    /* 115 SEG_MAIN_6C */ {10, 0x04},
    /* 116 SEG_MAIN_6D */ { 9, 0x08},
    /* 117 SEG_MAIN_6E */ { 9, 0x04},
    /* 118 SEG_MAIN_6F */ { 9, 0x01},
    /* 119 SEG_MAIN_6G */ { 9, 0x02},
    /* 120 SEG_MAIN_5A */ {12, 0x01},
    /* 121 SEG_MAIN_5B */ {12, 0x02},
    /* 122 SEG_MAIN_5C */ {12, 0x04},
    /* 123 SEG_MAIN_5D */ {11, 0x08},
    /* 124 SEG_MAIN_5E */ {11, 0x04},
    /* 125 SEG_MAIN_5F */ {11, 0x01},
    /* 126 SEG_MAIN_5G */ {11, 0x02},
    /* 127 SEG_MAIN_4A */ {14, 0x01},
    /* 128 SEG_MAIN_4B */ {14, 0x02},
    /* 129 SEG_MAIN_4C */ {14, 0x04},
    /* 130 SEG_MAIN_4D */ {13, 0x08},
    /* 131 SEG_MAIN_4E */ {13, 0x04},
    /* 132 SEG_MAIN_4F */ {13, 0x01},
    /* 133 SEG_MAIN_4G */ {13, 0x02},
    /* 134 SEG_MAIN_3A */ {16, 0x01},
    /* 135 SEG_MAIN_3B */ {16, 0x02},
    /* 136 SEG_MAIN_3C */ {16, 0x04},
    /* 137 SEG_MAIN_3D */ {15, 0x08},
    /* 138 SEG_MAIN_3E */ {15, 0x04},
    /* 139 SEG_MAIN_3F */ {15, 0x01},
    /* 140 SEG_MAIN_3G */ {15, 0x02},
    /* 141 SEG_MAIN_2A */ {18, 0x01},
    /* 142 SEG_MAIN_2B */ {18, 0x02},
    /* 143 SEG_MAIN_2C */ {18, 0x04},
    /* 144 SEG_MAIN_2D */ {17, 0x08},
    /* 145 SEG_MAIN_2E */ {17, 0x04},
    /* 146 SEG_MAIN_2F */ {17, 0x01},
    /* 147 SEG_MAIN_2G */ {17, 0x02},
    /* 148 SEG_MAIN_1A */ {20, 0x01},
    /* 149 SEG_MAIN_1B */ {20, 0x02},
    /* 150 SEG_MAIN_1C */ {20, 0x04},
    /* 151 SEG_MAIN_1D */ {19, 0x08},
    /* 152 SEG_MAIN_1E */ {19, 0x04},
    /* 153 SEG_MAIN_1F */ {19, 0x01},
    /* 154 SEG_MAIN_1G */ {19, 0x02},

    /*
     * Small (S-meter) 3-digit display, digit 3 (hundreds) → digit 1 (units).
     * Each digit occupies two bytes:
     *   high_byte = 21 + 2×digit   bits A=0x80 B=0x40 C=0x20 D=0x10
     *   low_byte  = 22 + 2×digit   bits E=0x20 F=0x80 G=0x40
     * Note the reversed bit order compared to the main display — hardware quirk.
     */
    /* 155 SEG_SMETER_3A */ {27, 0x80},
    /* 156 SEG_SMETER_3B */ {27, 0x40},
    /* 157 SEG_SMETER_3C */ {27, 0x20},
    /* 158 SEG_SMETER_3D */ {27, 0x10},
    /* 159 SEG_SMETER_3E */ {28, 0x20},
    /* 160 SEG_SMETER_3F */ {28, 0x80},
    /* 161 SEG_SMETER_3G */ {28, 0x40},
    /* 162 SEG_SMETER_2A */ {25, 0x80},
    /* 163 SEG_SMETER_2B */ {25, 0x40},
    /* 164 SEG_SMETER_2C */ {25, 0x20},
    /* 165 SEG_SMETER_2D */ {25, 0x10},
    /* 166 SEG_SMETER_2E */ {26, 0x20},
    /* 167 SEG_SMETER_2F */ {26, 0x80},
    /* 168 SEG_SMETER_2G */ {26, 0x40},
    /* 169 SEG_SMETER_1A */ {23, 0x80},
    /* 170 SEG_SMETER_1B */ {23, 0x40},
    /* 171 SEG_SMETER_1C */ {23, 0x20},
    /* 172 SEG_SMETER_1D */ {23, 0x10},
    /* 173 SEG_SMETER_1E */ {24, 0x20},
    /* 174 SEG_SMETER_1F */ {24, 0x80},
    /* 175 SEG_SMETER_1G */ {24, 0x40},
};

/* ── Digit encoding tables ─────────────────────────────────────────────── */
/*
 * Main display — two bytes per digit:
 *   high byte  bits A=0x01 B=0x02 C=0x04  (only bits 0-2 used)
 *   low byte   bits F=0x01 G=0x02 E=0x04 D=0x08  (only bits 0-3 used)
 *
 * Validated against numbers_20260605_051711.csv for digits 0-9.
 */
static const uint8_t kMainDigitHigh[10] = {
    /* 0 */ 0x07,  /* A B C */
    /* 1 */ 0x06,  /* B C */
    /* 2 */ 0x03,  /* A B */
    /* 3 */ 0x07,  /* A B C */
    /* 4 */ 0x06,  /* B C */
    /* 5 */ 0x05,  /* A C */
    /* 6 */ 0x05,  /* A C */
    /* 7 */ 0x07,  /* A B C */
    /* 8 */ 0x07,  /* A B C */
    /* 9 */ 0x07,  /* A B C */
};
static const uint8_t kMainDigitLow[10] = {
    /* 0 */ 0x0D,  /* F E D   (A B C F in standard: ABCDEF → F=0x01 E=0x04 D=0x08) */
    /* 1 */ 0x00,
    /* 2 */ 0x0E,  /* G E D */
    /* 3 */ 0x0A,  /* G D */
    /* 4 */ 0x03,  /* F G */
    /* 5 */ 0x0B,  /* F G D */
    /* 6 */ 0x0F,  /* F G E D */
    /* 7 */ 0x00,
    /* 8 */ 0x0F,  /* F G E D */
    /* 9 */ 0x0B,  /* F G D */
};

/*
 * Small display — two bytes per digit:
 *   high byte  bits A=0x80 B=0x40 C=0x20 D=0x10  (only bits 4-7 used)
 *   low byte   bits E=0x20 F=0x80 G=0x40          (only bits 5-7 used)
 *
 * The bit reversal relative to the main display is a hardware quirk.
 * Validated against numbers_20260605_051711.csv.
 */
static const uint8_t kSmallDigitHigh[10] = {
    /* 0 */ 0xF0,  /* A B C D */
    /* 1 */ 0x60,  /* B C */
    /* 2 */ 0xD0,  /* A B D */
    /* 3 */ 0xF0,  /* A B C D */
    /* 4 */ 0x60,  /* B C */
    /* 5 */ 0xB0,  /* A C D */
    /* 6 */ 0xB0,  /* A C D */
    /* 7 */ 0xE0,  /* A B C */
    /* 8 */ 0xF0,  /* A B C D */
    /* 9 */ 0xF0,  /* A B C D */
};
static const uint8_t kSmallDigitLow[10] = {
    /* 0 */ 0xA0,  /* E F */
    /* 1 */ 0x00,
    /* 2 */ 0x60,  /* E G */
    /* 3 */ 0x40,  /* G */
    /* 4 */ 0xC0,  /* F G */
    /* 5 */ 0xC0,  /* F G */
    /* 6 */ 0xE0,  /* E F G */
    /* 7 */ 0x00,
    /* 8 */ 0xE0,  /* E F G */
    /* 9 */ 0xC0,  /* F G */
};

/* ── Helpers ───────────────────────────────────────────────────────────── */

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}

/* ── Input parsing ─────────────────────────────────────────────────────── */

int opentmate2_parse_input_report(
    const uint8_t *report,
    size_t report_len,
    opentmate2_input_t *out_input)
{
    if (report == NULL || out_input == NULL) return OPENTMATE2_ERROR_NULL;
    if (report_len < OPENTMATE2_INPUT_MIN_SIZE) return OPENTMATE2_ERROR_SIZE;

    out_input->report_id = report[0];
    out_input->enc1 = read_u16_le(report + 1);
    out_input->enc2 = read_u16_le(report + 3);
    out_input->enc3 = read_u16_le(report + 5);
    out_input->keys = read_u16_le(report + 7);
    return OPENTMATE2_OK;
}

int32_t opentmate2_encoder_delta(uint16_t current, uint16_t previous)
{
    int32_t diff = (int32_t)current - (int32_t)previous;
    if (diff >  32767) diff -= 65536;
    else if (diff < -32768) diff += 65536;
    return diff;
}

int opentmate2_key_is_pressed(uint16_t keys, uint16_t mask)
{
    /* Active-low: bit clear means pressed. */
    return (keys & mask) == 0;
}

const char *opentmate2_key_name(uint16_t mask)
{
    switch (mask) {
    case OPENTMATE2_KEY_F1:           return "F1";
    case OPENTMATE2_KEY_F2:           return "F2";
    case OPENTMATE2_KEY_F3:           return "F3";
    case OPENTMATE2_KEY_F4:           return "F4";
    case OPENTMATE2_KEY_F5:           return "F5";
    case OPENTMATE2_KEY_F6:           return "F6";
    case OPENTMATE2_KEY_MAIN_ENCODER: return "MAIN_ENCODER";
    case OPENTMATE2_KEY_ENCODER2:     return "ENCODER2";
    case OPENTMATE2_KEY_ENCODER1:     return "ENCODER1";
    default:                          return "UNKNOWN";
    }
}

/* ── Output report building ────────────────────────────────────────────── */

int opentmate2_build_output_report(
    const uint8_t *lcd_vector,
    size_t lcd_vector_len,
    uint8_t *out_report,
    size_t out_report_len)
{
    if (lcd_vector == NULL || out_report == NULL) return OPENTMATE2_ERROR_NULL;
    if (lcd_vector_len != OPENTMATE2_LCD_VECTOR_SIZE ||
        out_report_len  < OPENTMATE2_REPORT_SIZE) {
        return OPENTMATE2_ERROR_SIZE;
    }

    memcpy(out_report, lcd_vector, OPENTMATE2_LCD_VECTOR_SIZE);
    memset(out_report + OPENTMATE2_LCD_VECTOR_SIZE, 0,
           OPENTMATE2_REPORT_SIZE - OPENTMATE2_LCD_VECTOR_SIZE);
    return OPENTMATE2_OK;
}

/* ── LCDVector initialisation ──────────────────────────────────────────── */

void opentmate2_lcd_init(uint8_t *lcd_vector)
{
    if (lcd_vector == NULL) return;

    memset(lcd_vector, 0, OPENTMATE2_LCD_VECTOR_SIZE);

    /* Timing defaults as captured from hardware (session_20260605_051711). */
    lcd_vector[OPENTMATE2_LCD_CONTRAST]  = 0x28u; /* 40 — good all-round value */
    lcd_vector[OPENTMATE2_LCD_REFRESH]   = 0x28u; /* 40 × 10 ms = 400 ms */
    lcd_vector[OPENTMATE2_LCD_SPEED1]    = 0x01u;
    lcd_vector[OPENTMATE2_LCD_SPEED2]    = 0x05u;
    lcd_vector[OPENTMATE2_LCD_SPEED3]    = 0x0Au;
    lcd_vector[OPENTMATE2_LCD_THR_12]    = 0x0Fu;
    lcd_vector[OPENTMATE2_LCD_THR_23]    = 0x19u;
    lcd_vector[OPENTMATE2_LCD_EVAL_TIME] = 0x0Au;
}

void opentmate2_lcd_clear_display(uint8_t *lcd_vector)
{
    if (lcd_vector == NULL) return;
    memset(lcd_vector, 0, 32);  /* segment area only; bytes 32..43 unchanged */
}

/* ── Segment control ───────────────────────────────────────────────────── */

int opentmate2_set_segment(uint8_t *lcd_vector, int segment_id, int on)
{
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;
    if (segment_id < 0 || (unsigned)segment_id >= OPENTMATE2_SEGMENT_COUNT)
        return OPENTMATE2_ERROR_RANGE;

    const seg_bit_t *e = &kSegmentMap[segment_id];
    if (on)
        lcd_vector[e->byte_idx] |=  e->mask;
    else
        lcd_vector[e->byte_idx] &= (uint8_t)~e->mask;
    return OPENTMATE2_OK;
}

/* ── Number display ────────────────────────────────────────────────────── */

int opentmate2_write_main_display(uint8_t *lcd_vector, uint32_t value_hz)
{
    int d;
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;

    if (value_hz > 999999999u) value_hz = 999999999u;

    /*
     * Walk digit positions 1 (units) through 9 (100 MHz), extracting one
     * decimal digit per iteration.  Blank leading positions.
     *
     * For digit d:
     *   high_byte index = 22 - 2*d  (holds A,B,C in bits 0-2)
     *   low_byte  index = 21 - 2*d  (holds F,G,E,D in bits 0-3)
     *
     * Only the four A-G bit positions are touched; other bits (underlines,
     * mode indicators sharing those bytes) are preserved.
     */
    for (d = 1; d <= 9; d++) {
        uint8_t hi = (uint8_t)(22 - 2 * d);
        uint8_t lo = (uint8_t)(21 - 2 * d);

        /* Clear only the digit segment bits, leave everything else. */
        lcd_vector[hi] &= 0xF8u;  /* mask out bits 0-2 (A B C) */
        lcd_vector[lo] &= 0xF0u;  /* mask out bits 0-3 (F G E D) */

        if (value_hz == 0u && d > 1) {
            continue;  /* blank — already cleared above */
        }

        lcd_vector[hi] |= kMainDigitHigh[value_hz % 10u];
        lcd_vector[lo] |= kMainDigitLow [value_hz % 10u];
        value_hz /= 10u;
    }

    return OPENTMATE2_OK;
}

int opentmate2_write_small_display(uint8_t *lcd_vector, uint32_t value)
{
    int d;
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;

    value %= 1000u;  /* display is 3 digits; wrap larger values */

    /*
     * Walk digit positions 1 (units) through 3 (hundreds).
     *
     * For digit d:
     *   high_byte index = 21 + 2*d  (holds A,B,C,D in bits 4-7)
     *   low_byte  index = 22 + 2*d  (holds E,F,G in bits 5-7)
     *
     * Bit ordering is reversed compared to the main display — hardware quirk.
     * Only the seven segment bits are modified; bits 0-3 of high_byte and
     * bits 0-4 of low_byte are preserved (SEG_HZ, SEG_K, SEG_mW_W live there).
     */
    for (d = 1; d <= 3; d++) {
        uint8_t hi = (uint8_t)(21 + 2 * d);
        uint8_t lo = (uint8_t)(22 + 2 * d);

        /* Clear only the digit segment bits. */
        lcd_vector[hi] &= 0x0Fu;  /* mask out bits 4-7 (A B C D) */
        lcd_vector[lo] &= 0x1Fu;  /* mask out bits 5-7 (E F G)   */

        if (value == 0u && d > 1) {
            continue;  /* blank */
        }

        lcd_vector[hi] |= kSmallDigitHigh[value % 10u];
        lcd_vector[lo] |= kSmallDigitLow [value % 10u];
        value /= 10u;
    }

    return OPENTMATE2_OK;
}

/* ── Status and appearance ─────────────────────────────────────────────── */

int opentmate2_set_status(uint8_t *lcd_vector, uint8_t led_byte)
{
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;
    lcd_vector[OPENTMATE2_LCD_LED_STATUS] = led_byte;
    return OPENTMATE2_OK;
}

int opentmate2_set_backlight(uint8_t *lcd_vector, uint8_t r, uint8_t g, uint8_t b)
{
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;
    lcd_vector[OPENTMATE2_LCD_BACKLIGHT_R] = r;
    lcd_vector[OPENTMATE2_LCD_BACKLIGHT_G] = g;
    lcd_vector[OPENTMATE2_LCD_BACKLIGHT_B] = b;
    return OPENTMATE2_OK;
}

int opentmate2_set_contrast(uint8_t *lcd_vector, uint8_t contrast)
{
    if (lcd_vector == NULL) return OPENTMATE2_ERROR_NULL;
    lcd_vector[OPENTMATE2_LCD_CONTRAST] = contrast;
    return OPENTMATE2_OK;
}
