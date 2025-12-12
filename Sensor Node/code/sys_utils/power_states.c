/*************************************************************************************
 * 
 * @file power_states.c
 * 
 * @brief Power State Management Utilities for the LoRa Water Quality Management System Sensor Node
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "power_states.h"

static rp2350_power_mgmt_setting_t mcu_all_on = {
    .domains = {
        .swcore_enable = true,
        .xip_enable = true,
        .sram0_enable = true,
        .sram1_enable = true
    }
};

bool check_for_power_saving_mode_boot(uint32_t *novo_mem_contents_buf, size_t buf_len, size_t *data_len) {
    if (buf_len < (MCU_POWMAN_NOVO_ELEMENTS - 1)) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "The buffer to write the contents of the non-volatile memory is too short!", "check_for_power_saving_mode_boot");
        return false;
    }

    uint32_t rxBuf[MCU_POWMAN_NOVO_ELEMENTS];
    power_mgmt_read_novo_memory_hal(rxBuf, MCU_POWMAN_NOVO_ELEMENTS);
    
    // Clear out the incoming buffer
    memset(novo_mem_contents_buf, 0x00, buf_len);

    memcpy(novo_mem_contents_buf, rxBuf + sizeof(uint32_t), MCU_POWMAN_NOVO_ELEMENTS - 1);

    for (int k = 0; k < buf_len; ++k) {
        if ((novo_mem_contents_buf[k] == 0) || (novo_mem_contents_buf[k] == 0xffffffff)) {
            *data_len = k;
            break;
        }
    }

    return (rxBuf[0] == POWER_SAVING_WAKE_MAGIC_NUMBER);
}

bool enter_power_saving_mode(rp2350_power_mgmt_setting_t *mcu_power_saving_setting, 
            sx126x_context_t *radio_context, uint64_t power_saving_duration_ms, uint32_t *novo_mem_contents, size_t novo_contents_len) {

    // Check the number of 32-bit elements to be written.
    if (novo_contents_len > (MCU_POWMAN_NOVO_ELEMENTS - 1))  { // Save 1 for the magic number at the beginning
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "The contents of the nonvolatile memory for power saving are too long!", "enter_power_saving_mode");
        return false;
    }

    // Disable the 5V rail
    gpio_write_hal(EN_5V, GPIO_LOW);

    // Put the radio in deep sleep mode
    lora_enter_sleep_mode(radio_context, false);    // Cold start the radio too.

    // Initialize the power management function of the MCU
    rp2350_power_state_context_t power_context;
    if (!power_mgmt_init_hal(mcu_power_saving_setting, &mcu_all_on, &power_context)) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Failed to initialize the power management functionality of the MCU!", "enter_power_saving_mode");
        return false;
    }

    // Write the power saving flag to the novo memory of the chip to indicate the MCU is waking up from a power management triggered cold start - no POST required
    // Initialize a buffer
    uint32_t novo_buf[MCU_POWMAN_NOVO_ELEMENTS];
    memset(novo_buf, 0x00, (sizeof(novo_buf) / sizeof(uint32_t)));
    
    // Write the wake magic number to the novo buffer, followed by the user-defined contents.
    novo_buf[0] = POWER_SAVING_WAKE_MAGIC_NUMBER;
    memcpy(novo_buf + sizeof(uint32_t), novo_mem_contents, novo_contents_len);
    power_mgmt_write_novo_memory_hal(novo_buf, MCU_POWMAN_NOVO_ELEMENTS);
    
    // Set the MCU in dormant mode
    int dormant_exit_code = power_mgmt_go_dormant_for_time_ms_hal(&power_context, power_saving_duration_ms);
    if (dormant_exit_code < 0) {
        printf("Error code: %d", dormant_exit_code);
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Failed to enter dormant mode!", "enter_power_saving_mode");
        return false;
    }

    return true;
}