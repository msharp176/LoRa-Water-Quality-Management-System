#include "main.h"

/**
 * @brief Struct containing all data necessary to define a single I2C instance within the HAL.
 */
typedef struct i2c_context_s {
    i2c_inst_t* inst;
    uint        sda;
    uint        scl;
    uint        baud;
} i2c_context_t;

i2c_context_t context_i2c_1 = {
    .baud = 100000,
    .inst = i2c1,
    .scl = GP27,
    .sda = GP26
};

static uint16_t wiper_positions[6] = {0, 50, 100, 150, 200, 256};

int main() {

    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

    printf("Initializing hardware [HAL]...");
    i2c_init_hal(&context_i2c_1);   // 

    printf("DONE\n");

    int wiper_position_index = 0;
    uint16_t wiper_position;

    while (true) {

        wiper_position = wiper_positions[wiper_position_index];

        printf("To advance the wiper position, press 't'. Next position: %d\n", wiper_position);

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        usb_console_write_hal("Setting...");

        // Set the wiper position
        mcp4651_set_wiper(&context_digipot_offset, MCP4651_WIPER_A, wiper_position);

        printf("DONE\n\n");

        wiper_position_index++;

        // Wrap around the index if it is out of bounds.
        if (wiper_position_index > 5) wiper_position_index = 0;
    }
}