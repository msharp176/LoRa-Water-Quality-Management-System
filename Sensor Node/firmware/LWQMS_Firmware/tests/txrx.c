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

    // Wait for the USB console to be opened on the host PC
    #ifdef RX
    wait_for_usb_console_connection_hal();
    #endif

    sleep_ms(100);

    print_banner();

    usb_console_write_hal("Initializing Hardware...");
    sx126x_initialize_hardware_context(&radio_0);

    #ifndef RX // TX Mode has this interrupt.
        gpio_setup_hal(context_irq_txInit.pin, false);
        gpio_irq_attach_hal(&context_irq_txInit);
    #endif

    gpio_setup_hal(STATUS_LED, true);
    gpio_write_hal(STATUS_LED, false);
    gpio_setup_hal(RX_LED, true);
    gpio_write_hal(RX_LED, false);

    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio...");
    sx126x_radio_setup(&radio_0);    
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up interrupts...");
    sx126x_interrupt_setup(&radio_0);
    usb_console_write_hal("DONE\n");

    #ifdef RX

    usb_console_write_hal("Setting up the radio for a receive operation...");
    
    lora_init_rx(
        &radio_0,
        &prototyping_mod_params,
        &prototyping_pkt_params
    );

    usb_console_write_hal("DONE\n\n\n");

    gpio_write_hal(STATUS_LED, true);

    while (true) {

        usb_console_write_hal("Setting RX Mode...");

        lora_rx(&radio_0, &prototyping_irq_masks, LWQMS_SYNC_WORD, SX126X_RX_CONTINUOUS);
        gpio_write_hal(RX_LED, true);

        usb_console_write_hal("DONE\n");

        usb_console_write_hal("Waiting for interrupt...");
        while (!sx126x_check_for_interrupt()) {}
        gpio_write_hal(RX_LED, false);
        usb_console_write_hal("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_RX_DONE) != 0) {
            usb_console_write_hal("RX Success!");
            // Retrieve the data
        }
        else if ((serviced_interrupts & SX126X_IRQ_TIMEOUT) != 0) {
            usb_console_write_hal("ERROR: TIMEOUT!");
        }
        else if ((serviced_interrupts & SX126X_IRQ_CRC_ERROR)) {
            usb_console_write_hal("ERROR: Bad CRC!");
        }
        else if ((serviced_interrupts & SX126X_IRQ_HEADER_ERROR)) {
            usb_console_write_hal("ERROR: Bad header!");
        }
        else {
            usb_console_write_hal("Bad operation!");
        }

        usb_console_write_hal("\n\n");

        char packet[256];
        uint8_t rxlen;

        lora_get_rx_data(&radio_0, packet, &rxlen);

        packet[rxlen] = '\0';

        printf("\033[1;37;41mReceived Packet: %s\033[0m\n\n\n", packet);
        
        //printf("Encrypted packet:\n\n");
        //hexdump(packet, rxlen, 0x00);

        //char msg[256];
        //size_t msg_len;

        //bool decryption_success = aes_128_decrypt(test_key, AES_BLOCKLEN, packet, rxlen, msg, &msg_len);

        //printf("Decrypted packet: %s\n\n", msg);

        sleep_ms(500);

    }    

    #else

    usb_console_write_hal("Setting up the radio for a transmit operation...");
    
    lora_init_tx(   &radio_0, 
                    &sx1262_22dBm_pa_params, 
                    &prototyping_mod_params,
                    22, 
                    SX126X_RAMP_200_US,
                    LWQMS_SYNC_WORD
                );

    usb_console_write_hal("DONE\n\n\n");

    gpio_write_hal(STATUS_LED, true);

    uint8_t packetnum = 0;

    char txBuf[50];

    // Encrypt the buffer
    uint8_t enc_buf[255];
    size_t encrypted_len;

    //bool encryption_success = aes_128_encrypt(test_key, AES_BLOCKLEN, txBuf, 89, enc_buf, &encrypted_len);

    while (true) {

        usb_console_write_hal("To transmit a packet, press 't'.\n");

        // Wait for input
        //while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive
        while (!tx_go) {}
        tx_go = false;

        // Clear the old buffer
        memset(txBuf, 0x00, 50);
        snprintf(txBuf, 50, "aloha! this is packet # [%d]", packetnum++);

        gpio_write_hal(RX_LED, true);

        // Transmit a packet
        lora_tx(&radio_0,
                &prototyping_irq_masks,
                &prototyping_pkt_params,
                txBuf,
                strnlen(txBuf, 50)
        );

        usb_console_write_hal("DONE\n");

        usb_console_write_hal("Waiting for interrupt...");
        while (!sx126x_check_for_interrupt()) {}
        gpio_write_hal(RX_LED, false);
        usb_console_write_hal("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_TX_DONE) != 0) {
            usb_console_write_hal("TX Success!");
        }
        else if ((serviced_interrupts & SX126X_IRQ_TIMEOUT) != 0) {
            usb_console_write_hal("ERROR: TIMEOUT!");
        }
        else {
            usb_console_write_hal("Bad operation!");
        }

        usb_console_write_hal("\n\n");
    }    

    #endif /* RX */
}



/* --- EOF ------------------------------------------------------------------ */