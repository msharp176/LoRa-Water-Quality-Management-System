/*************************************************************************************
 * 
 * @file mcp4651.c
 * 
 * @brief Driver for the MCP4651 Digital Potentiometer with I2C interface.
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 * @remark Likely compatible with MCP(453X, 455X, 463X, 465X) family of devices, however untested. 
 *
 * ************************************************************************************/

#include "mcp4651.h"

static uint8_t mcp4651_construct_command_byte(
    mcp4651_memory_addresses_t address,
    mcp4651_operation_types_t operation,
    uint16_t data
){
    return ((address   & 0x0F) << 4) |
           ((operation & 0x03) << 2) |
           ((data >> 8) & 0x03);
}


int mcp4651_set_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper, uint16_t position) {

    if (position > MCP4651_MAX_WIPER_INDEX) {
        char err_msg[256];
        snprintf(err_msg, 256, "Invalid wiper position requested for MCP4651: %d", position);
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, err_msg, "mcp4651_set_wiper");
        return -1;
    }

    uint8_t command_byte;

    switch (wiper) {
        case MCP4651_WIPER_A:
            // Get the command byte based on the configuration data provided
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_0, MCP4651_OPERATION_WRITE, position);
            
            // Update the wiper position for logging purposes
            context->wiper_position_a = position;

            break;
        case MCP4651_WIPER_B:
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_1, MCP4651_OPERATION_WRITE, position);
            context->wiper_position_b = position;
            break;
        case MCP4651_WIPER_BOTH:
            // Yeah yeah somewhat un-elegant and also not sure if this counts as recursive. Biden says no recursion.
            bool set_both_wipers_ok = false;
            do {
                if (mcp4651_set_wiper(context, MCP4651_WIPER_A, position) < 0) break;
                if (mcp4651_set_wiper(context, MCP4651_WIPER_B, position) < 0) break;
                set_both_wipers_ok = true;
            } while (0);

            // No need to do any i2c interaction HERE. This is done by re-calling the function anywho.
            return set_both_wipers_ok ? position : -1;
    }

    
    uint8_t data_byte = position & 0xff;    // Chop off the top 8 bits.
    
    const uint8_t txBuf[2] = {command_byte, data_byte};
    
    /*
    printf("[DEBUG] I2C write for %s with command_byte 0x%02X and data_byte 0x%02X\n",
           (wiper == MCP4651_WIPER_A ? "Wiper A" : (wiper == MCP4651_WIPER_B ? "Wiper B" : "Both")),
           command_byte, data_byte);

    */
   
    return (i2c_write_hal(context->i2c_context, context->addr, txBuf, 2) > 0) ? position : -1;
}

int mcp4651_increment_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper) {
    uint8_t command_byte;

    switch (wiper) {
        case MCP4651_WIPER_A:
            context->wiper_position_a++;
            if (context->wiper_position_a > MCP4651_MAX_WIPER_INDEX) {
                context->wiper_position_a = MCP4651_MAX_WIPER_INDEX;
                return -1;
            }
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_0, MCP4651_OPERATION_INCREMENT, 0);
            break;
        case MCP4651_WIPER_B:
            context->wiper_position_b++;
            if (context->wiper_position_b > MCP4651_MAX_WIPER_INDEX) {
                context->wiper_position_b = MCP4651_MAX_WIPER_INDEX;
                return -1;
            }
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_1, MCP4651_OPERATION_INCREMENT, 0);
            break;
        case MCP4651_WIPER_BOTH:
            bool increment_both_ok = false;
            do {
                if (mcp4651_increment_wiper(context, MCP4651_WIPER_A) < 0) break;
                if (mcp4651_increment_wiper(context, MCP4651_WIPER_B) < 0) break;
                increment_both_ok = true;
            } while (0);

            return increment_both_ok ? context->wiper_position_a : -1;
    }

    if (i2c_write_hal(context->i2c_context, context->addr, &command_byte, 1) > 0) {
        return (wiper == MCP4651_WIPER_A) ? context->wiper_position_a : context->wiper_position_b;
    }
    else {
        return -1;
    }
}

int mcp4651_decrement_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper) {
    uint8_t command_byte;

    switch (wiper) {
        case MCP4651_WIPER_A:
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_0, MCP4651_OPERATION_DECREMENT, 0);
            context->wiper_position_a--;
            break;
        case MCP4651_WIPER_B:
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_1, MCP4651_OPERATION_DECREMENT, 0);
            context->wiper_position_b--;
            break;
        case MCP4651_WIPER_BOTH:
            bool decrement_both_ok = false;
            do {
                if (mcp4651_decrement_wiper(context, MCP4651_WIPER_A) < 0) break;
                if (mcp4651_decrement_wiper(context, MCP4651_WIPER_B) < 0) break;
                decrement_both_ok = true;
            } while (0);

            return decrement_both_ok ? context->wiper_position_a : -1;
    }

    if (i2c_write_hal(context->i2c_context, context->addr, &command_byte, 1) > 0) {
        return (wiper == MCP4651_WIPER_A) ? context->wiper_position_a : context->wiper_position_b;
    }
    else {
        return -1;
    }}

int mcp4651_dummy_command(mcp4651_context_t *context)
{
    // Dummy write to TCON register with no electrical effect.
    // This resets any lingering increment/decrement/read state
    // inside the MCP4651 command parser.

    const uint16_t dummy_data = 0x00FF;   // TCON default: all switches ON (no effect)
    
    // Build command byte using your existing helper
    uint8_t command_byte = mcp4651_construct_command_byte(
                                MCP4651_MEMORY_ADDR_TCON_REG,
                                MCP4651_OPERATION_WRITE,
                                dummy_data
                           );

    // Low byte of data
    uint8_t data_byte = dummy_data & 0xFF;

    // Prepare I2C buffer (command + data)
    const uint8_t txBuf[2] = { command_byte, data_byte };

    // Transmit to device — return 0 on success
    return (i2c_write_hal(context->i2c_context, context->addr, txBuf, 2) > 0) 
            ? 0 
            : -1;
}


int mcp4651_disable(mcp4651_context_t *context) {
    
    uint8_t command_byte_write = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_TCON_REG, MCP4651_OPERATION_WRITE, 0x00);
    
    uint8_t txBuf[2] = { command_byte_write, 0x00 };

    return (i2c_write_hal(context->i2c_context, context->addr, txBuf, 2) > 0) ? 0 : -1;
}

int mcp4651_enable(mcp4651_context_t *context) {

    uint8_t command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_TCON_REG, MCP4651_OPERATION_WRITE, 0x01ff);

    uint8_t txBuf[2] = {command_byte, 0xff};

    return (i2c_write_hal(context->i2c_context, context->addr, txBuf, 2) > 0) ? 0 : -1;

}