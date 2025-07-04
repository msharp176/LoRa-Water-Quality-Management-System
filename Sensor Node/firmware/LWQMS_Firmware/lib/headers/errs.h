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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Error Codes

typedef enum lwqms_errs_e {
    ERR_SPI_TRANSACTION_FAIL = 0,
    ERR_BAD_CRC = 1,
    ERR_LORA_TIMEOUT = 2,
    ERR_ARGUMENT = 3,
    ERR_BAD_SETUP = 4
} lwqms_errs_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Error Severity Levels

typedef enum lwqms_err_severity_e {
    ERR_SEV_FATAL = 0,
    ERR_SEV_REBOOT = 1,
    ERR_SEV_NONFATAL = 2
} lwqms_err_severity_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

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

#endif

/* --- EOF ------------------------------------------------------------------ */