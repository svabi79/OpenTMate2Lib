#include "opentmate2/opentmate2.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

static void print_pressed(uint16_t keys, uint16_t mask)
{
    if (opentmate2_key_is_pressed(keys, mask)) {
        std::cout << "Pressed: " << opentmate2_key_name(mask) << "\n";
    }
}

int main()
{
    uint8_t report[OPENTMATE2_REPORT_SIZE] = {
        0x01, 0x4A, 0x00, 0x0B, 0x00, 0xFE, 0xFF, 0xFF, 0x01
    };

    opentmate2_input_t input {};
    const int rc = opentmate2_parse_input_report(report, sizeof(report), &input);
    if (rc != OPENTMATE2_OK) {
        std::cerr << "Failed to parse input report: " << rc << "\n";
        return 1;
    }

    std::cout << "ReportId: " << unsigned(input.report_id) << "\n";
    std::cout << "Enc1: " << input.enc1 << "\n";
    std::cout << "Enc2: " << input.enc2 << "\n";
    std::cout << "Enc3: " << input.enc3 << "\n";
    std::cout << "Keys: 0x" << std::hex << std::setw(4)
              << std::setfill('0') << input.keys << std::dec << "\n";

    print_pressed(input.keys, OPENTMATE2_KEY_F1);
    print_pressed(input.keys, OPENTMATE2_KEY_F2);
    print_pressed(input.keys, OPENTMATE2_KEY_F3);
    print_pressed(input.keys, OPENTMATE2_KEY_F4);
    print_pressed(input.keys, OPENTMATE2_KEY_F5);
    print_pressed(input.keys, OPENTMATE2_KEY_F6);
    print_pressed(input.keys, OPENTMATE2_KEY_MAIN_ENCODER);
    print_pressed(input.keys, OPENTMATE2_KEY_ENCODER2);
    print_pressed(input.keys, OPENTMATE2_KEY_ENCODER1);

    uint8_t lcd_vector[OPENTMATE2_LCD_VECTOR_SIZE] = {};
    lcd_vector[33] = 0x20;
    lcd_vector[34] = 0x80;
    lcd_vector[35] = 0x20;
    lcd_vector[36] = 0x28;

    uint8_t out_report[OPENTMATE2_REPORT_SIZE] = {};
    if (opentmate2_build_output_report(
            lcd_vector, sizeof(lcd_vector),
            out_report, sizeof(out_report)) != OPENTMATE2_OK) {
        std::cerr << "Failed to build output report\n";
        return 1;
    }

    std::cout << "Output report bytes: " << sizeof(out_report) << "\n";
    return 0;
}

