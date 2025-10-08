/******************************************************************************************************************** 
*   
*   @file main.c
*
*   @brief Main Driver file for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "main.h"

#define DIGIPOT_STEP_TIME_MS 5

#define STATUS_LED GP14
#define RX_LED GP15

#define RX

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Variables

char * gettysburg_address_trunc = "Four score and seven years ago our fathers brought forth on this continent a new nation.";
char * gettysburg_address = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth.";

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * To transmit data:
 * Initialize radio
 * Initialize TX mode
 * Attach the radio interrupt
 * Transmit data
 * Wait for interrupt
 */

static bool tx_go = false;

void isr_set_tx_go(void) {
    tx_go = true;
}

void print_banner(void) {
    printf("-- LoRa Water Quality Management System Sensor Node --\n");
    printf("Version 0.1, compiled %s, %s\n\n", __DATE__, __TIME__);
}

const gpio_driven_irq_context_t context_irq_txInit = {
    .callback = isr_set_tx_go,
    .pin = GP12,
    .source_mask = GPIO_IRQ_EDGE_FALL
};

// Example 128-bit key (16 bytes)
static const uint8_t test_key[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be connected
    wait_for_usb_console_connection_hal();

    sleep_ms(500);

    print_banner();

    // Initialize the gpio pins
    printf("Initializing hardware...");

    i2c_init_hal(&context_i2c_1);

    // Set to one-shot conversion mode, 16 bit resolution, and a PGA gain of 1.
    mcp3425_init(&context_adc, MCP3425_SPS_15_16BITS, MCP3425_PGA_1, false);

    bool configured_correctly = (!context_adc.continuous_conversion_mode_enabled) && (context_adc.sampling_rate == MCP3425_SPS_15_16BITS) && (context_adc.gain == MCP3425_PGA_1);

    if (!configured_correctly) {
        printf("Failed to configure ADC. Received one-shot, sampling, and gain values of: %d, %d, %d\n", context_adc.continuous_conversion_mode_enabled, context_adc.sampling_rate, context_adc.gain);
        while (1) {};   // Idle
    }
   
    printf("DONE\n");

    while (1) {
        
        printf("To take a reading, press 't'.\n");

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        double voltage = mcp3425_get_measurement(&context_adc);

        printf("Reading: %f\n\n", voltage);
    }

}



/* --- EOF ------------------------------------------------------------------ */