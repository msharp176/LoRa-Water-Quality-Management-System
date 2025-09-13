/*************************************************************************************
 * 
 * @file mcp4651.h
 * 
 * @brief Header file for MCP4651 Digital Potentiometer with I2C interface driver.
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 * @remark Likely compatible with MCP(453X, 455X, 463X, 465X) family of devices, however untested. 
 *
 * ************************************************************************************/

#ifndef MCP4651_H
#define MCP4651_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Includes

#include "hal.h"
#include "hardware.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Defines

/**
 * @brief MCP4651 Operation Types Enum
 */
typedef enum mcp4651_operation_types_e {
    MCP4651_OPERATION_WRITE     = 0b00,
    MCP4651_OPERATION_INCREMENT = 0b01,
    MCP4651_OPERATION_DECREMENT = 0b10,
    MCP4651_OPERATION_READ      = 0b11,
} mcp4651_operation_types_t;

/**
 * @brief MCP4651 Memory Addresses Enum
 */
typedef enum mcp4651_memory_addresses_e {
    MCP4651_MEMORY_ADDR_WIPER_0    = 0x00,
    MCP4651_MEMORY_ADDR_WIPER_1    = 0x01,
    MCP4651_MEMORY_ADDR_TCON_REG   = 0x04,
    MCP4651_MEMORY_ADDR_STATUS_REG = 0x05,
} mcp4651_memory_addresses_t;

/**
 * @brief MCP4651 Terminal Control Register (TCON) bits enum
 */
typedef enum mcp4651_tcon_reg_e {
    MCP4651_TCON_REG_GCEN = (1 << 8),
    MCP4651_TCON_REG_R1HW = (1 << 7),
    MCP4651_TCON_REG_R1A  = (1 << 6),
    MCP4651_TCON_REG_R1W  = (1 << 5),
    MCP4651_TCON_REG_R1B  = (1 << 4),
    MCP4651_TCON_REG_R0HW = (1 << 3),
    MCP4651_TCON_REG_R0A  = (1 << 2),
    MCP4651_TCON_REG_R0W  = (1 << 1),
    MCP4651_TCON_REG_R0B  = (1 << 0),
} mcp4651_tcon_reg_t;

/**
 * @brief MCP4651 Wiper Selection Enum
 */
typedef enum mcp4651_wipers_e {
    MCP4651_WIPER_A = 0,
    MCP4651_WIPER_B = 1,
    MCP4651_WIPER_BOTH = 2
} mcp4651_wipers_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * @brief Sets one of the wipers on the given MCP4651 Digital Potentiometer to the selected position
 * 
 * @param context: MCP4651 Implementation Information
 * @param wiper: `mcp4651_wipers_e` enum wiper selection
 * @param position: target position, between tap 0 and 256, inclusive.
 * 
 * @returns The wiper position set, or an error code if an error occurs.
 */
int mcp4651_set_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper, uint16_t position);

/**
 * @brief Increments the current position of the selected wiper for the giuven MCP4651 instance.
 *
 * @param context: MCP4651 Implementation Information
 * @param wiper: `mcp4651_wipers_e` enum wiper selection
 *
 * @returns The updated wiper position, or an error code if an error occurs.
 */
int mcp4651_increment_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper);

/**
 * @brief Decrements the current position of the selected wiper for the giuven MCP4651 instance.
 *
 * @param context: MCP4651 Implementation Information
 * @param wiper: `mcp4651_wipers_e` enum wiper selection
 *
 * @returns The updated wiper position, or an error code if an error occurs.
 */
int mcp4651_decrement_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper);

/**
 * @brief Disables both potentiometers on the given MCP4651 instance, disconnecting the terminals from the circuit.
 * 
 * @param context: MCP4651 Implementation Information
 * 
 * @returns 0 for a successful operation, -1 if an error has occurred.
 */
int mcp4651_disable(mcp4651_context_t *context);

/**
 * @brief Enables both potentiometers on the given MCP4651 instance, connecting the terminals to the circuit.
 * 
 * @param context: MCP4651 Implementation Information
 * 
 * @returns 0 for a successful operation, -1 if an error has occurred.
 */
int mcp4651_enable(mcp4651_context_t *context);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif

/* --- EOF ------------------------------------------------------------------ */