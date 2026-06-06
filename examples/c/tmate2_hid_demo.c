/*
 * Cross-platform end-to-end demo for the optional hidapi transport.
 *
 * With a TMate 2 connected: opens it, pushes one display frame
 * (backlight + 14.200.000 Hz + USB/RX indicators), and polls input for ~2 s.
 * With no device present: reports that and exits 0, so it is safe to run
 * anywhere.  Builds on Linux, macOS, and Windows (hidapi handles the backend).
 *
 * Build with -DOPENTMATE2_WITH_HIDAPI=ON; CMake adds the `tmate2_hid_demo`
 * target.
 */
#include "opentmate2/opentmate2.h"
#include "opentmate2/opentmate2_hid.h"

#include <stdint.h>
#include <stdio.h>

int main(void)
{
    opentmate2_hid_t *dev = opentmate2_hid_open();
    if (dev == NULL) {
        printf("No TMate 2 found (not connected, or claimed by another app).\n");
        printf("Transport is built and linked; nothing to exercise.\n");
        return 0;
    }

    printf("TMate 2 opened.\n");

    uint8_t lcd[OPENTMATE2_LCD_VECTOR_SIZE];
    opentmate2_lcd_init(lcd);
    opentmate2_set_backlight(lcd, 0, 50, 255);
    opentmate2_set_status(lcd, OPENTMATE2_LED_USB);
    opentmate2_write_main_display(lcd, 14200000u);
    opentmate2_write_small_display(lcd, 9u);
    opentmate2_set_segment(lcd, OPENTMATE2_SEG_USB, 1);
    opentmate2_set_segment(lcd, OPENTMATE2_SEG_RX, 1);
    opentmate2_set_segment(lcd, OPENTMATE2_SEG_HZ, 1);
    opentmate2_set_segment(lcd, OPENTMATE2_SEG_DOT1, 1);
    opentmate2_set_segment(lcd, OPENTMATE2_SEG_DOT2, 1);

    if (opentmate2_hid_write_lcd(dev, lcd, sizeof(lcd)) == OPENTMATE2_OK) {
        printf("Display frame sent: 14.200.000 Hz, USB, RX.\n");
    } else {
        printf("write failed.\n");
    }

    printf("Polling input for ~2 s (turn an encoder / press a key)...\n");
    int got = 0;
    for (int i = 0; i < 40; ++i) {
        opentmate2_input_t in;
        const int r = opentmate2_hid_read(dev, &in, 50);
        if (r > 0) {
            ++got;
            printf("  enc1=%5u  enc2=%5u  enc3=%5u  keys=0x%04x\n",
                   in.enc1, in.enc2, in.enc3, in.keys);
        }
    }
    printf("Received %d input report(s).\n", got);

    opentmate2_hid_close(dev);
    return 0;
}
