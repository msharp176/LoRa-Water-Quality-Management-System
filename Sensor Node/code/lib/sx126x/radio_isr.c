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
#include "sx126x_private_isrs.h"

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

    &isr_radio_irq_tx_done,             // Bit 0 - TXDone Flag
    &isr_radio_irq_rx_done,             // Bit 1 - RXDone Flag
    &isr_radio_irq_preamble_detected,   // Bit 2 - Preamble Detected Flag
    &isr_radio_irq_sync_word_valid,     // Bit 3 - Sync Word Valid Flag
    &isr_radio_irq_header_valid,        // Bit 4 - Header Valid Flag
    &isr_radio_irq_header_error,        // Bit 5 - Header Error Flag
    &isr_radio_irq_crc_error,           // Bit 6 - CRC Error Flag
    &isr_radio_irq_cad_done,            // Bit 7 - Channel Activity Detection Flag Done
    &isr_radio_irq_cad_detected,        // Bit 8 - Channel Activity Detected
    &isr_radio_irq_timeout,             // Bit 9 - RX/TX Timeout
    NULL,                               // Bit 10 - RFU by Mfg.
    NULL,                               // Bit 11 - RFU by Mfg.
    NULL,                               // Bit 12 - RFU by Mfg.
    NULL,                               // Bit 13 - RFU by Mfg.
    &isr_radio_irq_lr_fhss_hop,         // Bit 14 - Asserted at each hop in LKR-FHSS, after the PA has ramped up again.
    NULL                                // Bit 15 - RFU by Mfg.

};  // NULL pointers are those marked "RFU" in datasheet.

// Identifies if an IRQ was requested
volatile bool sx126x_radio_interrupt_triggered = false;

// Identifies the source of the sx126x radio interrupt request.
volatile uint8_t sx126x_radio_interrupting_pin;
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

bool read_irq_register(sx126x_context_t *radio_context, sx126x_irq_mask_t *irq_reg) {
    
    bool irq_ok;

    for (int k = 0; k < COMMS_RETRIES; k++) {

        do {
            
            if (sx126x_get_irq_status(radio_context, irq_reg) != SX126X_STATUS_OK) break;

            irq_ok = true;

        } while (0);

        if (irq_ok) break;
    }

    return irq_ok;
}

bool service_interrupts(sx126x_context_t *radio_context, sx126x_irq_mask_t irq_reg) {
    
    if (irq_reg != 0) { // Sanity check that an interrupt is actually present. No need to go through 16 iterations of a FOR loop during an ISR if not.

        for (int k = 0; k < SX126X_IRQ_REGISTER_WIDTH; k++) {
    
            uint16_t mask = (1 << k); // Check the bits of the IRQ register in ascending order.

            if ((irq_reg & mask) > 0) { // Determine if the bit was set.

                sx126x_isr_t handler = sx126x_isr_handlers[k];
                
                if (handler != NULL) {  // If a handler exists, run it.
                    handler(radio_context);
                }
            }
        }

    } else {
        isr_radio_irq_none(radio_context);
    }

    return true;
}

bool clear_interrupts(sx126x_context_t *radio_context) {

    bool irq_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {

        do {
            if (sx126x_clear_irq_status(radio_context, SX126X_IRQ_ALL) != SX126X_STATUS_OK) break;

            irq_ok = true;

        } while (0);

        if (irq_ok) break;
    }

    return irq_ok;
}

void sx126x_register_radio_irq_pin(sx126x_context_t * radio_context_ptr) {

    radio_dispatch_table[radio_context_ptr->irq_context->pin] = radio_context_ptr;

    return;

}

void sx126x_master_isr(gpio_driven_irq_context_t *context) {

    // Set the flag that there is an sx126x interrupt pending
    sx126x_radio_interrupt_triggered = true;

    // Set the source of the interrupt to be the triggering pin (and the index of the radio interrupt dispatch table)
    sx126x_radio_interrupting_pin = context->pin;
}

bool sx126x_check_for_interrupt(void) {
    return sx126x_radio_interrupt_triggered;
}
    
sx126x_irq_mask_t sx126x_manual_isr(sx126x_context_t *radio_context) {

    sx126x_irq_mask_t mask;

    bool read_ok = read_irq_register(radio_context, &mask);

    if (!read_ok) goto err;

    bool service_ok = service_interrupts(radio_context, mask);

    if (!service_ok) goto err;

    bool clear_ok = clear_interrupts(radio_context);

    if (!clear_ok) goto err;
    
    return mask;

    err:
    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI transaction failure during interrupt fetch/clear", "sx126x_manual_isr");

    return 0x0000;
}

sx126x_irq_mask_t sx126x_service_interrupts(void) {

    if (!sx126x_radio_interrupt_triggered || sx126x_radio_interrupting_pin < 0) return 0x0000;

    // Extract the interrupting radio source from the dispatch table
    sx126x_context_t *interrupting_radio = radio_dispatch_table[sx126x_radio_interrupting_pin];

    if (interrupting_radio == NULL) {
        char err_msg[200];

        sprintf(err_msg, "A sx126x radio module has initiated an interrupt on GPIO pin %d, however, the radio was not registered using sx126x_register_radio_irq_pin. The interrupt could not be serviced.\n", sx126x_radio_interrupting_pin);

        err_raise(ERR_BAD_SETUP, ERR_SEV_FATAL, err_msg, "sx126x_master_isr");
    }
        
    #ifdef DEBUG
        printf("Received an interrupt from %s\n", interrupting_radio->designator);
    #endif

    // 2. Read the current status of the IRQ register to determine which interrupts need handled
    sx126x_irq_mask_t irq_reg = 0;

    bool irq_ok = read_irq_register(interrupting_radio, &irq_reg);

    if (!irq_ok) goto err;

    #ifdef DEBUG
        printf("Interrupt mask received: %x\n", irq_reg);
    #endif

    irq_ok = service_interrupts(interrupting_radio, irq_reg);

    // 3. Clear the interrupt register since we will service all interrupts in this case.
    irq_ok = clear_interrupts(interrupting_radio);

    if (!irq_ok) goto err;

    // Clear the interrupt flag and pin
    sx126x_radio_interrupt_triggered = false;
    sx126x_radio_interrupting_pin = -1;    

    return irq_reg;

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