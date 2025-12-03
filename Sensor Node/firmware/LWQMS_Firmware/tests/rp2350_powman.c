#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/powman.h"
#include "tusb.h"

#define BLINKY_LED_PIN 15

static powman_power_state power_on_state;
static powman_power_state power_off_state;

void gpio_toggle(uint8_t pin) {
    bool state = gpio_get(pin);
    gpio_put(pin, !state);
}

void powman_init(void) {
    powman_timer_set_1khz_tick_source_lposc();
    powman_timer_start();
    powman_timer_set_ms(time_us_64() / 1000);

    powman_set_debug_power_request_ignored(true);

    powman_power_state P1_7 = POWMAN_POWER_STATE_NONE;
    powman_power_state P0_0 = POWMAN_POWER_STATE_NONE;

    enum powman_power_domains target_on_domains[] = {
        POWMAN_POWER_DOMAIN_SWITCHED_CORE, 
        POWMAN_POWER_DOMAIN_XIP_CACHE, 
        POWMAN_POWER_DOMAIN_SRAM_BANK0, 
        POWMAN_POWER_DOMAIN_SRAM_BANK1
    };

    for (int k = 0; k < (sizeof(target_on_domains) / sizeof(enum powman_power_domains)); k++) {
        P0_0 = powman_power_state_with_domain_on(P0_0, target_on_domains[k]);
    }

    power_on_state = P0_0;
    power_off_state = P1_7;
}

static int powman_go_dormant(void) {
    stdio_flush();

    if (!powman_configure_wakeup_state(power_off_state, power_on_state)) {
        return PICO_ERROR_INVALID_STATE;
    }

    // Set the reboot address to main (0x00000....)
    for (int k = 0; k < (sizeof(powman_hw->boot) / sizeof(powman_hw->boot[0])); k++) {
        powman_hw->boot[k] = 0;
    }

    int retval = powman_set_power_state(power_off_state);
    if (PICO_OK != retval) {
        return retval;
    }

    while (1) __wfi();  // Wait for interrupt.
}

static int powman_go_dormant_for_ms(uint64_t duration_ms) {
    printf("Powering off...\n");
    uint64_t alarm_time = powman_timer_get_ms() + duration_ms;
    powman_enable_alarm_wakeup_at_ms(alarm_time);
    return powman_go_dormant();
}

int main()
{
    sleep_ms(500);
    printf("I'M ALIVE!!\n");
    gpio_init(BLINKY_LED_PIN);
    gpio_set_dir(BLINKY_LED_PIN, true);
    gpio_put(BLINKY_LED_PIN, false);
    powman_init();

    for (int k = 0; k < 30; k++) {
        gpio_toggle(BLINKY_LED_PIN);
        sleep_ms(100);
        printf("Blink %d\n", k);
    }

    if (powman_go_dormant_for_ms(10000) < 0) {
        printf("Bad state!");
    }
}