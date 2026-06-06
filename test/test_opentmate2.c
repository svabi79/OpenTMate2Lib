/*
 * OpenTMate2Lib unit tests — dependency-free, plain C99.
 *
 * Uses an always-on CHECK() macro rather than assert(): asserts are compiled
 * out under NDEBUG (Release), which would make the suite a silent no-op in CI.
 * CHECK() always evaluates its condition and main() returns non-zero if any
 * check failed.
 */
#include "opentmate2/opentmate2.h"

#include <stdio.h>
#include <string.h>

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

/* ── Input parsing ─────────────────────────────────────────────────────── */

static void test_parse_input(void)
{
    /* report_id=0x01, enc1=0x0102, enc2=0x0304, enc3=0xFFFE, keys=0x01FF */
    const uint8_t report[9] = {
        0x01, 0x02, 0x01, 0x04, 0x03, 0xFE, 0xFF, 0xFF, 0x01
    };
    opentmate2_input_t in;
    CHECK(opentmate2_parse_input_report(report, sizeof report, &in) == OPENTMATE2_OK);
    CHECK(in.report_id == 0x01);
    CHECK(in.enc1 == 0x0102);
    CHECK(in.enc2 == 0x0304);
    CHECK(in.enc3 == 0xFFFE);
    CHECK(in.keys == 0x01FF);

    /* Error paths. */
    CHECK(opentmate2_parse_input_report(NULL, 9, &in) == OPENTMATE2_ERROR_NULL);
    CHECK(opentmate2_parse_input_report(report, 8, &in) == OPENTMATE2_ERROR_SIZE);
}

/* ── Encoder wrap-around delta ─────────────────────────────────────────── */

static void test_encoder_delta(void)
{
    CHECK(opentmate2_encoder_delta(10, 7) == 3);
    CHECK(opentmate2_encoder_delta(7, 10) == -3);
    CHECK(opentmate2_encoder_delta(0, 0) == 0);
    /* Forward wrap: 0x0002 after 0xFFFF is +3, not -65533. */
    CHECK(opentmate2_encoder_delta(0x0002, 0xFFFF) == 3);
    /* Backward wrap: 0xFFFF after 0x0002 is -3. */
    CHECK(opentmate2_encoder_delta(0xFFFF, 0x0002) == -3);
}

/* ── Active-low keys ───────────────────────────────────────────────────── */

static void test_keys(void)
{
    /* Idle = all bits set → nothing pressed. */
    CHECK(!opentmate2_key_is_pressed(OPENTMATE2_KEY_IDLE_MASK, OPENTMATE2_KEY_F1));
    /* F1 pressed = its bit cleared. */
    uint16_t keys = (uint16_t)(OPENTMATE2_KEY_IDLE_MASK & ~OPENTMATE2_KEY_F1);
    CHECK(opentmate2_key_is_pressed(keys, OPENTMATE2_KEY_F1));
    CHECK(!opentmate2_key_is_pressed(keys, OPENTMATE2_KEY_F2));
    CHECK(strcmp(opentmate2_key_name(OPENTMATE2_KEY_F1), "F1") == 0);
    CHECK(strcmp(opentmate2_key_name(0xAAAA), "UNKNOWN") == 0);
}

/* ── Output report build ───────────────────────────────────────────────── */

static void test_build_output(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    uint8_t report[OPENTMATE2_REPORT_SIZE];
    size_t i;
    for (i = 0; i < sizeof lcd; i++) lcd[i] = (uint8_t)(i + 1);

    CHECK(opentmate2_build_output_report(lcd, sizeof lcd, report, sizeof report) == OPENTMATE2_OK);
    CHECK(memcmp(report, lcd, OPENTMATE2_LCD_VECTOR_SIZE) == 0);
    for (i = OPENTMATE2_LCD_VECTOR_SIZE; i < OPENTMATE2_REPORT_SIZE; i++)
        CHECK(report[i] == 0);  /* padding zeroed */

    CHECK(opentmate2_build_output_report(lcd, 43, report, sizeof report) == OPENTMATE2_ERROR_SIZE);
    CHECK(opentmate2_build_output_report(lcd, sizeof lcd, report, 63) == OPENTMATE2_ERROR_SIZE);
}

/* ── LCD init defaults ─────────────────────────────────────────────────── */

static void test_lcd_init(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    memset(lcd, 0xAA, sizeof lcd);
    opentmate2_lcd_init(lcd);
    CHECK(lcd[OPENTMATE2_LCD_CONTRAST]  == 0x28);
    CHECK(lcd[OPENTMATE2_LCD_REFRESH]   == 0x28);
    CHECK(lcd[OPENTMATE2_LCD_SPEED1]    == 0x01);
    CHECK(lcd[OPENTMATE2_LCD_SPEED2]    == 0x05);
    CHECK(lcd[OPENTMATE2_LCD_SPEED3]    == 0x0A);
    CHECK(lcd[OPENTMATE2_LCD_THR_12]    == 0x0F);
    CHECK(lcd[OPENTMATE2_LCD_THR_23]    == 0x19);
    CHECK(lcd[OPENTMATE2_LCD_EVAL_TIME] == 0x0A);
    CHECK(lcd[0] == 0 && lcd[31] == 0);  /* segment area cleared */
}

/* ── Segment set/clear + range ─────────────────────────────────────────── */

