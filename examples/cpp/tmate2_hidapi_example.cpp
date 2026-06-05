/*
    Optional hidapi integration sketch.

    This file is intentionally not part of the default CMake build because
    hidapi availability differs by platform. It shows the expected control flow
    once hidapi is installed and linked by the host application.
*/

#include "opentmate2/opentmate2.h"

#include <hidapi/hidapi.h>

#include <array>
#include <iostream>

int main()
{
    if (hid_init() != 0) {
        std::cerr << "hid_init failed\n";
        return 1;
    }

    hid_device *device = hid_open(OPENTMATE2_VENDOR_ID, OPENTMATE2_PRODUCT_ID, nullptr);
    if (device == nullptr) {
        std::cerr << "TMate 2 not found\n";
        hid_exit();
        return 1;
    }

    std::array<unsigned char, OPENTMATE2_REPORT_SIZE> report {};
    while (true) {
        const int n = hid_read_timeout(device, report.data(), report.size(), -1);
        if (n < 0) {
            std::cerr << "hid_read failed\n";
            break;
        }

        opentmate2_input_t input {};
        if (opentmate2_parse_input_report(report.data(), static_cast<size_t>(n), &input) == OPENTMATE2_OK) {
            std::cout << "enc=(" << input.enc1 << "," << input.enc2 << "," << input.enc3
                      << ") keys=0x" << std::hex << input.keys << std::dec << "\n";
        }
    }

    hid_close(device);
    hid_exit();
    return 0;
}
