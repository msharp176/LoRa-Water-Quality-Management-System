/******************************************************************************************************************** 
*   
*   @file hardware.c
*
*   @brief Global Hardware Context Types and Definitions for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "hardware.h"

extern void sx126x_master_isr(gpio_driven_irq_context_t *context);

spi_context_t context_spi_0 = {
    .inst = spi0,
    .baud = SPI_FREQ_GLOBAL,
    .miso = GP16,
    .mosi = GP19,
    .sck = GP18,
    .phase = SPI_CPHA_0,            // Standard Motorola/Freescale SPI setup
    .polarity = SPI_CPOL_0,
    .xfer_bits = 8,
    .lsb_msb_first = SPI_MSB_FIRST
};

const gpio_driven_irq_context_t irq_radio_0 = {
    .pin = RADIO_DIO1,
    .source_mask = GPIO_IRQ_EDGE_RISE,
    .callback = &sx126x_master_isr
};

sx126x_context_t radio_0 = {
    .busy = RADIO_BUSY,
    .irq_context = &irq_radio_0,
    .rst =  RADIO_RESET,
    .cs =   RADIO_CS,
    .spi_context = &context_spi_0, // SPI Bus 0
    .radio_operation_timeout_us = RADIO_TIMEOUT_GLOBAL_US,
    .designator = "RADIO 0"
};
uint8_t err_led = ERROR_LED;