#ifndef OPENTMATE2_OPENTMATE2_HID_H
#define OPENTMATE2_OPENTMATE2_HID_H

/*
 * OpenTMate2Lib — optional cross-platform USB HID transport.
 *
 * A thin wrapper over hidapi (https://github.com/libusb/hidapi), which itself
 * abstracts the native HID stacks: hidraw/libusb on Linux, IOKit on macOS, and
 * the Windows HID API on Windows.  One small module therefore covers all three
 * platforms — pair it with the transport-agnostic protocol core in
 * opentmate2.h.
 *
 * This file is built only when the library is configured with
 * -DOPENTMATE2_WITH_HIDAPI=ON; the protocol core stays dependency-free.
 *
 * Report-ID framing (handled here so callers stay in device report bytes):
 *   - Reads: hidapi returns the device's report payload directly (the byte
 *     observed as 0x01 is byte 0), which is exactly what
 *     opentmate2_parse_input_report() expects — no adjustment needed.
 *   - Writes: hidapi's hid_write() expects a leading report-ID byte.  The
 *     TMate 2 OUT report is unnumbered, so a 0 is prepended -> 65 bytes.
 */

#include <stddef.h>
#include "opentmate2/opentmate2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque device handle. */
typedef struct opentmate2_hid opentmate2_hid_t;

/*
 * Open the first connected TMate 2 (VID 0x1721 / PID 0x0614).
 * Initialises hidapi on first use.  Returns NULL if no device is present or on
 * any library/allocation error.  The returned handle must be released with
 * opentmate2_hid_close().
 */
opentmate2_hid_t *opentmate2_hid_open(void);

/* Close the device and free the handle.  Safe to call with NULL. */
void opentmate2_hid_close(opentmate2_hid_t *handle);

/*
 * Read and parse one input report.
 *   timeout_ms : >0 blocks up to that many ms; 0 = non-blocking; <0 = block.
 * Returns:
 *    1  -> a report was read and parsed into *out_input
 *    0  -> timeout, no report available (out_input untouched)
 *   <0 -> error (opentmate2_result_t: NULL args or read failure)
 */
int opentmate2_hid_read(opentmate2_hid_t *handle,
                        opentmate2_input_t *out_input,
                        int timeout_ms);

/*
 * Frame a 44-byte LCDVector into the 64-byte output report and write it to the
 * device (prepending the report-ID byte hidapi requires).
 *   lcd_vector / lcd_len : must be exactly OPENTMATE2_LCD_VECTOR_SIZE bytes.
 * Returns OPENTMATE2_OK, OPENTMATE2_ERROR_NULL, or OPENTMATE2_ERROR_SIZE
 * (the latter also for a write failure).
 */
int opentmate2_hid_write_lcd(opentmate2_hid_t *handle,
                             const uint8_t *lcd_vector,
                             size_t lcd_len);

#ifdef __cplusplus
}
#endif

#endif /* OPENTMATE2_OPENTMATE2_HID_H */
