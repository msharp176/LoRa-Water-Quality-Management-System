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

/**
 * RP2350 I/O Map (Raspberry Pi Pico 2 - Will need revisited for final PCB layout)
 * 
 * 1:   GP0             UART Console TX Reserved
 * 2:   GP1             UART Console RX Reserved
 * 3:   GND             -----------------
 * 4:   GP2             sx1262 DIO1 (IRQ)
 * 5:   GP3             
 * 6:   GP4             I2C0 SDA Reserved
 * 7:   GP5             I2C0 SCL Reserved
 * 8:   GND             -----------------
 * 9:   GP6             sx1262 BUSY
 * 10:  GP7             sx1262 RESET
 * 11:  GP8             
 * 12:  GP9
 * 13:  GND             -----------------
 * 14:  GP10
 * 15:  GP11
 * 16:  GP12
 * 17:  GP13
 * 18:  GND             -----------------
 * 19:  GP14
 * 20:  GP15
 * 21:  GP16            SPI0 MISO
 * 22:  GP17            sx1262 Chip Select
 * 23:  GND             -----------------
 * 24:  GP18            SPI0 SCK
 * 25:  GP19            SPI0 MOSI
 * 26:  GP20
 * 27:  GP21
 * 28:  GND             -----------------
 * 29:  GP22
 * 30:  RUN             -----------------
 * 31:  GP26
 * 32:  GP27
 * 33:  GND             -----------------
 * 34:  GP28
 * 35:  ADC_VREF
 * 36:  3V3_OUT
 * 37:  3V3_EN
 * 38:  GND             -----------------
 * 39:  VSYS (5V_REG)   -----------------
 * 40:  VBUS (5V_UREG)  -----------------
 * 
 * --------------------------------------------------------------------------
 * 
 * Protocol Busses
 * 
 * SPI0: sx1262 radio module
 * SPI1:
 * I2C0:
 * I2C1:
 * UART0: Debug Console (on final board)
 * UART1:
 */

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
#include "global_defs.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Definitions & Aliases

#define LORA_TX_POWER_DBM 22

#define LWQMS_SYNC_WORD 0x42    // The meaning of life, the universe, and everything

#define LORA_TIMEOUT_MS 10000

sx126x_mod_params_lora_t prototyping_mod_params = {
    .sf = SX126X_LORA_SF7,
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

sx126x_dio_irq_masks_t prototyping_irq_masks = {
    .system_mask = (SX126X_IRQ_TX_DONE) | (SX126X_IRQ_RX_DONE) | (SX126X_IRQ_TIMEOUT),
    .dio1_mask = (SX126X_IRQ_TX_DONE) | (SX126X_IRQ_RX_DONE) | (SX126X_IRQ_TIMEOUT),
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


#endif

/* --- EOF ------------------------------------------------------------------ */