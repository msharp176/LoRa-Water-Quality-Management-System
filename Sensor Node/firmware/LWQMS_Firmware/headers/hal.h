/*************************************************************************************
 * 
 * @file hal.h
 * 
 * @brief Global Hardware Abstraction Layer (HAL) Header File for LoRa Water Quality Management System Sensor Node Firmware 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#pragma once

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "main.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

/**
 * 
 * @brief Struct containing all data necessary to define a single SPI instance within the HAL
 * 
 */
typedef struct {
    spi_inst_t* inst;
    uint8_t     mosi;
    uint8_t     miso;
    uint8_t     sck;
    uint32_t    baud;
    uint8_t     xfer_bits;
    uint8_t     polarity;
    uint8_t     phase;
    uint8_t     lsb_msb_first;
} spi_context_t;

/**
 * 
 * @brief Sets up a GPIO pin within the HAL
 * 
 * @param  gpio_context     (uint8_t) GPIO pin identifier information
 * @param  IsOutput         Specify if the pin is an output or input.
 * 
 * @returns None
 * 
 */
void gpio_setup_hal(const void *gpio_context, bool IsOutput);

/**
 * 
 * @brief De-Initializes a GPIO pin within the HAL
 * 
 * @param gpio_context (uint8_t) GPIO pin identifier information
 * 
 * @returns None
 * 
 */
void gpio_terminate_hal(const void *gpio_context);

/**
 * 
 * @brief Writes a boolean status to a GPIO pin using the HAL
 * 
 * @param  gpio_context     (uint8_t) GPIO pin identifier information
 * @param  state            Desired State (HIGH/LOW) of GPIO pin
 * 
 * @returns None
 * 
 */
void gpio_write_hal(const void *gpio_context, bool state);

/**
 * 
 * @brief Reads the current state of a GPIO pin using the HAL
 * 
 * @param  gpio_context: (uint8_t) GPIO pin identifier information
 * 
 * @returns The state of the GPIO pin
 * 
 */
bool gpio_read_hal(const void *gpio_context);

/**
 * 
 * @brief Initialize the given SPI context using the HAL
 * 
 * @param spi_context: (spi_context_t) Implementation Information for the desired SPI instance to initialize
 * 
 * @returns The baud rate of the SPI instance
 * 
 */
uint spi_init_hal(const void *spi_context);

/**
 * 
 * @brief Deinitialize the given SPI context using the HAL
 * 
 * @param spi_context: (spi_context_t) Implementation Information for the desired SPI instance to initialize
 * 
 * @returns None
 * 
 */
void spi_terminate_hal(const void* spi_context);

/**
 * 
 * @brief Write data over SPI using the HAL
 * 
 * @param spi_context: (spi_context_t) Implementation Information for the desired SPI instance to use
 * @param data:        The data to be written
 * @param len:         The number of bytes to be written
 * 
 * @returns The number of bytes written.
 * 
 */
uint spi_write_hal(const void *spi_context, char * data, int len);

/**
 * 
 * @brief Read data from the SPI bus using the HAL
 * 
 * @param spi_context   (spi_context_t) Implementation Information for the desired SPI instance to use
 * @param buf:          Buffer to store read bytes
 * @param len:          The number of bytes to be read
 * 
 * @returns The number of bytes read.
 * 
 */
uint spi_read_hal(const void *spi_context, char * buf, int len);

/**
 * 
 * @brief Writes a buffer to the SPI bus, and reads data from that same bus simultaneously using the HAL.
 * 
 * @param spi_context: (spi_context_t) Implementation Information for the desired SPI instance to use
 * @param txData:      Buffer containing bytes to write to the SPI bus. This buffer will be overwritten in-place by the bytes read back in
 * @param len:         The number of bytes to be written/read
 * 
 * @returns The number of bytes written/read
 * 
 */
uint spi_rw_hal(const void *spi_context, char * txData, int len);