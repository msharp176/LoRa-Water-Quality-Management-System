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

static uint8_t mcp4651_construct_command_byte(mcp4651_memory_addresses_t address, mcp4651_operation_types_t operation, uint16_t data) {
    /**
     * The structure of the command byte looks like this:
     * AAAACCDD
     * 
     * where:
     * A = memory address
     * C = command (operation) type
     * D = MSB0 and MSB1 of the following data byte.
     * 
     * To set wiper A to position 256, you would have:
     *      00000001 and then 00000000 data. Bit 9 of 0x100 carries over to the last position in the command byte 
     */

    return (address << 4) | (operation << 2) | (data >> 8);
}

int mcp4651_set_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper, uint16_t position) {

    if (position > 256) {
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

    const uint8_t txBuf[2] = {command_byte, position};

    return (i2c_write_hal(context->i2c_context, context->addr, txBuf, 2) > 0) ? position : -1;
}

int mcp4651_increment_wiper(mcp4651_context_t *context, mcp4651_wipers_t wiper) {
    uint8_t command_byte;

    switch (wiper) {
        case MCP4651_WIPER_A:
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_0, MCP4651_OPERATION_INCREMENT, 0);
            context->wiper_position_a++;
            break;
        case MCP4651_WIPER_B:
            command_byte = mcp4651_construct_command_byte(MCP4651_MEMORY_ADDR_WIPER_1, MCP4651_OPERATION_INCREMENT, 0);
            context->wiper_position_b++;
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