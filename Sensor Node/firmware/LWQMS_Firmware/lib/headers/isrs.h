/******************************************************************************************************************** 
*   
*   @file isrs.h
*
*   @brief Interrupt Service Routines for the LoRa Water Quality Management System
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*   @remark Since interrupt handling is largely platform-dependent, isrs.c/h are NOT platform agnostic. The functions referenced in other files will be, however.
*
*********************************************************************************************************************/

#ifndef ISRS_H
#define ISRS_H

#include "main.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

/**
 * @brief A handler for a GPIO-driven interrupt
 */
typedef void (*gpio_isr_handler_t)(gpio_driven_irq_context_t*);  // A function pointer to a GPIO ISR handler that accepts a pointer to an IRQ context type as its input.

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Interrupt Definitions

extern gpio_driven_irq_context_t irq_txDone;

extern gpio_driven_irq_context_t irq_rxDone;

extern gpio_driven_irq_context_t irq_button1;

extern gpio_driven_irq_context_t irq_button2;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// GPIO-Driven Interrupts

/**
 * @brief Master Interrupt Service Routine for GPIO-driven Interrupts
 * 
 * @param gpio_pin The GPIO pin which raised the interrupt - passed in by hardware ISR
 * @param irq_src  The interrupt triggering event - passed in by hardware ISR
 * 
 * @returns None
 */
void isr_gpio_master(uint gpio_pin, uint32_t irq_src); 

/**
 * @brief Register a GPIO-driven interrupt handler
 * 
 * @param context GPIO-driven interrupt handler context
 * 
 * @returns None
 */
void register_gpio_isr(gpio_driven_irq_context_t *context);

/**
 * @brief Unregister a GPIO-driven interrupt handler
 * 
 * @param context GPIO-driven interrupt handler context
 * 
 * @returns None
 */
void unregister_gpio_isr(gpio_driven_irq_context_t *context);


/**
 * @brief Do-Nothing ISR used for prototyping purposes
 */
void isr_placeholder(void);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


#endif