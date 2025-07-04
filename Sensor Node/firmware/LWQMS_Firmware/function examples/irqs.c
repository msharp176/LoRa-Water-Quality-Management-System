/******************************************************************************************************************** 
*   
*   @file irqs.c
*
*   @brief GPIO-driven interrupt example using the Custom-Built Raspberry Pi Pico HAL
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "main.h"

uint8_t err_led = ERROR_LED;

const gpio_driven_irq_context_t irq1 = {
    .pin = GP14,
    .source_mask = GPIO_IRQ_EDGE_FALL,
    .callback = master_irq_callback
};

const gpio_driven_irq_context_t irq2 = {
    .pin = GP15,
    .source_mask = GPIO_IRQ_EDGE_RISE,
    .callback = master_irq_callback
};

void handle_irq1(void) {

    gpio_irq_ack_hal(&irq1);

    gpio_toggle_hal(&err_led);

    return;

}

void handle_irq2(void) {

    gpio_irq_ack_hal(&irq2);

    usb_console_write_hal("Rising edge interrupt!\n");

    return;

}

void master_irq_callback(uint gpio_pin, uint32_t event_type) {

    switch (gpio_pin) {
        case GP14:
            handle_irq1();
            break;
        case GP15:
            handle_irq2();
            break;
        default:
            break;
    }

}

int main()
{
    init_usb_console_hal();

    gpio_setup_hal(&err_led, true);
    gpio_write_hal(&err_led, GPIO_LOW);
    
    gpio_setup_hal(&irq1, false);
    gpio_setup_hal(&irq2, false);

    gpio_irq_attach_hal(&irq1);
    gpio_irq_attach_hal(&irq2);

    while (true) {}
}