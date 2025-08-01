/******************************************************************************************************************** 
*   
*   @file hardware.h
*
*   @brief Global Hardware Context Types and Definitions for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef HARDWARE_H
#define HARDWARE_H

#include "pico/stdlib.h"
#include "hardware/spi.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// C Standard Library Includes



//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Project Includes


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Definitions & Aliases

#define RADIO_BUSY GP6
#define RADIO_RESET GP7
#define RADIO_DIO1 GP2
#define RADIO_CS GP17
#define SPI0_MOSI GP19
#define SPI0_MISO GP16
#define SPI0_SCK GP18

#pragma region GPIO Aliases

#define GPIO_LOW 0
#define GPIO_HIGH 1

#define GP0 0
#define GP1 1
#define GP2 2
#define GP3 3
#define GP4 4
#define GP5 5
#define GP6 6
#define GP7 7
#define GP8 8
#define GP9 9
#define GP10 10
#define GP11 11
#define GP12 12
#define GP13 13
#define GP14 14
#define GP15 15
#define GP16 16
#define GP17 17
#define GP18 18
#define GP19 19
#define GP20 20
#define GP21 21
#define GP22 22
#define GP25 25
#define GP26 26
#define GP27 27
#define GP28 28

#pragma endregion

#define SPI_FREQ_GLOBAL 1000000 // 1 MHz
#define SPI_RETRIES 5

#define RADIO_TIMEOUT_GLOBAL_US 10000 // 10 ms

#define ERROR_LED GP25  // Use the onboard LED for now

#define QTY_GPIO_PINS NUM_BANK0_GPIOS

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

/**
 * 
 * @brief Struct containing all data necessary to define a single SPI instance within the HAL
 * 
 */
typedef struct spi_context_s {
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
 * @brief GPIO driven IRQ configuration data
 */
typedef struct gpio_driven_irq_context_s {
    uint8_t pin;
    uint32_t source_mask;
    void * callback;
} gpio_driven_irq_context_t;

/**
 * @brief sx126x Radio Configuration Data
 */
typedef struct sx126x_context_s {
    gpio_driven_irq_context_t * irq_context;    // The interrupt event trigger of the module.
    spi_context_t* spi_context;                 // The SPI bus of the radio module
    uint8_t        rst;                         // Reset - Pull LOW for 100us to initiate a reset.
    uint8_t        busy;                        // Busy Indicator - HIGH indicates that sx126x is busy and can not be written to
    uint32_t       radio_operation_timeout_us;  // Timeout to wait for radio to leave busy state
    uint8_t        cs;                          // Chip select GPIO Pin.
    char *         designator;                  // Radio designator string
} sx126x_context_t;

/**
 * @brief A handler for a GPIO-driven interrupt
 */
typedef void (*gpio_isr_handler_t)(gpio_driven_irq_context_t*);  // A function pointer to a GPIO ISR handler that accepts a pointer to an IRQ context type as its input.

/**
 * @brief Error Sources for the LoRa Water Quality Management System
 */
typedef enum lwqms_errs_e {
    ERR_SPI_TRANSACTION_FAIL = 0,
    ERR_BAD_CRC = 1,
    ERR_LORA_TIMEOUT = 2,
    ERR_ARGUMENT = 3,
    ERR_BAD_SETUP = 4
} lwqms_errs_t;

/**
 * @brief Error Logging Levels for the LoRa Water Quality Management System
 */
typedef enum lwqms_err_severity_e {
    ERR_SEV_FATAL = 0,
    ERR_SEV_REBOOT = 1,
    ERR_SEV_NONFATAL = 2
} lwqms_err_severity_t;


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Hardware References

extern sx126x_context_t radio_0;

extern spi_context_t context_spi_0;

extern uint8_t err_led;

const extern gpio_driven_irq_context_t irq_radio_0;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif