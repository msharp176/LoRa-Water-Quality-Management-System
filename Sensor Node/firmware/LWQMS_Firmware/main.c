/******************************************************************************************************************** 
*   
*   @file main.c
*
*   @brief Main Driver file for the LoRa Water Quality Management System Sensor Node Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/


#include "pico/stdlib.h"
#include "pico/stdio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "hal.h"
#include "sx126x.h"

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

sx126x_context_t radio_0 = {
    .busy = GP20,
    .irq = GP21,
    .rst = GP22,
    .cs = GP17,
    .spi_context = &context_spi_0, // SPI Bus 0
    .radio_operation_timeout_us = RADIO_TIMEOUT_GLOBAL_US,
    .designator = "RADIO 0"
};

char * gettysburg_address = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth. Abraham Lincoln November 19, 1863";

int main()
{
    stdio_init_all();

    sx126x_init(&radio_0);

    uint msg_len = strlen(gettysburg_address);

    char * buf = malloc(msg_len);

    while (true) {
        sleep_ms(30);

        strncpy(buf, gettysburg_address, msg_len);
        
        gpio_write_hal(&radio_0.cs, GPIO_LOW);
        sleep_ms(1);
        
        spi_rw_hal(radio_0.spi_context, buf, msg_len);
        
        sleep_ms(1);
        gpio_write_hal(&radio_0.cs, GPIO_HIGH);
        
        printf("Received Data:\n%s", buf);

        sleep_ms(30);

    }
}
