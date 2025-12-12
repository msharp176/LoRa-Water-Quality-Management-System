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

#include "hal.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "hardware.h"
#include "system_config.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Interrupt Definitions

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
 * @remark Use the function: `gpio_attach_hal` from `hal.h` to register a GPIO-driven interrupt. This function ONLY registers the callback with the main ISR dispatch table. 
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