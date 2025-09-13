#include "main.h"

static uint16_t wiper_positions[6] = {0, 50, 100, 150, 200, 256};

int main() {

    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

    printf("Initializing hardware [HAL]...");
    // This example will use I2C0 on the default SDA and SCL pins (GP4, GP5 on a Pico)
    i2c_init_hal(&context_i2c_1);

    // Make the I2C pins available to picotool
    //bi_decl(bi_2pins_with_func(GP4, GP5, GPIO_FUNC_I2C));
    printf("DONE\n");

    int wiper_position_index = 0;
    uint16_t wiper_position;

    while (true) {

        wiper_position = wiper_positions[wiper_position_index];

        printf("To advance the wiper position, press 't'. Next position: %d\n", wiper_position);

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        usb_console_write_hal("Setting...");

        mcp4651_set_wiper(&context_digipot_offset, MCP4651_WIPER_A, wiper_position);

        printf("DONE\n\n");

        wiper_position_index++;

        if (wiper_position_index > 5) wiper_position_index = 0;
    }
}