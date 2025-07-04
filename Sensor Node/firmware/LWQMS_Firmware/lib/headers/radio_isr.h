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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// C Standard Library Includes

#include <stdlib.h>
#include <string.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes

#include "sx126x.h"
#include "sx126x_hal.h"

#include "errs.h"
#include "hardware.h"
#include "isrs.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions and Aliases

#define SX126X_IRQ_REGISTER_WIDTH 16

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

typedef void (*sx126x_isr_t)(sx126x_context_t*);  // A function pointer to an ISR that accepts a pointer to a radio context type as its input.

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


/**
 * @brief Register an sx126x radio with the master radio ISR handler.
 * 
 * @param radio_context_ptr Pointer to valid `sx126x_context_s` struct. When the corresponding IRQ pin raises an interrupt, this radio will be serviced
 * 
 * @note This function does NOT replace the standard callback. The radio IRQ must be registered globally using `gpio_irq_attach_hal`
 * 
 * @returns None
 */
void sx126x_register_radio_irq_pin(sx126x_context_t * radio_context_ptr);

/**
 * @brief Checks if an sx126x interrupt has been serviced.
 * 
 * @param irq_mask Bitmask for sx126x interrupts. Can represent one or more interrupts.
 * 
 * @returns Interrupt service status
 * 
 * @note Upon a success (function returns true), the corresponding interrupt service flags will be cleared and ready to detect the next interrupt service.s
 */
bool sx126x_check_for_irq_service(uint16_t irq_mask);

/**
 * @brief Master Interrupt Service Routine for the SX126X. Selects which specific interrupt subroutine to execute based on the contents of the IRQ register
 * 
 * @param pin: The pin which raised the interrupt. Used to identify the radio module
 * 
 * @note All SX126X radio DIOx interrupts should have their callback registered to be here within the master ISR dispatch table
 */
void sx126x_master_isr(gpio_driven_irq_context_t *context);

// All sub-isrs will be private to within radio_isr.c so that they can not be mistakenly assigned as a callback to a global isr.

/* --- EOF ------------------------------------------------------------------ */