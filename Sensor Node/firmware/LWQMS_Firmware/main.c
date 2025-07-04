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

void master_irq_callback(uint gpio_pin, uint32_t event_type);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Variables

char * gettysburg_address = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth. Abraham Lincoln November 19, 1863";

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * To transmit data:
 * Initialize radio
 * Initialize TX mode
 * Attach the radio interrupt
 * Transmit data
 * Wait for interrupt
 */

int main()
{
    init_usb_console_hal();

    wait_for_usb_console_connection_hal();

    sleep_ms(1000);

    usb_console_write_hal("USB Connected. Woohoo!\n");

    sx126x_init(&radio_0);

    // Initialize the GPIO pins
    gpio_setup_hal(&err_led, true);

    gpio_setup_hal(&(irq_button1.pin), false);
    gpio_setup_hal(&(irq_button2.pin), false);

    gpio_irq_attach_hal(&irq_button1);
    gpio_irq_attach_hal(&irq_button2);

    usb_console_write_hal("IRQ configured. Waiting for interrupt.\n");

    while (true) {}
}

/* --- EOF ------------------------------------------------------------------ */