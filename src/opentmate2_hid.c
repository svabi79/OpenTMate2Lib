#include "opentmate2/opentmate2_hid.h"

/* hidapi's header lives at <hidapi/hidapi.h> on most installs (Linux
 * libhidapi-dev, Windows), but Homebrew/macOS pkg-config points directly at
 * the hidapi dir, exposing it as <hidapi.h>. Accept either. */
#if defined(__has_include)
#  if __has_include(<hidapi/hidapi.h>)
#    include <hidapi/hidapi.h>
#  else
#    include <hidapi.h>
#  endif
#else
#  include <hidapi/hidapi.h>
#endif

#include <stdlib.h>
#include <string.h>

struct opentmate2_hid {
    hid_device *dev;
};

opentmate2_hid_t *opentmate2_hid_open(void)
{
    if (hid_init() != 0) {
        return NULL;
    }

    hid_device *dev = hid_open(OPENTMATE2_VENDOR_ID, OPENTMATE2_PRODUCT_ID, NULL);
    if (dev == NULL) {
        return NULL;
    }

    opentmate2_hid_t *handle =
        (opentmate2_hid_t *)calloc(1, sizeof(*handle));
    if (handle == NULL) {
        hid_close(dev);
        return NULL;
    }
    handle->dev = dev;
    return handle;
}

void opentmate2_hid_close(opentmate2_hid_t *handle)
{
    if (handle == NULL) {
        return;
    }
    if (handle->dev != NULL) {
        hid_close(handle->dev);
    }
    free(handle);
}

int opentmate2_hid_read(opentmate2_hid_t *handle,
                        opentmate2_input_t *out_input,
                        int timeout_ms)
{
    if (handle == NULL || handle->dev == NULL || out_input == NULL) {
        return OPENTMATE2_ERROR_NULL;
    }

    uint8_t buf[OPENTMATE2_REPORT_SIZE];
    const int n = hid_read_timeout(handle->dev, buf, sizeof(buf), timeout_ms);
    if (n < 0) {
        return OPENTMATE2_ERROR_SIZE;  /* read failure */
    }
    if (n == 0) {
        return 0;  /* timeout */
    }

    /* hidapi already strips the leading report-ID byte for unnumbered reports,
     * so buf[0] is the device byte observed as 0x01 — parse directly. */
    if (opentmate2_parse_input_report(buf, (size_t)n, out_input) != OPENTMATE2_OK) {
        return OPENTMATE2_ERROR_SIZE;
    }
    return 1;
}

int opentmate2_hid_write_lcd(opentmate2_hid_t *handle,
                             const uint8_t *lcd_vector,
                             size_t lcd_len)
{
    if (handle == NULL || handle->dev == NULL || lcd_vector == NULL) {
        return OPENTMATE2_ERROR_NULL;
    }

    uint8_t report[OPENTMATE2_REPORT_SIZE];
    const int rc = opentmate2_build_output_report(
        lcd_vector, lcd_len, report, sizeof(report));
    if (rc != OPENTMATE2_OK) {
        return rc;
    }

    /* hidapi's hid_write() expects a leading report-ID byte.  The TMate 2 OUT
     * report is unnumbered, so prepend 0 -> 65 bytes. */
    uint8_t out[OPENTMATE2_REPORT_SIZE + 1];
    out[0] = 0x00;
    memcpy(out + 1, report, OPENTMATE2_REPORT_SIZE);

    const int written = hid_write(handle->dev, out, sizeof(out));
    if (written < 0) {
        return OPENTMATE2_ERROR_SIZE;
    }
    return OPENTMATE2_OK;
}
