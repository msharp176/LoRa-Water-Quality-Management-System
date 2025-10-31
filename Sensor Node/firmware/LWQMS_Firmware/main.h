/******************************************************************************************************************** 
*   
*   @file main.h 
*
*   @brief Main Header File for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef MAIN_H
#define MAIN_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// C Standard Library

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Raspberry Pi Pico SDK Includes

#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "pico/time.h"
#include "tusb.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes

#include "hal.h"
#include "errs.h"
#include "sx126x.h"
#include "isrs.h"
#include "hardware.h"
#include "lora.h"
#include "system_config.h"
#include "mcp4651.h"
#include "mcp3425.h"
#include "mxl23l3233f.h"
#include "tmux1309.h"
#include "encryption.h"

#include "sx126x.h"

#include "rdt3.h"
#include "lwqms_pkt.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Definitions & Aliases

#define LORA_TX_POWER_DBM 22

#define LWQMS_SYNC_WORD 0x42    // The meaning of life, the universe, and everything

#define LORA_TIMEOUT_MS 10000

sx126x_mod_params_lora_t prototyping_mod_params = {
    .sf = SX126X_LORA_SF10,
    .bw = SX126X_LORA_BW_125,
    .cr = SX126X_LORA_CR_4_5,
    .ldro = 0x00
};

sx126x_pkt_params_lora_t prototyping_pkt_params = {
    .preamble_len_in_symb = 8,
    .header_type = SX126X_LORA_PKT_EXPLICIT,
    .crc_is_on = false,
    .invert_iq_is_on = false   // Do not invert the IQ unless needed to make network invisible to other LoRa devices - typically involved with LoRaWAN
};

sx126x_dio_irq_masks_t prototyping_irq_masks = {
    .system_mask = (SX126X_IRQ_TX_DONE) | (SX126X_IRQ_RX_DONE) | (SX126X_IRQ_TIMEOUT) | (SX126X_IRQ_CRC_ERROR) | (SX126X_IRQ_HEADER_ERROR),
    .dio1_mask = (SX126X_IRQ_TX_DONE) | (SX126X_IRQ_RX_DONE) | (SX126X_IRQ_TIMEOUT) | (SX126X_IRQ_CRC_ERROR) | (SX126X_IRQ_HEADER_ERROR),
    .dio2_mask = 0x00,
    .dio3_mask = 0x00
};

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

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
// Main ISRs

void isr_toggle_led(void);

void isr_print(void);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

extern lora_setup_t lora_phy_setup;

#endif

/* --- EOF ------------------------------------------------------------------ */