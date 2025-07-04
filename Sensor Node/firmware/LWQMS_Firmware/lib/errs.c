/******************************************************************************************************************** 
*   
*   @file errs.c 
*
*   @brief Function and Type Definitions for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "errs.h"
#include "hal.h"
#include "main.h"
#include <stdio.h>
#include "pico/stdlib.h"

void err_raise(lwqms_errs_t err_code, lwqms_err_severity_t severity, char * err_msg, char * err_context) {
    
    uint8_t err_led = ERROR_LED;

    char err_info[100];
    
    gpio_setup_hal(&err_led, true);
    gpio_write_hal(&err_led, GPIO_HIGH);

    switch (severity) {
        case ERR_SEV_FATAL:
            sprintf(err_info, "ERROR [%d] (FATAL) Encountered in %s: %s", err_code, err_context, err_msg);
            log_error(err_info);
            while (true) {
                // Idle
            }
            break;
        case ERR_SEV_REBOOT:
            sprintf(err_info, "ERROR [%d] (reboot required) Encountered in %s: %s", err_code, err_context, err_msg);
            log_error(err_info);
            reboot();
        case ERR_SEV_NONFATAL:
            sprintf(err_info, "ERROR [%d] (non-fatal) Encountered in %s: %s", err_code, err_context, err_msg);
            log_error(err_info);
            for (int k = 0; k < 8; k++) {
                gpio_write_hal(&err_led, GPIO_HIGH);
                sleep_ms(250);
                gpio_write_hal(&err_led, GPIO_LOW);
                sleep_ms(250);
            }
        default:
            // Do nothing
    }

    gpio_write_hal(&err_led, GPIO_LOW);
}

void err_clear() {

    uint8_t err_led = ERROR_LED;

    log_error("Errors cleared");
    gpio_setup_hal(&err_led, true);
    gpio_write_hal(&err_led, GPIO_LOW);
}

void log_error(char * err_msg_full) {
    #ifdef DEBUG
        printf("%s\n", err_msg_full);
    #else 
        // Do nothing for now
    #endif
}