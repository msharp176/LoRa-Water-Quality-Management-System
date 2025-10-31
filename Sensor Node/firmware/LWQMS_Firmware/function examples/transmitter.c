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


int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    //wait_for_usb_console_connection_hal();

    sleep_ms(100);

    print_banner();

    usb_console_write_hal("Initializing Hardware...");
    sx126x_initialize_hardware_context(&context_radio_0);

    gpio_setup_hal(context_irq_txInit.pin, false);
    gpio_setup_hal(STATUS_LED, true);
    gpio_write_hal(STATUS_LED, false);
    gpio_setup_hal(RX_LED, true);
    gpio_write_hal(RX_LED, false);

    gpio_irq_attach_hal(&context_irq_txInit);

    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio...");
    sx126x_radio_setup(&context_radio_0);    
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up interrupts...");
    sx126x_interrupt_setup(&context_radio_0);
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio for a transmit operation...");
    
    lora_init_tx(   &context_radio_0, 
                    &sx1262_14dBm_pa_params, 
                    &prototyping_mod_params,
                    14, 
                    SX126X_RAMP_200_US,
                    LWQMS_SYNC_WORD
                );

    usb_console_write_hal("DONE\n\n\n");

    gpio_write_hal(STATUS_LED, true);

    char * txBuf = "Four score and seven years ago our fathers brought forth on this continent a new nation.";

    while (true) {

        usb_console_write_hal("To transmit a packet, press 't'.\n");

        // Wait for input
        //while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive
        while (!tx_go) {}
        tx_go = false;

        usb_console_write_hal("Sending Packet...");

        gpio_write_hal(RX_LED, true);

        // Transmit a packet
        lora_tx(&context_radio_0,
                &prototyping_irq_masks,
                &prototyping_pkt_params,
                txBuf,
                89
        );

        usb_console_write_hal("DONE\n");

        usb_console_write_hal("Waiting for interrupt...");
        while (!sx126x_check_for_interrupt()) {}
        gpio_write_hal(RX_LED, false);
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
}



/* --- EOF ------------------------------------------------------------------ */