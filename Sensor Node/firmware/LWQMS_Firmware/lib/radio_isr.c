/******************************************************************************************************************** 
*   
*   @file radio_isr.h
*
*   @brief SX126X Interrupt Service Routines
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "radio_isr.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Radio Dispatch Table

/**
 * @brief Array of pointers to valid sx126x radio context structs. The index of the array corresponds to the radio's IRQ pin.
 */
static sx126x_context_t *radio_dispatch_table[QTY_GPIO_PINS] = {NULL};

/**
 * @brief Array of function pointers to the interrupt-specific service routines
 */
static sx126x_isr_t sx126x_isr_handlers[SX126X_IRQ_REGISTER_WIDTH] = 
{

    &isr_radio_irq_tx_done,             // Bit 0
    &isr_radio_irq_rx_done,             // Bit 1
    &isr_radio_irq_preamble_detected,   // Bit 2
    &isr_radio_irq_sync_word_valid,     // Bit 3
    &isr_radio_irq_header_valid,        // Bit 4
    &isr_radio_irq_header_error,        // Bit 5
    &isr_radio_irq_crc_error,           // Bit 6
    &isr_radio_irq_cad_done,            // Bit 7
    &isr_radio_irq_cad_detected,        // Bit 8
    &isr_radio_irq_timeout,             // Bit 9
    NULL,                               // Bit 10
    NULL,                               // Bit 11
    NULL,                               // Bit 12
    NULL,                               // Bit 13
    &isr_radio_irq_lr_fhss_hop,         // Bit 14
    NULL                                // Bit 15

};  // NULL pointers are those marked "RFU" in datasheet.

// This will keep track of which interrupts have been serviced. As the master ISR executes, it will set the bits of the mask corresponding to which interrupt handlers it fires.
static uint16_t interrupt_service_mask = 0x0000;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


void sx126x_register_radio_irq_pin(sx126x_context_t * radio_context_ptr) {

    radio_dispatch_table[radio_context_ptr->irq] = radio_context_ptr;

    #ifdef DEBUG
        printf("Registered Radio: %s with interrupt pin %d", radio_context_ptr->designator, radio_context_ptr->irq);
    #endif

    return;

}

bool sx126x_check_for_irq_service(uint16_t irq_mask) {

    bool serviced = (interrupt_service_mask & irq_mask) > 0;

    if (serviced) {
        interrupt_service_mask &= !irq_mask;    // Set the checked interrupt bits to 0. Only execute this line if a success so that we can wait for multiple interrupts to be fired.
    }

    return serviced;

}

void sx126x_master_isr(gpio_driven_irq_context_t *context) {

    // 1. Get the radio instance that is causing the interrupt.
    sx126x_context_t * interrupting_radio = radio_dispatch_table[context->pin];
    
    #ifdef DEBUG
        printf("Received an interrupt from %s\n", interrupting_radio->designator);
    #endif

    if (interrupting_radio == NULL) {
        char err_msg[200];

        sprintf(err_msg, "A sx126x radio module has initiated an interrupt on GPIO pin %d, however, the radio was not registered using sx126x_register_radio_irq_pin. The interrupt could not be serviced.\n", context->pin);

        err_raise(ERR_BAD_SETUP, ERR_SEV_FATAL, err_msg, "sx126x_master_isr");
    }


    // 2. Read the current status of the IRQ register to determine which interrupts need handled
    sx126x_irq_mask_t irq_reg = 0;

    bool irq_ok = false; 

    for (int k = 0; k < SPI_RETRIES; k++) {

        do {
            
            if (sx126x_get_irq_status(interrupting_radio, &irq_reg) != SX126X_STATUS_OK) break;

            irq_ok = true;

        } while (0);

        if (irq_ok) break;
    }

    if (!irq_ok) goto err;

    #ifdef DEBUG
        printf("Interrupt mask received: %x\n", irq_reg);
    #endif

    if (irq_reg != 0) { // Sanity check that an interrupt is actually present. No need to go through 16 iterations of a FOR loop during an ISR if not.

        for (int k = 0; k < SX126X_IRQ_REGISTER_WIDTH; k++) {
    
            uint16_t mask = (1 << k); // Check the bits of the IRQ register in ascending order.

            if ((irq_reg & mask) > 0) { // Determine if the bit was set.

                sx126x_isr_t handler = sx126x_isr_handlers[k];
                
                if (handler != NULL) {  // If a handler exists, run it.
                    handler(interrupting_radio);
                }

                interrupt_service_mask |= mask; // Set the corresponding flag in the interrupt service mask.
            }
        }

    } else {
        isr_radio_irq_none(interrupting_radio);
    }

    // 3. Clear the interrupt register since we will service all interrupts in this case.
    irq_ok = false;
    for (int k = 0; k < SPI_RETRIES; k++) {

        do {
            if (sx126x_clear_irq_status(interrupting_radio, SX126X_IRQ_ALL) != SX126X_STATUS_OK) break;

            irq_ok = true;

        } while (0);

        if (irq_ok) break;
    }

    if (!irq_ok) goto err;

    return;

    err:
    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI transaction failure during interrupt fetch/clear", "sx126x_master_isr");
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Interrupt Service Routines ----> PRIVATE!

void isr_radio_irq_none(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_tx_done(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_rx_done(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_preamble_detected(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_sync_word_valid(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_header_valid(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_header_error(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_crc_error(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_cad_done(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_cad_detected(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_timeout(sx126x_context_t* radio) {
    return;
}

void isr_radio_irq_lr_fhss_hop(sx126x_context_t* radio) {
    return;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------