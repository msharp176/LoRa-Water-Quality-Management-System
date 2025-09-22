/******************************************************************************************************************** 
*   
*   @file lora.h 
*
*   @brief Functions and Type Definitions for the LoRa physical layer communication implementation
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef LORA_H
#define LORA_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Debug Logging

#define LORA_FREQ_NORTH_AMERICA 915000000   //915 MHz

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes

#include "hardware.h"
#include "errs.h"
#include "sx126x_hal.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// sx126x Driver Includes

#include "sx126x.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

typedef struct sx126x_dio_irq_masks_s {
    uint16_t system_mask;
    uint16_t dio1_mask;
    uint16_t dio2_mask;
    uint16_t dio3_mask;
} sx126x_dio_irq_masks_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// SX126X Buffer Base Addresses

#define LORA_TX_BUF_BASE 0x00
#define LORA_RX_BUF_BASE 0x80

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Other Definitions

#define LORA_LDRO_ON 0x01
#define LORA_LDRO_OFF 0x00

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * @brief Software-level initialization of radio parameters and circuit settings. 
 * Sets up the module to use the DC/DC supply, the DIO2/3 pins as controls, and calibrates the radio.
 * 
 * @param [in] context Radio Implementation Parameters
 * 
 * @returns SPI transaction status
 * 
 * @remark Does not setup the physical hardware GPIO pins or SPI bus.
 * 
 * @author Matthew Sharp
 */
sx126x_status_t sx126x_radio_setup(const void* context);

/**
 * @brief Initialize the sx126x in transmit mode
 * 
 * @param radio_context                 sx126x hardware implementation information
 * @param power_amplifier_config        `sx126x_pa_cfg_params_s` PA configuration data struct
 * @param lora_modulation_parameters    `sx126x_mod_params_lora_s` LoRa Modulation configuration data struct
 * @param txPower                   Transmit power in dBm
 * @param ramp_time                 Transmit Ramp Time. Must be a member of `sx126x_ramp_time_e` enum.
 * @param sync_word                 LoRa Sync Word
 * 
 * @returns Radio Initialization Status 
 */
bool lora_init_tx(  const sx126x_context_t* radio_context, 
                    sx126x_pa_cfg_params_t* power_amplifier_config,
                    sx126x_mod_params_lora_t* lora_modulation_parameters, 
                    int8_t  txPower,
                    sx126x_ramp_time_t ramp_time,
                    uint8_t sync_word);

/**
 * @brief Transmit data over LoRa
 * 
 * @param radio_context                 sx126x hardware implementation info
 * @param radio_interrupt_cfg           `sx126x_dio_irq_masks_s` LoRa interrupt configuration data struct
 * @param lora_packet_parameters        `sx126x_pkt_params_lora_s` LoRa packet configuration data struct. Can be passed with or without the payload length.
 * @param buf                           Pointer to data to be sent
 * @param len                           Length of data to transmit
 * 
 * @returns Radio Transmit Status
 */
bool lora_tx(   const sx126x_context_t* radio_context,
                sx126x_dio_irq_masks_t* radio_interrupt_cfg,
                sx126x_pkt_params_lora_t* lora_packet_parameters,
                uint8_t *buf, 
                uint8_t len);

/**
 * @brief Initialize the sx126x in receive mode
 * 
 * @param radio_context                 sx126x hardware implementation info
 * @param lora_modulation_parameters    `sx126x_mod_params_lora_s` LoRa Modulation configuration data struct
 * @param lora_packet_parameters        `sx126x_pkt_params_lora_s` LoRa packet configuration data struct
 * 
 * @returns Radio Initialization Status
 */
bool lora_init_rx(  sx126x_context_t* radio_context,
                    sx126x_mod_params_lora_t* lora_modulation_parameters, 
                    sx126x_pkt_params_lora_t* lora_packet_parameters);

/**
 * @brief Receive data over LoRa
 * 
 * @param radio_context             sx126x hardware implementation info
 * @param interrupt_cfg             `sx126x_dio_irq_masks_s`   LoRa interrupt configuration data struct
 * @param sync_word                 LoRa Sync Word
 * @param timeout_ms                RX Timeout in milliseconds
 * @param buf                       Pointer to data buffer for received data
 * @param len                       Length of data buffer
 * 
 * @returns Radio Reception Status
 */
bool lora_rx(   sx126x_context_t* radio_context,
                sx126x_dio_irq_masks_t* interrupt_cfg,
                uint8_t sync_word,
                uint32_t timeout_ms);

/**
 * @brief Chooses whether to turn on the LoRa Low Data Rate Optimization based on the selected LoRa Modulation Params.
 * 
 * @remark Requires that spreading factor (SF), bandwidth (BW), and coding rate (CR) are already set
 * 
 * @param modulation_params pointer to `sx126x_mod_params_lora_t` struct to be edited in-place 
 * 
 * @returns None 
 */
void set_lora_ldro_val(sx126x_mod_params_lora_t* modulation_params);

/**
 * @brief Retrieves data stored in the RX Buffer from the last packet received.
 * 
 * @param radio_context sx126x hardware implementation info
 * @param rxBuf Buffer to store received bytes. THIS BUFFER MUST HAVE A SIZE GREATER THAN OR EQUAL TO THE TOTAL DATA BUFFER CAPACITY (256 BYTES) TO AVOID SEG FAULTS!
 * @param rxLen Received packet length
 */
bool lora_get_rx_data(sx126x_context_t* radio_context, uint8_t * rxBuf, uint8_t * rxLen);
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif

/* --- EOF ------------------------------------------------------------------ */