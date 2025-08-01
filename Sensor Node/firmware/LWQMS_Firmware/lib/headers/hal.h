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

#ifndef HAL_H
#define HAL_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes

#include "global_defs.h"
#include "hardware.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// C Standard Headers

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Pico SDK Includes

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "hardware/irq.h"
#include "hardware/resets.h"
#include "tusb.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// External Dependencies

extern void err_raise(lwqms_errs_t err_code, lwqms_err_severity_t severity, char * err_msg, char * err_context);

extern void isr_gpio_master(uint gpio_pin, uint32_t irq_src);

extern void register_gpio_isr(gpio_driven_irq_context_t *context);

extern void unregister_gpio_isr(gpio_driven_irq_context_t *context);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion


#pragma region GPIO

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// GPIO Functions

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
 * @brief Convenience method to invert the current state of a GPIO pin
 * 
 * @param  gpio_context: (uint8_t) GPIO pin identifier information
 * 
 * @returns The final state of the gpio pin
 * 
 */
bool gpio_toggle_hal(const void *gpio_context);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region SPI

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// SPI

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

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region I2C

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// I2C

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region Watchdog

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Watchdog & Reboot

/**
 * 
 * @brief Reboots the MCU using the watchdog timer
 * 
 */
void reboot(void);

/**
 * 
 * @brief Checks if the MCU is booting up after a reboot command was issued or a clean boot
 * 
 * @returns True if the MCU is booting due to a reboot command, false for a clean boot
 * 
 */
bool check_if_rebooted_or_clean_boot(void);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region Interrupts

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Interrupts

/**
 * @brief Attach an interrupt to a GPIO pin. GPIO pin must be setup first using `gpio_setup_hal`
 * 
 * @remark Use this function to attach a gpio-driven interrupt. Do not manually register the interrupt, this function handles that operation.
 * 
 * @param irq_context pointer to `gpio_driven_irq_context_t` struct
 * 
 * @returns None
 */
void gpio_irq_attach_hal(const void *irq_context);

/**
 * @brief Detach an interrupt from a specified GPIO pin
 * 
 * @param irq_context pointer to `gpio_driven_irq_context_t` struct
 * 
 * @returns None 
 */
void gpio_irq_detach_hal(const void *irq_context);

/**
 * @brief Acknowledge an interrupt raised by a specified GPIO pin
 * 
 * @param irq_context pointer to `gpio_driven_irq_context_t` struct
 * 
 * @returns None
 */
void gpio_irq_ack_hal(const void *irq_context);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region USB Console

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// USB Console

/**
 * @brief Initialize the USB Console port
 * 
 * @returns None
 */
void init_usb_console_hal(void);

/**
 * @brief Close the USB Console port
 * 
 * @returns None
 */
void deinit_usb_console_hal(void);

/**
 * @brief Checks if a USB console is connected
 * 
 * @returns The console connection status
 */
bool is_usb_console_connected_hal(void);

/**
 * @brief Checks if a USB console is available
 * 
 * @related [is_usb_console_connected_hal]
 * 
 * @returns The console availability status
 */
bool is_usb_console_available_hal(void);

/**
 * @brief Waits for a USB console to be connected. Will not return until one is detected
 * 
 * @returns Console connection status
 */
bool wait_for_usb_console_connection_hal(void);

/**
 * @brief Wait for a USB console port to be connected with a timeout
 * 
 * @param timeout_ms Timeout in milliseconds
 * 
 * @returns True for a connected console, false if timeout
 */
bool wait_for_usb_console_connection_with_timeout_hal(uint32_t timeout_ms);

/**
 * @brief Retrieve a character from the usb console
 * 
 * @returns The character retrieved
 */
char usb_console_getchar_hal(void);

/**
 * @brief Write a single character to the USB console
 * 
 * @param c the character to write
 */
int usb_console_putchar_hal(char c);

/**
 * @brief Retrieve a character from the usb console with timeout
 * 
 * @param timeout_microseconds Timeout in microseconds
 * 
 * @returns The character retrieved
 */
char usb_console_getchar_timeout_us_hal(uint32_t timeout_microseconds);

/**
 * @brief Write a buffer to the usb console
 * 
 * @param buf The data to be written
 * 
 * @returns The number of characters written. If less than zero, the write failed.
 */
int usb_console_write_hal(char * buf);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#endif

/* --- EOF ------------------------------------------------------------------ */