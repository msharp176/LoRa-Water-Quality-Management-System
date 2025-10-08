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

#include "main.h"

#define DIGIPOT_STEP_TIME_MS 5

#define STATUS_LED GP14
#define RX_LED GP15

#define RX

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Variables

char * gettysburg_address_trunc = "Four score and seven years ago our fathers brought forth on this continent a new nation.";
char * gettysburg_address = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth.";

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

/**
 * To transmit data:
 * Initialize radio
 * Initialize TX mode
 * Attach the radio interrupt
 * Transmit data
 * Wait for interrupt
 */

static bool tx_go = false;

void isr_set_tx_go(void) {
    tx_go = true;
}

void print_banner(void) {
    printf("-- LoRa Water Quality Management System Sensor Node --\n");
    printf("Version 0.1, compiled %s, %s\n\n", __DATE__, __TIME__);
}

const gpio_driven_irq_context_t context_irq_txInit = {
    .callback = isr_set_tx_go,
    .pin = GP12,
    .source_mask = GPIO_IRQ_EDGE_FALL
};

// Example 128-bit key (16 bytes)
static const uint8_t test_key[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be connected
    wait_for_usb_console_connection_hal();

    sleep_ms(500);

    print_banner();

    // Initialize the gpio pins
    printf("Initializing hardware...");
    // Initialize the SPI bus
    spi_init_hal(&context_spi_0);

    // Initialize chip select and pull it high to start
    gpio_setup_hal(context_flash_0.cs, true);
    gpio_write_hal(context_flash_0.cs, GPIO_HIGH);

    printf("DONE\n");

    // Enable writes to the chip
    mxl23l3233f_write_enable(&context_flash_0);

    // ERASURE --------------------------------------------------------------------------------------------------

    printf("Testing chip erase...");

    // Perform a full chip erasure
    mxl23l3233f_chip_erase(&context_flash_0);

    // Readback the entire chip data. Ensure every byte is set to 0xff.

    // Array to hold the ideal erased sector.
    char erased_sector[FLASH_SECTOR_SIZE];
    memset(erased_sector, 0xff, FLASH_SECTOR_SIZE);
    
    // Array to hold read back data for erasure validation
    char erase_rxBuf[FLASH_SECTOR_SIZE];

    uint32_t offset = 0x00;

    bool erase_passed = true;

    for (int k = 0; k < context_flash_0.sectors; k++) {
        // Clear out the rx buffer
        memset(erase_rxBuf, 0x00, FLASH_SECTOR_SIZE);

        // Read the sector data
        mxl23l3233f_read_data(&context_flash_0, erase_rxBuf, FLASH_SECTOR_SIZE, offset);

        // Check that the sector was indeed erased.
        if (memcmp(erase_rxBuf, erased_sector, FLASH_SECTOR_SIZE) != 0) {
            printf("\nFailed to erase sector %d!!\n", k);
            hexdump(erase_rxBuf, FLASH_SECTOR_SIZE, 0x00);
            erase_passed = false;
        }

        if (k == 0) {
            // Print out the first sector for fun.
            printf("First sector after erasure:\n\n");
            hexdump(erase_rxBuf, FLASH_SECTOR_SIZE, 0x00);
        }
    }

    if (erase_passed) {
        printf("PASS\n");
    }
    else {
        printf("FAIL\n");
    }
    
    // ---------------------------------------------------------------------------------------------------------------

    // Read and Write ------------------------------------------------------------------------------------------------

    // Write the gettysburg address to the first page of the flash
    printf("Testing write and read operations...");

    // Write the gettsyburg address
    mxl23l3233f_write_data(&context_flash_0, gettysburg_address, 1474, 0x00);

    // Read it back
    char gettysburg_readback[1500];
    memset(gettysburg_readback, 0x00, 1500);

    mxl23l3233f_read_data(&context_flash_0, gettysburg_readback, 1474, 0x00);

    if (memcmp(gettysburg_readback, gettysburg_address, 1474) != 0) {
        printf("FAIL\n");
    }
    else {
        printf("PASS\n");
    }

    printf("\n\nReadback:\n\n");
    hexdump(gettysburg_readback, 1474, 0x00);

    // ---------------------------------------------------------------------------------------------------------------

    // Power Down ------------------------------------------------------------------------------------------------


    printf("Powering down chip...");

    mxl23l3233f_deep_power_down(&context_flash_0);

    printf("DONE\n");

    // Attempt to read the ID of the chip while it is powered down. Should receive no response
    char IDBuf_sleep[4];
    memset(IDBuf_sleep, 0x00, 4);
    
    mxl23l3233f_read_jedec_id(&context_flash_0, IDBuf_sleep, 4);

    printf("ID Readback\n");
    hexdump(IDBuf_sleep, 4, 0x00);

    mxl23l3233f_deep_power_down_release(&context_flash_0);

    // Clear the ID buffer
    memset(IDBuf_sleep, 0x00, 4);
    mxl23l3233f_read_jedec_id(&context_flash_0, IDBuf_sleep, 4);

    printf("ID Readback:\n");
    hexdump(IDBuf_sleep, 4, 0x00);

    // ---------------------------------------------------------------------------------------------------------------

    // Idle
    while (1) {}

}



/* --- EOF ------------------------------------------------------------------ */