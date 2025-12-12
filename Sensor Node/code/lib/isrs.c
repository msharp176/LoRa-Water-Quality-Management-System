/******************************************************************************************************************** 
*   
*   @file isrs.c
*
*   @brief Interrupt Service Routines for the LoRa Water Quality Management System
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "isrs.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// ISR Dispatch Table

/**
 * @brief An array of function pointers to GPIO-driven interrupt service routines. The index of the table corresponds to the GPIO pin triggering the interrupt.
 */
static gpio_isr_handler_t isr_dispatch_table[QTY_GPIO_PINS] = {NULL}; // The STATIC modifier ensures this array can not be modified from outside of this file.

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Interrupt Definitions

void isr_toggle_led(void);
void isr_print(void);

gpio_driven_irq_context_t irq_button1 = {
    .pin = GP14,
    .source_mask = GPIO_IRQ_EDGE_FALL,
    .callback = isr_toggle_led
};

gpio_driven_irq_context_t irq_button2 = {
    .pin = GP15,
    .source_mask = GPIO_IRQ_EDGE_FALL,
    .callback = isr_print
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


void isr_gpio_master(uint gpio_pin, uint32_t irq_src) {

    /**
     * "I heard that his interrupt service routine selection method is O(1)" - All Women
     * https://en.meming.world/images/en/thumb/7/72/Girls_Gossiping.jpg/450px-Girls_Gossiping.jpg
     */

    // Acknowledge the ISR
    gpio_driven_irq_context_t received_context_data = {
        .pin = gpio_pin,
        .source_mask = irq_src,
        .callback = NULL        // At this point, the callback matters not
    };

    // Acknowledge the interrupt is being serviced on the RP2350
    gpio_irq_ack_hal(&received_context_data);

    // Extract the interrupt handler
    gpio_isr_handler_t handler = isr_dispatch_table[gpio_pin];

    // Execute the handler
    if (handler != NULL) {
        handler(&received_context_data);
    }

}

void register_gpio_isr(gpio_driven_irq_context_t *context) {
    
    isr_dispatch_table[context->pin] = context->callback;

    return;
}

void unregister_gpio_isr(gpio_driven_irq_context_t *context) {

    isr_dispatch_table[context->pin] = NULL;

    return;
}

void isr_placeholder(void) {
    // Do nothing
}

void isr_toggle_led(void) {

    gpio_toggle_hal(err_led);

}

void isr_print(void) {
    usb_console_write_hal("IRQ!!!\n");
}