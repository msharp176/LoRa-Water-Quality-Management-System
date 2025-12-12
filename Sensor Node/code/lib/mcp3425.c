/******************************************************************************************************************** 
*   
*   @file mcp3425.c
*
*   @brief Driver for the MCP3425 16 bit A-D converter with I2C interface
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "mcp3425.h"

static double mcp3425_convert_code_to_voltage(uint16_t raw_data, mcp3425_sps_t sampling_rate, mcp3425_pga_t gain_setting) {
        
    // Check the sign bit
    if ((raw_data & (1 << 15)) > 0) {
        // We are negative. Take the 2's complement of the data.
        raw_data = ~raw_data + 1;
    }

    // First, divide by the PGA setting. These are all powers of 2, so a right-shift will do.
    raw_data = raw_data >> gain_setting;

    double volts_per_lsb;

    switch (sampling_rate) {
        case MCP3425_SPS_240_12BITS:
            volts_per_lsb = 0.001;      // 1 mV
            break;
        case MCP3425_SPS_60_14BITS:
            volts_per_lsb = 0.000250;   // 250 uV
            break;
        case MCP3425_SPS_15_16BITS:
            volts_per_lsb = 0.0000625;  // 62.5 uV
            break;
    }

    // Convert to a double by multiplying by the LSB.
    double voltage = (double)raw_data * volts_per_lsb;

    return voltage;
}

bool mcp3425_init(mcp3425_context_t *context, mcp3425_sps_t sampling_rate, mcp3425_pga_t gain, bool continuous_mode_en) {
    // Note that this call does not initialize the i2c bus

    // Capture:  prepend with zeroes   last bit                  bottom 2 bits              bottom 2 bits
    uint8_t cfg_reg = 0x00 | ((continuous_mode_en & 1) << 4) | ((sampling_rate & 3) << 2) | (gain & 3);

    bool cfg_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // Write the byte to the configuration register
            if (i2c_write_hal(context->i2c_context, context->addr, &cfg_reg, 1) < 0) break;
            
            // Read the configuration byte back
            uint8_t rxBuf[3];
            if (i2c_read_hal(context->i2c_context, context->addr, rxBuf, 3) < 0) break;
            
            // Check the configuration register against the written byte
            uint8_t readback_config = rxBuf[2] & 0x1f;
        
            if (readback_config == cfg_reg) {
                cfg_ok = true;
            }
        } while (0);

        if (cfg_ok) {
            // Write the params to the context by performing a sanity-check read. This also enables secondary validation outside this function's scope.
            mcp3425_get_params(context);
            return true;
        }
    }

    err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to configure the A-D converter!", "mcp3425_init");

    return false;
}

bool mcp3425_get_measurement(mcp3425_context_t *context, double *reading) {

    /**
     * Algorithm:
     * 1. Read the configuration register. Using the OC byte, determine if we are in one-shot or continuous conversion mode.
     * 2. If we are in one-shot mode, write a 1 to RDY (keeping other bits intact) to initiate a conversion.
     * 3. Keep reading from the device until RDY = 0. Process the corresponding data and return it.
     */

    bool measure_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // 1. Read the configuration register
            uint8_t rxBuf[3];
            if (i2c_read_hal(context->i2c_context, context->addr, rxBuf, 3) < 0) break;

            // Extract the configuration register
            uint8_t cfg_reg = rxBuf[2];

            // Determine the mode and other characteristics about the device
            bool one_shot_conversion_mode_is_set = (cfg_reg & MCP3425_CFG_REG_CONV_MODE) == 0;                 // If OC bit is 1 then we are in continuous conversion mode
            mcp3425_sps_t sampling_rate = (cfg_reg & (MCP3425_CFG_REG_SPS_1 | MCP3425_CFG_REG_SPS_0)) >> 2;
            mcp3425_pga_t gain_setting = (cfg_reg & (MCP3425_CFG_REG_PGA_1 | MCP3425_CFG_REG_PGA_0));

            if (one_shot_conversion_mode_is_set) {
                // If we are in one-shot conversion mode, initiate a conversion by writing a one to the RDY bit of the configuration register
                cfg_reg |= MCP3425_CFG_REG_RDY;
                if (i2c_write_hal(context->i2c_context, context->addr, &cfg_reg, 1) < 0) break;
            }

            // Now, wait until the conversion is finished. Once a zero is read back on the RDY bit of the configuration register, send back the fresh data.
            while ((cfg_reg & MCP3425_CFG_REG_RDY) != 0) {
                // Read the data and store it in the pre-allocated rx buffer.
                if (i2c_read_hal(context->i2c_context, context->addr, rxBuf, 3) < 0) break;

                // Update the configuration register
                cfg_reg = rxBuf[2];
            } 

            // Return the fresh data
            *reading = mcp3425_convert_code_to_voltage(((rxBuf[0] << 8) | rxBuf[1]), sampling_rate, gain_setting);

            measure_ok = true;
        } while (0);

        if (measure_ok) return true;
    }

    err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to get measurement from ADC!", "mcp3425_get_measurement");

    return false;
}

void mcp3425_get_params(mcp3425_context_t *context) {
    // 1. Read the configuration register
    uint8_t rxBuf[3];
    i2c_read_hal(context->i2c_context, context->addr, rxBuf, 3);

    // Extract the configuration register
    uint8_t cfg_reg = rxBuf[2];

    // Determine the mode and other characteristics about the device
    context->continuous_conversion_mode_enabled = (cfg_reg & MCP3425_CFG_REG_CONV_MODE) > 0;                 // If OC bit is 1 then we are in continuous conversion mode
    context->sampling_rate = (cfg_reg & (MCP3425_CFG_REG_SPS_1 | MCP3425_CFG_REG_SPS_0)) >> 2;
    context->gain = (cfg_reg & (MCP3425_CFG_REG_PGA_1 | MCP3425_CFG_REG_PGA_0));
}