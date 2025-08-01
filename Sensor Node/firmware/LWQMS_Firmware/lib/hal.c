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

#include "hal.h"

#pragma region GPIO

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// GPIO Functions

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

bool gpio_toggle_hal(const void *gpio_context) {

    bool current_pin_state = gpio_read_hal(gpio_context);

    gpio_write_hal(gpio_context, !current_pin_state);

    return !current_pin_state;

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region SPI

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// SPI

uint spi_init_hal(const void* spi_context) {
    
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

void spi_reset_hal(const void* spi_context) {

    const spi_context_t* setup = (const spi_context_t *)spi_context;

    uint8_t SPI_RESET_BLK;

    if (setup->inst == spi0) {
        SPI_RESET_BLK = RESET_SPI0;
    } else if (setup->inst == spi1) {
        SPI_RESET_BLK = RESET_SPI1;
    } else {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "The requested SPI block does not exist.", "spi_reset_hal");
    }

    reset_block(SPI_RESET_BLK);

}

uint spi_write_hal(const void* spi_context, char * data, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
    
    return spi_write_blocking(cxt->inst, data, len);
    
}

uint spi_read_hal(const void* spi_context, char * buf, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
    
    uint8_t txData = 0x00;
    
    uint bytes = spi_read_blocking(cxt->inst, txData, buf, len);
    
    return bytes;
}

uint spi_rw_hal(const void* spi_context, char * txData, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
    
    char * rxData = malloc(len);
    
    uint bytes = spi_write_read_blocking(cxt->inst, txData, rxData, len);
    
    memcpy(txData, rxData, len);    // something something memory unsafe something something
    
    free(rxData);
    
    // TODO: Verify that the txData buffer indeed is overwritten.
    
    return bytes;
}

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

void reboot(void) {
    #ifdef DEBUG
    printf("Rebooting...\n");
    #endif
    
    watchdog_enable(10, false);
    watchdog_reboot(0, 0, 0);
    
    while (true) {};
}

bool check_if_rebooted_or_clean_boot(void) {
    return watchdog_caused_reboot();
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region Interrupts

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Interrupts 

void gpio_irq_attach_hal(const void *irq_context) {

    const gpio_driven_irq_context_t * context = (const gpio_driven_irq_context_t *)irq_context;

    register_gpio_isr(context);

    gpio_set_irq_enabled_with_callback(context->pin, context->source_mask, true, &isr_gpio_master);

}

void gpio_irq_detach_hal(const void *irq_context) {

    const gpio_driven_irq_context_t * context = (const gpio_driven_irq_context_t *)irq_context;

    unregister_gpio_isr(context);

    gpio_set_irq_enabled_with_callback(context->pin, context->source_mask, false, &isr_gpio_master);

}

void gpio_irq_ack_hal(const void *irq_context) {

    const gpio_driven_irq_context_t * context = (const gpio_driven_irq_context_t *)irq_context;

    gpio_acknowledge_irq(context->pin, context->source_mask);

}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region USB Console

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// USB Console

void init_usb_console_hal(void) {
    stdio_init_all();
}

void deinit_usb_console_hal(void) {
    stdio_deinit_all();
}

bool is_usb_console_connected_hal(void) {
    return tud_cdc_connected();
}

bool is_usb_console_available_hal(void) {
    return tud_cdc_available();
}

bool wait_for_usb_console_connection_hal(void) {
    while (!is_usb_console_connected_hal()) {}

    return true;
}

bool wait_for_usb_console_connection_with_timeout_hal(uint32_t timeout_ms) {
    
    uint32_t ms_elapsed = 0;
    
    while (!is_usb_console_connected_hal()) {

        if (ms_elapsed > timeout_ms) {
            return false;
        }

        sleep_ms(1);
        ms_elapsed++;
    }

    return true;
}

char usb_console_getchar_hal(void) {
    return getchar(); 
}

int usb_console_putchar_hal(char c) {
    return putchar(c);
}

char usb_console_getchar_timeout_us_hal(uint32_t timeout_microseconds) {
    return getchar_timeout_us(timeout_microseconds);
}

int usb_console_write_hal(char * buf) {
    return printf("%s", buf);
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

/* --- EOF ------------------------------------------------------------------ */