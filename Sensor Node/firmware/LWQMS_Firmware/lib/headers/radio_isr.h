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
#include <stdio.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes

#include "sx126x.h"
#include "errs.h"
#include "hardware.h"
#include "isrs.h"
#include "system_config.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions and Aliases

#define SX126X_IRQ_REGISTER_WIDTH 16

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

typedef struct sx126x_context_s sx126x_context_t;

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
 * @brief Master Interrupt Service Routine for the SX126X. Sets a flag which can be retrieved using `sx126x_check_for_interrupt`
 * 
 * @param pin: The pin which raised the interrupt. Used to identify the radio module
 * 
 * @note All SX126X radio DIOx interrupts should have their callback registered to be here within the master ISR dispatch table
 */
void sx126x_master_isr(gpio_driven_irq_context_t *context);

/**
 * @brief Manually service interrupts on the sx126x
 */
sx126x_irq_mask_t sx126x_manual_isr(sx126x_context_t *radio_context);

/**
 * @brief Checks if any registered sx126x instance has raised an interrupt.
 * 
 * @returns True for a pending interrupt, false otherwise.
 */
bool sx126x_check_for_interrupt(void);

/**
 * @brief Services an interrupt on the most recently interrupting sx126x radio instance.
 * 
 * @returns The contents of the IRQ register of the sx126x, indicating which interrupt service routines were triggered.
 */
sx126x_irq_mask_t sx126x_service_interrupts(void);

// All sub-isrs will be private to within radio_isr.c so that they can not be mistakenly assigned as a callback to a global isr.

/* --- EOF ------------------------------------------------------------------ */