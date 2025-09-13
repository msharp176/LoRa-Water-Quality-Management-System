/*************************************************************************************
 * 
 * @file tmux1309.c
 * 
 * @brief Driver for the TMUX1309 Dual 4:1 Bidirectional Analog Multiplexer
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "tmux1309.h"

int tmux1309_init(tmux1309_context_t *context) {
    // Initialize all GPIO pins for the given TMUX1309 Instance
    uint8_t pins[3] = {context->enable, context->sel0, context->sel1};

    for (int k = 0; k < 3; k++) {
        // Extract the pin
        uint8_t pin = pins[k];

        // Initialize the pin
        gpio_setup_hal(pin, true);

        // Use a pull-up resistor
        gpio_set_pull_resistor_hal(pin, true);

        // Set pins high
        gpio_write_hal(pin, true);
    }

    return 0;
}

int tmux1309_disable(tmux1309_context_t *context) {
    gpio_write_hal(context->enable, GPIO_HIGH);
}

int tmux1309_enable(tmux1309_context_t *context) {
    gpio_write_hal(context->enable, GPIO_LOW);
}

int tmux1309_set_output(tmux1309_context_t *context, uint8_t selection) {

    // Input validation
    if (selection > 3) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Invalid Output Selection on TMUX1309! Output selection must be between 0 and 3.", "tmux1309_set_output");
        return -1;
    }

    // Get the target GPIO states
    bool sel0_state = (selection & 0x01) > 0;
    bool sel1_state = (selection & 0x02) > 0;

    // Enable the multiplexer
    tmux1309_enable(context);

    // Write the GPIO states
    gpio_write_hal(context->sel0, sel0_state);
    gpio_write_hal(context->sel1, sel1_state);

    return 0;
}