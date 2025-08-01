/******************************************************************************************************************** 
*   
*   @file sx126x_private_isrs.h
*
*   @brief Interrupt source specific interrupt service routine function declarations. Only to be used in radio_isr.c.
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "hardware.h"

void isr_radio_irq_none(sx126x_context_t* radio);

void isr_radio_irq_tx_done(sx126x_context_t* radio);

void isr_radio_irq_rx_done(sx126x_context_t* radio);

void isr_radio_irq_preamble_detected(sx126x_context_t* radio);

void isr_radio_irq_sync_word_valid(sx126x_context_t* radio);

void isr_radio_irq_header_valid(sx126x_context_t* radio);

void isr_radio_irq_header_error(sx126x_context_t* radio);

void isr_radio_irq_crc_error(sx126x_context_t* radio);

void isr_radio_irq_cad_done(sx126x_context_t* radio);

void isr_radio_irq_cad_detected(sx126x_context_t* radio);

void isr_radio_irq_timeout(sx126x_context_t* radio);

void isr_radio_irq_lr_fhss_hop(sx126x_context_t* radio);