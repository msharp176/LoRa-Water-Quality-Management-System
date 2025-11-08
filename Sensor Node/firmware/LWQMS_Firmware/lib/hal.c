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

void gpio_setup_hal(const uint8_t pin, bool IsOutput) {

    gpio_init(pin);
    gpio_set_dir(pin, IsOutput);
    
    return;
}

void gpio_set_pull_resistor_hal(const uint8_t pin, bool IsPullUp) {
    gpio_set_pulls(pin, IsPullUp, !IsPullUp);
    return;
}

void gpio_terminate_hal(const uint8_t pin) {
    gpio_deinit(pin);
    return;
}

void gpio_write_hal(const uint8_t pin, bool state) {
        
    gpio_put(pin, state);
    
}

bool gpio_read_hal(const uint8_t pin) {    
    return gpio_get(pin);
}

bool gpio_toggle_hal(const uint8_t pin) {

    bool current_pin_state = gpio_read_hal(pin);

    gpio_write_hal(pin, !current_pin_state);

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

uint spi_write_hal(const void* spi_context, uint8_t * data, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
    
    return spi_write_blocking(cxt->inst, data, len);
    
}

uint spi_read_hal(const void* spi_context, uint8_t * buf, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
    
    uint8_t txData = 0x00;
    
    uint bytes = spi_read_blocking(cxt->inst, txData, buf, len);
    
    return bytes;
}

uint spi_rw_hal(const void* spi_context, uint8_t *txData, uint8_t *rxData, int len) {
    
    const spi_context_t * cxt = (const spi_context_t *)spi_context;
        
    uint bytes = spi_write_read_blocking(cxt->inst, txData, rxData, len);
    
    return bytes;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

#pragma region I2C

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// I2C

uint i2c_init_hal(const void* i2c_context) {
    
    const i2c_context_t *setup = (const i2c_context_t*)i2c_context;

    uint baud = i2c_init(setup->inst, setup->baud);

    gpio_set_function(setup->sda, GPIO_FUNC_I2C);
    gpio_set_function(setup->scl, GPIO_FUNC_I2C);
    
    gpio_pull_up(setup->sda);
    gpio_pull_up(setup->scl);

    return baud;    
}

void i2c_terminate_hal(const void* i2c_context) {

    const i2c_context_t *setup = (const i2c_context_t*)i2c_context;

    // De-initialize the i2c instance on the MPU
    i2c_deinit(setup->inst);

    // De-init the GPIO
    gpio_deinit(setup->sda);
    gpio_deinit(setup->scl);

    return;
}

int i2c_write_hal(const void* i2c_context, uint8_t address, const uint8_t* txData, uint len) {

    const i2c_context_t *setup = (const i2c_context_t*)i2c_context;

    // Write the data and catch errors as they occur.
    int bytes_written = i2c_write_blocking(setup->inst, address, txData, len, false);

    return bytes_written;
}

int i2c_read_hal(const void* i2c_context, uint8_t address, uint8_t* rxData, uint len) {

    const i2c_context_t *setup = (const i2c_context_t*)i2c_context;

    // Write the data and catch errors as they occur.
    int bytes_read = i2c_read_blocking(setup->inst, address, rxData, len, false);

    return bytes_read;
}

int i2c_write_then_read_hal(const void* i2c_context, uint8_t address, uint8_t *txData, uint8_t *rxData, uint txLen, uint rxLen) {

    const i2c_context_t *setup = (const i2c_context_t*)i2c_context;

    bool i2c_ok = false;

    int bytes_written = 0, bytes_read = 0;

    do {
        bytes_written = i2c_write_blocking(setup->inst, address, txData, txLen, true);
        if (bytes_written < 0) break;

        bytes_read = i2c_read_blocking(setup->inst, address, rxData, rxLen, false);
        if (bytes_read < 0) break;

        i2c_ok = true;
    } while (0);

    if (!i2c_ok) {
        err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "I2C Transaction Failure", "i2c_write_then_read");
    }

    return bytes_written < 0 ? bytes_written : bytes_read;
}

static bool reserved_i2c_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void i2c_scan_hal(const i2c_context_t* i2c_context) {
    printf("\nI2C Bus Scan\n");
    printf("   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");

    for (int addr = 0; addr < (1 << 7); ++addr) {
        if (addr % 16 == 0) {
            printf("%02x ", addr);
        }

        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_i2c_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_context->inst, addr, &rxdata, 1, false);

        printf(ret < 0 ? "." : "@");
        printf(addr % 16 == 15 ? "\n" : "  ");
    }
    printf("Done.\n");
    return;
}

int i2c_get_available_addresses_hal(const i2c_context_t* i2c_context, uint8_t* addressesBuf, uint8_t addressBufSize, uint8_t *addressesFound) {

    int address_idx = 0;

    for (int addr = 0; addr < (1 << 7); ++addr) {
        // Perform a 1-byte dummy read from the probe address. If a slave
        // acknowledges this address, the function returns the number of bytes
        // transferred. If the address byte is ignored, the function returns
        // -1.

        // Skip over any reserved addresses.
        int ret;
        uint8_t rxdata;
        if (reserved_i2c_addr(addr))
            ret = PICO_ERROR_GENERIC;
        else
            ret = i2c_read_blocking(i2c_context->inst, addr, &rxdata, 1, false);

        if (ret > -1) {
            addressesBuf[address_idx] = addr;
            address_idx++;
            *addressesFound = address_idx;
            
            // Ensure we are writing to valid memory
            if (address_idx > (addressBufSize - 1)) {
                return -1;
            }
        }
    }

    return 0;
}

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

int get_user_input_hal(char * buf, uint buflen) {    
    
    char * ptr;
    
    for (ptr = buf; (ptr - buf) < (buflen - 1); ptr++) {
        *ptr = usb_console_getchar_hal();

        usb_console_putchar_hal(*ptr);

        if (*ptr == '\r' || *ptr == '\n') {
            printf("\n");
            break;
        }
    }

    // Guarantee null termination
    *ptr = 0x00;

    // Return the number of characters read.
    return ptr - buf;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#pragma endregion

/* --- EOF ------------------------------------------------------------------ */