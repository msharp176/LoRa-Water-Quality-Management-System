#include <stdio.h>
#include "pico/stdlib.h"

#define BLINK_INTERVAL 250

int main() {

    const uint led_pin = 25;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    // Initialize chosen serial port
    stdio_init_all();

    // Loop forever
    while (true) {

        // Blink LED
        printf("Blinking!\r\n");
        gpio_put(led_pin, true);
        sleep_ms(BLINK_INTERVAL);
        gpio_put(led_pin, false);
        sleep_ms(BLINK_INTERVAL);
    }
}