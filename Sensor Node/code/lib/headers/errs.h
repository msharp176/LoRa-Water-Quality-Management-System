/******************************************************************************************************************** 
*   
*   @file errs.h 
*
*   @brief LoRa Water Quality Management System Error Code Definitions and Error Handling Functions
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef ERRS_H
#define ERRS_H

#include <stdio.h>
#include "hardware.h"
#include "pico/stdlib.h"
#include "system_config.h"
#include "hal.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * 
 * @brief Raises an error
 * 
 * @param err_code:     The error code
 * @param severity:     `lwqms_err_severity_e` error severity
 * @param err_msg:      Corresponding error message
 * @param err_context:  Identifying information about where error occurred
 * 
 * @returns None
 * 
 */
void err_raise(lwqms_errs_t err_code, lwqms_err_severity_t severity, char * err_msg, char * err_context);

/**
 * 
 * @brief Clear all errors
 * 
 * @returns None
 * 
 */
void err_clear(void);

/**
 * @brief Log an error
 * 
 * @param err_msg_full The message to be directed to stderr
 * 
 * @returns None
 */
void log_error(char * err_msg_full);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* ERRS_H */

/* --- EOF ------------------------------------------------------------------ */