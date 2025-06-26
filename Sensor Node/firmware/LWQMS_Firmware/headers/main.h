/******************************************************************************************************************** 
*   
*   @file main.h 
*
*   @brief Function and Type Definitions for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Debug Logging

#define DEBUG

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#pragma once
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hal.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Definitions & Aliases

#pragma region GPIO

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

#define RADIO_TIMEOUT_GLOBAL_US 10000 // 10 ms
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

typedef struct sx126x_context {
    spi_context_t* spi_context;
    uint8_t        rst;                         // Reset - Pull LOW for 100us to initiate a reset.
    uint8_t        busy;                        // Busy Indicator - HIGH indicates that sx126x is busy and can not be written to
    uint8_t        irq;                         // Interrupt - DIO1 asserted HIGH on interrupt
    uint32_t       radio_operation_timeout_us;  // Timeout to wait for radio to leave busy state
    uint8_t        cs;                          // Chip select GPIO Pin.
    char *         designator;                  // Radio designator string
} sx126x_context_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

/* --- EOF ------------------------------------------------------------------ */