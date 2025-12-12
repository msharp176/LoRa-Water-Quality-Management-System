/*************************************************************************************
 * 
 * @file power_states.c
 * 
 * @brief Header File for Power State Management Utilities for the LoRa Water Quality Management System Sensor Node
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#ifndef POWER_STATES_H
#define POWER_STATES_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies 

#include "hal.h"
#include "lora.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions

#define POWER_SAVING_WAKE_MAGIC_NUMBER 0xBEEFCAFE

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Enums 
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

/**
 * @brief Checks if the MCU is booting due to being woken from the power saving mode, or from a complete power cycle. 
 *  Returns any data written to nonvolatile memory prior to entering the dormant state.
 * 
 * @param novo_mem_contents_buf Buffer to store the contents of the nonvolatile memory.
 * @param buf_len Length of destination buffer. Must be a minimum of `MCU_POWMAN_NOVO_ELEMENTS - 1` long, as the first element is reserved
 * @param data_len Number of data bytes retrieved before empty memory was found.
 * 
 * @returns True for boot due to power saving mode enable, false for complete power cycle boot.
 */
bool check_for_power_saving_mode_boot(uint32_t *novo_mem_contents_buf, size_t buf_len, size_t *data_len);

/**
 * @brief Puts the entire sensor node in power-saving mode by disabling the 5V rail, setting the radio in deep sleep mode, and putting the MCU
 *  in a dormant state.
 * 
 * @param mcu_power_saving_setting Setting to use during dormant (power-saving) mode by the RP2350
 * @param radio_context LoRa Radio Hardware Implementation Information
 * @param power_saving_duration_ms Time to stay in the power-saving mode, in milliseconds
 * @param novo_mem_contents User-defined contents to write to non-volatile (novo) memory when the MCU enters power saving mode. 
 *          Can be retrieved by calling `check_for_power_saving_mode_boot` upon re-entry into the entry point of the program.
 * @param novo_contents_len The number of bytes to write to the non-volatile memory. Maximum length of `MCU_POWMAN_NOVO_ELEMENTS - 1`, 
 *          as the first element is reserved.
 * 
 * @returns Ideally, nothing. The MCU enters the power saving mode and thus no code is executed.
 */
bool enter_power_saving_mode(rp2350_power_mgmt_setting_t *mcu_power_saving_setting, 
            sx126x_context_t *radio_context, uint64_t power_saving_duration_ms, uint32_t *novo_mem_contents, size_t novo_contents_len);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* POWER_STATES_H */