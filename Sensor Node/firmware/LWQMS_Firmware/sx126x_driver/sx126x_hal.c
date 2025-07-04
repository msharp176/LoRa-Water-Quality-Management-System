/*************************************************************************************
 * 
 * @file sx126x_hal.c
 * 
 * @brief Implementation of the sx126x Hardware Abstraction Layer (HAL) for the Raspberry Pi Pico
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "sx126x_hal.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "main.h"
#include "pico/time.h"
#include "hal.h"
#include <stdio.h>

#define CHECK_RADIO_BUSY(CONTEXT) if (wait_for_radio_ready(CONTEXT) == SX126X_HAL_STATUS_ERROR) return SX126X_HAL_STATUS_ERROR

#pragma region SPI

sx126x_hal_status_t sx126x_hal_write(   const void* context, 
                                        const uint8_t* command, 
                                        const uint16_t command_length,
                                        const uint8_t* data, 
                                        const uint16_t data_length 
)   {

    // Cast the void pointer context to the sx126x context type
    const sx126x_context_t* radio_inst = (const sx126x_context_t*)context;

    // Check if the radio is busy
    CHECK_RADIO_BUSY(context);

    // Assert Chip Select
    gpio_write_hal(&(radio_inst->cs), GPIO_LOW);

    // Write the command
    if (command_length > 0) {
        spi_write_hal(radio_inst->spi_context, command, command_length);
    }

    // Write the data
    if (data_length > 0) {
        spi_write_hal(radio_inst->spi_context, data, data_length);
    }

    // De-Assert Chip Select
    gpio_write_hal(&(radio_inst->cs), GPIO_HIGH);

    // Return a success
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_read(const void* context, 
                                    const uint8_t* command, 
                                    const uint16_t command_length, 
                                    uint8_t* data, 
                                    const uint16_t data_length 
) {
    
    // Cast the void pointer context to the sx126x context type
    const sx126x_context_t* radio_inst = (const sx126x_context_t*)context;

    // Check if the radio is busy
    CHECK_RADIO_BUSY(context);

    // Assert Chip Select
    gpio_write_hal(&(radio_inst->cs), GPIO_LOW);

    // Write the command
    if (command_length > 0) {
        spi_write_hal(radio_inst->spi_context, command, command_length);
    }

    // Read back data (data will not be shifted out until posedge SCK)
    if (data_length > 0) {
        spi_read_hal(radio_inst->spi_context, data, data_length);
    }

    // De-Assert Chip Select
    gpio_write_hal(&(radio_inst->cs), GPIO_HIGH);

    // Return a success
    return SX126X_HAL_STATUS_OK;
}

#pragma endregion

#pragma region GENERAL OPS

sx126x_hal_status_t sx126x_hal_reset( const void* context ) {
    
    // Cast the void pointer context to the sx126x context type
    const sx126x_context_t* radio_inst = (const sx126x_context_t*)context;

    #ifdef DEBUG
        printf("Resetting %s\n", radio_inst->designator);
    #endif

    // Assert the RESET Pin LOW
    gpio_write_hal(&(radio_inst->rst), GPIO_LOW);

    // Hold RESET LOW for 100us
    sleep_us(150);  // to be safe

    // De-Assert RESET Pin
    gpio_write_hal(&(radio_inst->rst), GPIO_HIGH);

    // Wait for the radio to no longer be busy
    CHECK_RADIO_BUSY(context);

    // Return an OK status
    return SX126X_HAL_STATUS_OK;
}

sx126x_hal_status_t sx126x_hal_wakeup( const void* context ) {
    // Cast the void pointer context to the sx126x context type
    const sx126x_context_t* radio_inst = (const sx126x_context_t*)context;

    #ifdef DEBUG
        printf("Waking up %s\n", radio_inst->designator);
    #endif

    // Check if the radio is busy
    CHECK_RADIO_BUSY(context);   // In case this function gets called when the sx126x is not in SLEEP mode - should not be HIGH under normal operation.

    // Pull Chip Select LOW to wakeup the chip
    gpio_write_hal(&(radio_inst->cs), GPIO_LOW);

    // Wait for the radio to enter STDBY_RC mode
    CHECK_RADIO_BUSY(context);

    return SX126X_HAL_STATUS_OK;
}

#pragma endregion

#pragma region HELPERS

// Convenience method to prevent lengthy and repetitive HAL code.
sx126x_hal_status_t wait_for_radio_ready(const void* context) {

    // First, cast the void pointer context to a radio context
    const sx126x_context_t* radio_inst = (const sx126x_context_t *)context;

    #ifdef DEBUG
        printf("Checking if %s busy...", radio_inst->designator);
    #endif

    absolute_time_t tout = make_timeout_time_us(radio_inst->radio_operation_timeout_us);
    while (gpio_read_hal(&(radio_inst->busy))) {
        if (get_absolute_time() > tout) {
            
            #ifdef DEBUG
                printf("TIMEOUT\n");
            #endif
            
            return SX126X_HAL_STATUS_ERROR;
        }
    }

    #ifdef DEBUG
        printf("OK\n");
    #endif

    return SX126X_HAL_STATUS_OK;
}

void sx126x_hal_init(const void* context) {

    // First, cast the void pointer context to a radio context
    const sx126x_context_t* radio_inst = (const sx126x_context_t *)context;

    /* Initialize all GPIO pins */

    // Outputs to Radio
    gpio_setup_hal(&(radio_inst->cs), true);
    gpio_setup_hal(&(radio_inst->rst), true);

    // Inputs from radio
    gpio_setup_hal(&(radio_inst->busy), false);
    gpio_setup_hal(&(radio_inst->irq), false);

    // Setup the SPI instance
    uint baud = spi_init_hal(radio_inst->spi_context);

    // Put initial values on the output pins
    gpio_write_hal(&(radio_inst->cs), GPIO_HIGH);
    gpio_write_hal(&(radio_inst->rst), GPIO_HIGH);

    // Register the Radio with the master IRQ handler
    sx126x_register_radio_irq_pin(radio_inst);

    #ifdef DEBUG
        printf("Initialized %s GPIO Pins and SPI interface at %u Hz\n", radio_inst->designator, baud);
    #endif

    return;
}

