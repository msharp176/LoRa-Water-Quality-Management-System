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


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// sx126x Driver Includes

#include "sx126x.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Pre-Defined Optimal Power Amplifier Operating Modes (per datasheet)

sx126x_pa_cfg_params_t sx1262_14dBm_pa_params = {
    .pa_duty_cycle = 0x02,
    .hp_max = 0x02,
    .device_sel = 0x00,
    .pa_lut = 0x01
};

sx126x_pa_cfg_params_t sx1262_17dBm_pa_params = {
    .pa_duty_cycle = 0x02,
    .hp_max = 0x03,
    .device_sel = 0x00,
    .pa_lut = 0x01 
};

sx126x_pa_cfg_params_t sx1262_20dBm_pa_params = {
    .pa_duty_cycle = 0x03,
    .hp_max = 0x05,
    .device_sel = 0x00,
    .pa_lut = 0x01
};

sx126x_pa_cfg_params_t sx1262_22dBm_pa_params = {
    .pa_duty_cycle = 0x04,
    .hp_max = 0x07,
    .device_sel = 0x00,
    .pa_lut = 0x01
};

sx126x_pa_cfg_params_t sx1261_10dBm_pa_params = {
    .pa_duty_cycle = 0x01,
    .hp_max = 0x00,
    .device_sel = 0x01,
    .pa_lut = 0x01
};

sx126x_pa_cfg_params_t sx1261_14dBm_pa_params = {
    .pa_duty_cycle = 0x04,
    .hp_max = 0x00,
    .device_sel = 0x01,
    .pa_lut = 0x01
};

sx126x_pa_cfg_params_t sx1261_15dBm_pa_params = {
    .pa_duty_cycle = 0x06,
    .hp_max = 0x00,
    .device_sel = 0x01,
    .pa_lut = 0x01
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// TX Parameters

#define LORA_TX_POWER_DBM 22

#define LORA_LDRO_ON 0x01
#define LORA_LDRO_OFF 0x00

#define LWQMS_SYNC_WORD 0x42    // The meaning of life, the universe, and everything

#define LORA_TIMEOUT_MS 10000

sx126x_mod_params_lora_t prototyping_mod_params = {
    .sf = SX126X_LORA_BW_007,
    .bw = SX126X_LORA_BW_125,
    .cr = SX126X_LORA_CR_4_5,
    .ldro = 0x00
};

sx126x_pkt_params_lora_t prototyping_pkt_params = {
    .preamble_len_in_symb = 12,
    .header_type = SX126X_LORA_PKT_EXPLICIT,
    .crc_is_on = true,
    .invert_iq_is_on = false   // Do not invert the IQ unless needed to make network invisible to other LoRa devices - typically involved with LoRaWAN
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// IRQ Masks

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
// Function Definitions

/**
 * @brief Initialize the sx126x in transmit mode
 * 
 * @param radio_context             sx126x hardware implementation information
 * @param power_amplifier_config    `sx126x_pa_cfg_params_s` PA configuration data struct
 * @param txPower:                  Transmit power in dBm
 * @param ramp_time                 Transmit Ramp Time. Must be a member of `sx126x_ramp_time_e` enum.
 * 
 * @returns Radio Initialization Status 
 */
bool lora_init_tx(  const sx126x_context_t* radio_context, 
                    sx126x_pa_cfg_params_t* power_amplifier_config,
                    int8_t  txPower,
                    sx126x_ramp_time_t ramp_time); 

/**
 * @brief Transmit data over LoRa
 * 
 * @param radio_context                 sx126x hardware implementation info
 * @param lora_modulation_parameters    `sx126x_mod_params_lora_s` LoRa Modulation configuration data struct
 * @param lora_packet_parameters        `sx126x_pkt_params_lora_s` LoRa packet configuration data struct
 * @param interrupt_cfg                 `sx126x_dio_irq_masks_s`   LoRa interrupt configuration data struct
 * @param buf                           Pointer to data to be sent
 * @param len                           Length of data to transmit
 * 
 * @returns Radio Transmit Status
 * 
 */
bool lora_tx(   const sx126x_context_t* radio_context, 
                sx126x_mod_params_lora_t* lora_modulation_parameters, 
                sx126x_pkt_params_lora_t* lora_packet_parameters,
                sx126x_dio_irq_masks_t* interrupt_cfg,
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
 * @param buf                       Pointer to data buffer for received data
 * @param len                       Length of data buffer
 * 
 * @returns Radio Reception Status
 */
bool lora_rx(   sx126x_context_t* radio_context,
                sx126x_dio_irq_masks_t* interrupt_cfg,
                uint8_t *buf, 
                uint32_t len);

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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif

/* --- EOF ------------------------------------------------------------------ */