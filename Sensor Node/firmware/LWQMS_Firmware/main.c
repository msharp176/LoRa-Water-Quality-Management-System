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

void print_banner(void) {
    printf("-- LoRa Water Quality Management System Sensor Node --\n");
    printf("Version 0.1, compiled %s, %s\n\n", __DATE__, __TIME__);
}


int main() {

    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

    print_banner();

    printf("Initializing Hardware...");
    i2c_init_hal(&context_i2c_1);
    printf("DONE\n");

    i2c_scan_hal(&context_i2c_1);

    /*
    while (true) {
        while (context_digipot_offset.wiper_position_a < 256) {
            mcp4651_increment_wiper(&context_digipot_offset, MCP4651_WIPER_A);
            printf("%u\n", context_digipot_offset.wiper_position_a);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }

        while (context_digipot_offset.wiper_position_a > 0) {
            mcp4651_decrement_wiper(&context_digipot_offset, MCP4651_WIPER_A);
            printf("%u\n", context_digipot_offset.wiper_position_a);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }

        while (context_digipot_offset.wiper_position_b < 256) {
            mcp4651_increment_wiper(&context_digipot_offset, MCP4651_WIPER_B);
            printf("%u\n", context_digipot_offset.wiper_position_b);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }

        while (context_digipot_offset.wiper_position_b > 0) {
            mcp4651_decrement_wiper(&context_digipot_offset, MCP4651_WIPER_B);
            printf("%u\n", context_digipot_offset.wiper_position_b);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }
    }
    */

    /*
    printf("Setting Pot Values...");
    // Set the DC offsets to 0 and 0, with a gain of around 1 to check the op-amp connections.
    mcp4651_set_wiper(&context_digipot_offset, MCP4651_WIPER_BOTH, 0);
    mcp4651_set_wiper(&context_digipot_gain, MCP4651_WIPER_A, 154);
    mcp4651_set_wiper(&context_digipot_gain, MCP4651_WIPER_B, 255);
    printf("DONE\n");

    printf("Enabling Output 0...");
    tmux1309_set_output(&context_mux_0, 0);
    printf("DONE\n");

    */
    // Idle
    while (true) {};

    /*
    while (true) {
        while (context_digipot_offset.wiper_position_a < 256) {
            mcp4651_increment_wiper(&context_digipot_offset, MCP4651_WIPER_A);
            printf("%u\n", context_digipot_offset.wiper_position_a);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }

        while (context_digipot_offset.wiper_position_a > 0) {
            mcp4651_decrement_wiper(&context_digipot_offset, MCP4651_WIPER_A);
            printf("%u\n", context_digipot_offset.wiper_position_a);
            sleep_ms(DIGIPOT_STEP_TIME_MS);
        }
    }
    */
}


/*
int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

    sleep_ms(100);

    /*
    usb_console_write_hal("Initializing Hardware...");
    sx126x_initialize_hardware_context(&radio_0);
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio...");
    sx126x_radio_setup(&radio_0);    
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up interrupts...");
    sx126x_interrupt_setup(&radio_0);
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio for a transmit operation...");
    
    lora_init_tx(   &radio_0, 
                    &sx1262_14dBm_pa_params, 
                    &prototyping_mod_params,
                    14, 
                    SX126X_RAMP_200_US,
                    LWQMS_SYNC_WORD
                );

    usb_console_write_hal("DONE\n\n\n");

    char * txBuf = "Four score and seven years ago our fathers brought forth on this continent a new nation.";

    while (true) {

        usb_console_write_hal("To transmit a packet, press 't'.\n");

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        usb_console_write_hal("Sending Packet...");

        // Transmit a packet
        lora_tx(&radio_0,
                &prototyping_irq_masks,
                &prototyping_pkt_params,
                txBuf,
                89
        );

        usb_console_write_hal("DONE\n");

        usb_console_write_hal("Waiting for interrupt...");
        while (!sx126x_check_for_interrupt()) {}
        usb_console_write_hal("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_TX_DONE) != 0) {
            usb_console_write_hal("TX Success!");
        }
        else if ((serviced_interrupts & SX126X_IRQ_TIMEOUT) != 0) {
            usb_console_write_hal("ERROR: TIMEOUT!");
        }
        else {
            usb_console_write_hal("Bad operation!");
        }

        usb_console_write_hal("\n\n");
    }
    */
//}


/* --- EOF ------------------------------------------------------------------ */