/******************************************************************************************************************** 
*   
*   @file system_config.h 
*
*   @brief Program-wide configuration data for the LoRa Water Quality Management System Sensor Node firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies
#include "hardware.h"
#include "hal.h"
#include "lora.h"
#include "sx126x_hal.h"
#include "mxl23l3233f.h"
#include "tmux1309.h"
#include "lwqms_pkt.h"
#include "software_defined_inst_amp.h"
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Debug Logging

#define DEBUG

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Configuration Type

typedef struct node_config_s {
    uint16_t   ID;     // 10 characters + null termination.
    double latitude;
    double longitude;
    char   sync_word;    
} node_config_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// POST Error Codes

typedef enum lwqms_post_err_codes_e {

    POST_OK = 0,
    POST_ERR_GPIO_INIT = -1,
    POST_ERR_SPI_FLASH_FAIL = -2,
    POST_ERR_I2C_COMMS_FAIL = -3,
    POST_ERR_RADIO_INIT_FAIL = -4,
    POST_ERR_ADC_INIT_FAIL = -5,
    POST_ERR_DIGIPOT_INIT_FAIL = -6,
    POST_ERR_NO_CONFIG_EXISTS = -7,
    POST_ERR_I2C_DEVICE_NOT_DETECTED = -8,

} lwqms_post_err_codes_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Key SPI Flash Memory Addresses

#define FLASH_ADDR_CONFIG 0x00
#define FLASH_ADDR_TURB_SEN_SETTINGS 0x100
#define FLASH_ADDR_TEMP_SEN_SETTINGS 0x200
#define FLASH_ADDR_EXTRA_SEN_SETTINGS 0x300
#define FLASH_ADDR_PH_SEN_SETTINGS 0x400
#define FLASH_ADDR_BULK_DATA 0x1000

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

void print_node_configuration(node_config_t *config);

int initialize_gpio();

int read_system_config_data(mxl23l3233f_context_t * flash_context, node_config_t * cfgBuf);

int write_system_config_data(mxl23l3233f_context_t * flash_context, node_config_t * config);

node_config_t get_setup_data();

lwqms_post_err_codes_t power_on_self_test();

lwqms_pkt_t get_custom_packet();

sdia_wiper_settings_t get_wiper_setting();

uint16_t string_to_uint16_t(const char *str, int base);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

extern node_config_t sys_configuration;


#endif /* SYSTEM_CONFIG_H */