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

bool watchdog_init_hal(uint32_t timeout_ms) {
    if (timeout_ms > WATCHDOG_MAX_DELAY_MS) {
        return false;
    }
    else {
        watchdog_enable(timeout_ms, true);
        return true;
    }
}

void watchdog_feed_hal() {
    watchdog_update();
}

void watchdog_deinit_hal() {
    watchdog_disable();
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

#pragma region Power Management

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Power Management

bool power_mgmt_init_hal(void *power_mgmt_context_dormant, void *power_mgmt_context_active, void *processed_power_context_buf) {
    powman_timer_set_1khz_tick_source_lposc();  // Use the low-power oscillator as a clock reference
    powman_timer_start();
    powman_timer_set_ms(time_us_64() / 1000);   // Convert us to ms
    powman_set_debug_power_request_ignored(true);   // Enable power down even when debugging.

    // Cast the incoming power setting contexts to a rp2350_power_management structure.
    rp2350_power_mgmt_setting_t *off_state = (rp2350_power_mgmt_setting_t *)power_mgmt_context_dormant;
    rp2350_power_mgmt_setting_t *on_state = (rp2350_power_mgmt_setting_t *)power_mgmt_context_active;

    // Initialize buffers to store the hardware-specific power management values.
    powman_power_state dormant_state = POWMAN_POWER_STATE_NONE;
    powman_power_state active_state = POWMAN_POWER_STATE_NONE;
    
    // Manually create a domains array to force agreement between x->as_arr[] index and corresponding power domain
    enum powman_power_domains domains[] = {
        POWMAN_POWER_DOMAIN_SWITCHED_CORE,
        POWMAN_POWER_DOMAIN_XIP_CACHE,
        POWMAN_POWER_DOMAIN_SRAM_BANK0,
        POWMAN_POWER_DOMAIN_SRAM_BANK1
    };
    
    // Fill the dormant & active state buffers with their designated values.
    for (int k = 0; k < (sizeof(domains) / sizeof(enum powman_power_domains)); ++k) {
        // Do not set the SRAM bits - they will fail validation check. Upon wakeup, these will be brought up by the warm start.
        if ((domains[k] == POWMAN_POWER_DOMAIN_SRAM_BANK0) || (domains[k] == POWMAN_POWER_DOMAIN_SRAM_BANK1)) continue;

        // For each power domain, check if it was enabled in the incoming setting structure, and enable it in hardware if so.
        dormant_state = off_state->as_arr[k] ? powman_power_state_with_domain_on(dormant_state, domains[k]) : dormant_state;
        active_state = on_state->as_arr[k] ? powman_power_state_with_domain_on(active_state, domains[k]) : active_state;
    }

    // Write the finished states to the outgoing power context buffer structure
    ((rp2350_power_state_context_t *)processed_power_context_buf)->dormant_power_state = dormant_state;
    ((rp2350_power_state_context_t *)processed_power_context_buf)->active_power_state = active_state;

    return true;
}

#define POWMAN_BOOT_VECTOR_ELEMENT_COUNT 4

int power_mgmt_go_dormant_hal(void *power_context) {
    
    // Cast the incoming power context to a hardware-specific power definition structure.
    rp2350_power_state_context_t *power_states = (rp2350_power_state_context_t *)power_context;

    // Set the power state management registers with the incoming hardware-specific power state values.
    if (!powman_configure_wakeup_state((powman_power_state)(power_states->dormant_power_state), (powman_power_state)(power_states->active_power_state))) return PICO_ERROR_INVALID_STATE;

    // Write zeroes to the boot register, signaling a cold boot. 
    // TODO (not for this project, but future): Enable Warm Start/Resuming program with customizeable boot vector.
    for (int k = 0; k < POWMAN_BOOT_VECTOR_ELEMENT_COUNT; ++k) {
        powman_hw->boot[k] = 0;
    }

    printf("Powering off...\n");
    stdio_flush();
    stdio_deinit_all();
    watchdog_deinit_hal();  // back to the kennel
    int retval = powman_set_power_state(power_states->dormant_power_state);
    if (PICO_OK != retval) return retval;

    while (1) __wfi();  // Wait for interrupt.
}

int power_mgmt_go_dormant_for_time_ms_hal(void *power_context, uint64_t duration_ms) {
    // Enable an alarm after the specified duration, then enter the dormant state
    uint64_t alarm_time_ms = powman_timer_get_ms() + duration_ms;
    powman_enable_alarm_wakeup_at_ms(alarm_time_ms);
    return power_mgmt_go_dormant_hal(power_context);
}

int power_mgmt_go_dormant_until_irq_hal(void *power_context, gpio_driven_irq_context_t *trigger) {
    // Check the trigger sources for use with the powman call
    bool trigger_is_edge_driven = (trigger->source_mask & (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)) > 0;
    bool trigger_is_high_or_rising = (trigger->source_mask & (GPIO_IRQ_LEVEL_HIGH | GPIO_IRQ_EDGE_RISE)) > 0; 
    
    // Enable GPIO wakeup in the power management system
    powman_enable_gpio_wakeup(0, trigger->pin, trigger_is_edge_driven, trigger_is_high_or_rising);
    
    // Enter the dormant state
    return power_mgmt_go_dormant_hal(power_context);
}

int power_mgmt_write_novo_memory_hal(uint32_t *data, size_t len) {
    if (len > MCU_POWMAN_NOVO_ELEMENTS) return -1;

    for (int k = 0; k < len; ++k) {
        // Write the elements to the scratch register one-by-one as it is volatile. Therefore, we have some optimization disagreement.
        powman_hw->scratch[k] = data[k];
    }

    return 0;
}

int power_mgmt_read_novo_memory_hal(uint32_t *data, size_t buf_len) {
    if (buf_len < MCU_POWMAN_NOVO_ELEMENTS) return -1;

    for (int k = 0; k < MCU_POWMAN_NOVO_ELEMENTS; ++k) {
        data[k] = powman_hw->scratch[k];
    }

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


#pragma endregion

/* --- EOF ------------------------------------------------------------------ */