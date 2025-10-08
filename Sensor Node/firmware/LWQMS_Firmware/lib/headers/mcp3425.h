/******************************************************************************************************************** 
*   
*   @file mcp3425.c
*
*   @brief Header File for the MCP3425 16 bit A-D converter with I2C interface driver
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef MCP3425_H
#define MCP3425_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Includes

#include "hal.h"
#include "hardware.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Defines

typedef enum mcp3425_cfg_reg_e {
    MCP3425_CFG_REG_RDY = (1 << 7),
    MCP3425_CFG_REG_CONV_MODE = (1 << 4),
    MCP3425_CFG_REG_SPS_1 = (1 << 3),
    MCP3425_CFG_REG_SPS_0 = (1 << 2),
    MCP3425_CFG_REG_PGA_1 = (1 << 1),
    MCP3425_CFG_REG_PGA_0 = (1 << 0)
} mcp3425_cfg_reg_t;

typedef enum mcp3425_sps_e {
    MCP3425_SPS_240_12BITS = 0,
    MCP3425_SPS_60_14BITS = 1,
    MCP3425_SPS_15_16BITS = 2
} mcp3425_sps_t;

typedef enum mcp3425_pga_e {
    MCP3425_PGA_1 = 0,
    MCP3425_PGA_2 = 1,
    MCP3425_PGA_4 = 2,
    MCP3425_PGA_8 = 3
} mcp3425_pga_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * @brief Initializes the MCP3425 ADC with the given sampling parameters
 * 
 * @param context: ADC implementation details
 * @param sampling_rate: The sampling rate (and corresponding resolution) of the ADC
 * @param gain: PGA gain setting
 * @param continuous_mode_en: Set to TRUE to enable continuous conversion mode, FALSE for one-shot conversion mode.
 * 
 * @returns None
 */
void mcp3425_init(mcp3425_context_t *context, mcp3425_sps_t sampling_rate, mcp3425_pga_t gain, bool continuous_mode_en);

/**
 * @brief Gets a measurement value from the MCP3425 ADC. If the device is in one-shot mode, this triggers a one-shot conversion, and idles until
 * the conversion is complete. For continuous conversion mode, this function does not return until NEW data is read from the ADC.
 * 
 * @param context: ADC implementation details
 * 
 * @returns The voltage reading
 */
double mcp3425_get_measurement(mcp3425_context_t *context);

/**
 * @brief Reads the configuration parameters from the ADC and writes their values to the mcp3425 context structure in-place.
 * 
 * @param context: ADC implementation details
 * 
 * @returns None
 */
void mcp3425_get_params(mcp3425_context_t *context);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------



#endif /* MCP3425_H */

/* --- EOF ------------------------------------------------------------------ */