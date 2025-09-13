/*************************************************************************************
 * 
 * @file tmux1309.h
 * 
 * @brief Header file for the TMUX1309 Dual 4:1 Bidirectional Analog Multiplexer Driver
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#ifndef TMUX1309_H
#define TMUX1309_H

#include "hardware.h"
#include "hal.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * @brief Initializes all GPIO pins associated with the given TMUX1309 instance, and sets all outputs HIGH (disabling mux)
 * 
 * @returns 0 for a successful operation
 */
int tmux1309_init(tmux1309_context_t *context);

/**
 * @brief Disables the given TMUX1309, preventing any inputs from propagating to the output.
 * 
 * @returns 0 for a successful operation
 */
int tmux1309_disable(tmux1309_context_t *context);

/**
 * @brief Enables the given TMUX1309, allowing signals to propagate to the output.
 * 
 * @returns 0 for a successful operation
 */
int tmux1309_enable(tmux1309_context_t *context);

/**
 * @brief Sets the output of the given TMUX1309 based on the selection provided (0->3).
 * 
 * @returns 0 for a successful operation
 */
int tmux1309_set_output(tmux1309_context_t *context, uint8_t selection);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


#endif /* TMUX1309_H */

/* --- EOF ------------------------------------------------------------------ */