sx126x_status_t sx126x_hal_initialize_radio(const void* context) {

    bool init_ok = false;

    for (int k = 0; k < SPI_RETRIES; k++) {

        do {
            // 1. Perform a reset of the radio module
            if (sx126x_hal_reset(context) != SX126X_STATUS_OK)                          break;
        
            // 2. Wakeup the radio
            if (sx126x_hal_wakeup(context) != SX126X_STATUS_OK)                         break;
        
            // 3. Set the regulator mode
            if (sx126x_set_reg_mode(context, SX126X_REG_MODE_DCDC) != SX126X_STATUS_OK) break;
        
            // 4. Set DIO2 to control the RF switch
            if (sx126x_set_dio2_as_rf_sw_ctrl(context, true) != SX126X_STATUS_OK)       break;

            // 5. Set DIO3 to control the TCXO
            if (sx126x_set_dio3_as_tcxo_ctrl(context, SX126X_TCXO_CTRL_3_3V, SX126X_TCXO_TIMEOUT) != SX126X_STATUS_OK)  break;

            // 6. Calibrate the radio
            if (sx126x_cal(context, SX126X_CAL_ALL) != SX126X_STATUS_OK)                break;

            init_ok = true;

        } while (0);

        if (init_ok) {
            return SX126X_STATUS_OK;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X module during radio initialization", "sx126x_hal_initialize_radio");

    return SX126X_STATUS_ERROR;

}

#pragma endregion

/* --- EOF ------------------------------------------------------------------ */