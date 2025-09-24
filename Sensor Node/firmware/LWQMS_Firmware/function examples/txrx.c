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

void test_encrypt_decrypt(const char *message) {

    uint8_t ciphertext[255];    // max LoRa payload
    uint8_t decrypted[223];     // max plaintext

    size_t ciphertext_len = 0;
    size_t decrypted_len = 0;

    size_t msg_len = strlen(message);

    printf("Original (%zu bytes): %s\n", msg_len, message);

    // Encrypt
    bool enc_ok = aes_128_encrypt(test_key, sizeof(test_key),
                                  (const uint8_t*)message, msg_len,
                                  ciphertext, &ciphertext_len);

    if (!enc_ok) {
        printf("Encryption failed!\n");
        return;
    }

    printf("Ciphertext length: %zu bytes\n", ciphertext_len);

    printf("Original:\n\n");
    hexdump(message, msg_len, 0x00);

    printf("Ciphertext:\n\n");
    hexdump(ciphertext, 255, 0x00);

    // Decrypt
    bool dec_ok = aes_128_decrypt(test_key, sizeof(test_key),
                                  ciphertext, ciphertext_len,
                                  decrypted, &decrypted_len);

    if (!dec_ok) {
        printf("Decryption failed!\n");
        return;
    }

    // Null-terminate for printing (safe because decrypted_len ≤ 223)
    decrypted[decrypted_len] = '\0';

    printf("Decrypted (%zu bytes): %s\n", decrypted_len, decrypted);

    // Compare
    if ((decrypted_len == msg_len) &&
        (memcmp(message, decrypted, msg_len) == 0)) {
        printf("Round-trip success!\n\n");
    } else {
        printf("Round-trip mismatch!\n\n");
    }
}

int main()
{

    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

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

    char * txBuf = "Four score and seven years ago our fathers brought forth on this continent a new nation.";

    while (true) {

        usb_console_write_hal("Setting RX Mode...");

        lora_rx(&radio_0, &prototyping_irq_masks, LWQMS_SYNC_WORD, 60000);
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

        //printf("Encrypted packet:\n\n");
        //hexdump(packet, rxlen, 0x00);

        char msg[256];
        size_t msg_len;

        bool decryption_success = aes_128_decrypt(test_key, AES_BLOCKLEN, packet, rxlen, msg, &msg_len);

        printf("Decrypted packet: %s\n\n", msg);

        sleep_ms(500);

    }    

    #else

    usb_console_write_hal("Setting up the radio for a transmit operation...");
    
    lora_init_tx(   &radio_0, 
                    &sx1262_14dBm_pa_params, 
                    &prototyping_mod_params,
                    14, 
                    SX126X_RAMP_200_US,
                    LWQMS_SYNC_WORD
                );

    usb_console_write_hal("DONE\n\n\n");

    gpio_write_hal(STATUS_LED, true);

    char * txBuf = "Four score and seven years ago our fathers brought forth on this continent a new nation.";

    // Encrypt the buffer
    uint8_t enc_buf[255];
    size_t encrypted_len;

    bool encryption_success = aes_128_encrypt(test_key, AES_BLOCKLEN, txBuf, 89, enc_buf, &encrypted_len);

    while (true) {

        usb_console_write_hal("To transmit a packet, press 't'.\n");

        // Wait for input
        //while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive
        while (!tx_go) {}
        tx_go = false;

        usb_console_write_hal("Sending Packet...");

        gpio_write_hal(RX_LED, true);

        // Transmit a packet
        lora_tx(&radio_0,
                &prototyping_irq_masks,
                &prototyping_pkt_params,
                enc_buf,
                encrypted_len
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