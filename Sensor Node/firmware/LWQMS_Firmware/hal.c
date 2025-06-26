/*************************************************************************************
 * 
 * @file hal.c
 * 
 * @brief Global Hardware Abstraction Layer (HAL) for LoRa Water Quality Management System Sensor Node Firmware 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "pico/stdio.h"
#include "pico/stdlib.h"
#include "main.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hal.h"
#include <string.h>
#include <stdlib.h>

void gpio_setup_hal(const void *gpio_context, bool IsOutput) {
    const uint8_t* pin = (const uint8_t *)gpio_context;

    gpio_init(*pin);
    gpio_set_dir(*pin, IsOutput);

    return;
}

void gpio_terminate_hal(const void *gpio_context) {
    const uint8_t* pin = (const uint8_t *)gpio_context;
    gpio_deinit(*pin);

    return;
}

void gpio_write_hal(const void *gpio_context, bool state) {

    const uint8_t* pin = (const uint8_t *)gpio_context;

    gpio_put(*pin, state);

}

bool gpio_read_hal(const void *gpio_context) {

    const uint8_t* pin = (const uint8_t *)gpio_context;

    return gpio_get(*pin);
}

uint spi_init_hal(const void *spi_context) {

    const spi_context_t* setup = (const spi_context_t *)spi_context;

    uint baud = spi_init(setup->inst, setup->baud);

    spi_set_format(setup->inst, setup->xfer_bits, setup->polarity, setup->phase, setup->lsb_msb_first);

    gpio_set_function(setup->miso, GPIO_FUNC_SPI);
    gpio_set_function(setup->mosi, GPIO_FUNC_SPI);
    gpio_set_function(setup->sck, GPIO_FUNC_SPI);

    return baud;
}

void spi_terminate_hal(const void* spi_context) {

    const spi_context_t* setup = (const spi_context_t *)spi_context;

    spi_deinit(setup->inst);

    gpio_deinit(setup->miso);
    gpio_deinit(setup->mosi);
    gpio_deinit(setup->sck);

    return;
}

uint spi_write_hal(const void *spi_context, char * data, int len) {

    const spi_context_t * cxt = (const spi_context_t *)spi_context;

    return spi_write_blocking(cxt->inst, data, len);

}

uint spi_read_hal(const void *spi_context, char * buf, int len) {

    const spi_context_t * cxt = (const spi_context_t *)spi_context;

    uint8_t txData = 0x00;

    uint bytes = spi_read_blocking(cxt->inst, txData, buf, len);

    return bytes;
}

uint spi_rw_hal(const void * spi_context, char * txData, int len) {

    const spi_context_t * cxt = (const spi_context_t *)spi_context;

    char * rxData = malloc(len);

    uint bytes = spi_write_read_blocking(cxt->inst, txData, rxData, len);

    memcpy(txData, rxData, len);    // something something memory unsafe something something

    free(rxData);

    // TODO: Verify that the txData buffer indeed is overwritten.

    return bytes;
}