static void test_set_segment(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    memset(lcd, 0, sizeof lcd);

    /* SEG_RX maps to byte 0, mask 0x04 (per capture). */
    CHECK(opentmate2_set_segment(lcd, OPENTMATE2_SEG_RX, 1) == OPENTMATE2_OK);
    CHECK(lcd[0] == 0x04);
    CHECK(opentmate2_set_segment(lcd, OPENTMATE2_SEG_RX, 0) == OPENTMATE2_OK);
    CHECK(lcd[0] == 0x00);

    /* Range checks. */
    CHECK(opentmate2_set_segment(lcd, -1, 1) == OPENTMATE2_ERROR_RANGE);
    CHECK(opentmate2_set_segment(lcd, OPENTMATE2_SEGMENT_COUNT, 1) == OPENTMATE2_ERROR_RANGE);
    CHECK(opentmate2_set_segment(NULL, OPENTMATE2_SEG_RX, 1) == OPENTMATE2_ERROR_NULL);
}

/* ── Main display: known frequency + indicator-bit preservation ────────── */

static void test_main_display(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    memset(lcd, 0, sizeof lcd);

    /* Light an indicator that shares a digit byte: SEG_RIT = byte 13, 0x10. */
    CHECK(opentmate2_set_segment(lcd, OPENTMATE2_SEG_RIT, 1) == OPENTMATE2_OK);
    CHECK((lcd[13] & 0x10) != 0);

    /* Write 14 200 000 Hz, then re-write a different value; the RIT bit in
     * byte 13 must survive both writes (display touches only bits 0-3). */
    CHECK(opentmate2_write_main_display(lcd, 14200000u) == OPENTMATE2_OK);
    CHECK((lcd[13] & 0x10) != 0);
    CHECK(opentmate2_write_main_display(lcd, 7000000u) == OPENTMATE2_OK);
    CHECK((lcd[13] & 0x10) != 0);

    /* value 0 blanks all but digit 1 (which shows '0'). Digit 1 = bytes 19/20. */
    memset(lcd, 0, sizeof lcd);
    CHECK(opentmate2_write_main_display(lcd, 0u) == OPENTMATE2_OK);
    /* '0' on digit 1: high byte 20 bits A B C = 0x07, low byte 19 = F E D = 0x0D. */
    CHECK((lcd[20] & 0x07) == 0x07);
    CHECK((lcd[19] & 0x0F) == 0x0D);
    /* Digit 2 (bytes 17/18) must be blank. */
    CHECK((lcd[18] & 0x07) == 0x00);
    CHECK((lcd[17] & 0x0F) == 0x00);

    /* Clamp: out-of-range frequency must not crash and stays bounded. */
    CHECK(opentmate2_write_main_display(lcd, 0xFFFFFFFFu) == OPENTMATE2_OK);
}

/* ── Small display: known value + reversed bit layout ──────────────────── */

static void test_small_display(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    memset(lcd, 0, sizeof lcd);

    /* Value 0: digit 1 (high byte 23, low byte 24) shows '0' = high 0xF0, low 0xA0. */
    CHECK(opentmate2_write_small_display(lcd, 0u) == OPENTMATE2_OK);
    CHECK((lcd[23] & 0xF0) == 0xF0);
    CHECK((lcd[24] & 0xE0) == 0xA0);

    /* >= 1000 wraps modulo 1000. */
    CHECK(opentmate2_write_small_display(lcd, 1234u) == OPENTMATE2_OK); /* shows 234 */
    /* SEG_HZ lives in byte 23 bit0; small display must not disturb low bits. */
    memset(lcd, 0, sizeof lcd);
    CHECK(opentmate2_set_segment(lcd, OPENTMATE2_SEG_HZ, 1) == OPENTMATE2_OK);
    CHECK((lcd[23] & 0x01) != 0);
    CHECK(opentmate2_write_small_display(lcd, 59u) == OPENTMATE2_OK);
    CHECK((lcd[23] & 0x01) != 0);  /* HZ preserved */
}

/* ── Status / backlight / contrast ─────────────────────────────────────── */

static void test_status_backlight(void)
{
    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    memset(lcd, 0, sizeof lcd);
    CHECK(opentmate2_set_status(lcd, OPENTMATE2_LED_USB | OPENTMATE2_LED_LOCK) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_LED_STATUS] == 0x03);
    CHECK(opentmate2_set_click(lcd, 1) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_LED_STATUS] == 0x07);
    CHECK(opentmate2_set_click(lcd, 0) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_LED_STATUS] == 0x03);
    CHECK(opentmate2_toggle_click(lcd) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_LED_STATUS] == 0x07);
    CHECK(opentmate2_toggle_click(lcd) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_LED_STATUS] == 0x03);
    CHECK(opentmate2_set_backlight(lcd, 10, 20, 30) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_BACKLIGHT_R] == 10);
    CHECK(lcd[OPENTMATE2_LCD_BACKLIGHT_G] == 20);
    CHECK(lcd[OPENTMATE2_LCD_BACKLIGHT_B] == 30);
    CHECK(opentmate2_set_contrast(lcd, 0x28) == OPENTMATE2_OK);
    CHECK(lcd[OPENTMATE2_LCD_CONTRAST] == 0x28);
}

int main(void)
{
    test_parse_input();
    test_encoder_delta();
    test_keys();
    test_build_output();
    test_lcd_init();
    test_set_segment();
    test_main_display();
    test_small_display();
    test_status_backlight();
    if (g_failures == 0) {
        printf("All OpenTMate2Lib tests passed.\n");
        return 0;
    }
    fprintf(stderr, "%d OpenTMate2Lib check(s) failed.\n", g_failures);
    return 1;
}
