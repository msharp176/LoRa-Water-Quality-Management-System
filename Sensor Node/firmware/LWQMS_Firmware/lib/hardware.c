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

i2c_context_t context_i2c_0 = {
    .baud = 100000,
    .inst = i2c0,
    .scl = GP5,
    .sda = GP4,
};

i2c_context_t context_i2c_1 = {
    .baud = 100000,
    .inst = i2c1,
    .scl = GP27,
    .sda = GP26
};

const gpio_driven_irq_context_t irq_radio_0 = {
    .pin = GP8,
    .source_mask = GPIO_IRQ_EDGE_RISE,
    .callback = &sx126x_master_isr
};

sx126x_context_t radio_0 = {
    .busy = GP9,
    .irq_context = &irq_radio_0,
    .rst =  GP10,
    .cs =   GP11,
    .spi_context = &context_spi_0, // SPI Bus 0
    .radio_operation_timeout_us = RADIO_TIMEOUT_GLOBAL_US,
    .designator = "RADIO 0",
    .tx_buf_start = 0x00,
    .rx_buf_start = 0x00
};

mcp4651_context_t context_digipot_offset = {
    .addr = 0x28,
    .base_resistance = 50000,
    .i2c_context = &context_i2c_1,
    .wiper_position_a = 0x80,
    .wiper_position_b = 0x80,
};

mcp4651_context_t context_digipot_gain = {
    .addr = 0x2a,
    .base_resistance = 50000,
    .i2c_context = &context_i2c_1,
    .wiper_position_a = 0x80,
    .wiper_position_b = 0x80
};

mcp4651_context_t context_digipot_reference = {
    .addr = 0x2b,
    .base_resistance = 50000,
    .i2c_context = &context_i2c_1,
    .wiper_position_a = 0x80,
    .wiper_position_b = 0x80
};

mcp3425_context_t context_adc = {
    .addr = 0x68,
    .i2c_context = &context_i2c_1,
};

mxl23l3233f_context_t context_flash_0 = {
    .spi_context = &context_spi_0,
    .cs = GP7,
    .size = 32000000,
    .sectors = 1024,
    .blocks_32kB = 128,
    .blocks_64kB = 64
};

tmux1309_context_t context_mux_0 = {
    .enable = GP2,
    .sel0 = GP3,
    .sel1 = GP4
};

uint8_t err_led = ERROR_LED;