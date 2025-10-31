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
#include "system_config.h"

#define POWER_5V_COOLDOWN_DURATION_MS 10000

//#define RX

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Global Variables

char * gettysburg_address_trunc = "Four score and seven years ago our fathers brought forth on this continent a new nation.";
char * gettysburg_address = "Four score and seven years ago our fathers brought forth on this continent, a new nation, conceived in Liberty, and dedicated to the proposition that all men are created equal. Now we are engaged in a great civil war, testing whether that nation, or any nation so conceived and so dedicated, can long endure. We are met on a great battle-field of that war. We have come to dedicate a portion of that field, as a final resting place for those who here gave their lives that that nation might live. It is altogether fitting and proper that we should do this. But, in a larger sense, we can not dedicate -- we can not consecrate -- we can not hallow -- this ground. The brave men, living and dead, who struggled here, have consecrated it, far above our poor power to add or detract. The world will little note, nor long remember what we say here, but it can never forget what they did here. It is for us the living, rather, to be dedicated here to the unfinished work which they who fought here have thus far so nobly advanced. It is rather for us to be here dedicated to the great task remaining before us -- that from these honored dead we take increased devotion to that cause for which they gave the last full measure of devotion -- that we here highly resolve that these dead shall not have died in vain -- that this nation, under God, shall have a new birth of freedom -- and that government of the people, by the people, for the people, shall not perish from the earth.";

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

void print_banner(void) {
    printf("-- LoRa Water Quality Management System Sensor Node --\n");
    printf("Version 0.1, compiled %s, %s\n\n", __DATE__, __TIME__);
}

void startup_menu() {
    char * menu_options = 
    "---- Startup Menu ----\n"
    "Please listen closely as our menu options have changed.\n\n"
    "-------------------------------------------------------\n\n"
    "Option Letter (case-insensitive) | Action\n"
    "-> c | Clear the current configuration data\n"
    "-> s | Set (overwrites existing) configuration data\n"
    "-> p | Print the current configuration data\n"
    "-> e | Enables the 5V rail (10 second cooldown until rail can be disabled.)\n"
    "-> d | Disables the 5V rail (10 second cooldown until rail can be enabled.)\n"
    "-> i | Performs an I2C scan for devices\n"
    "-> r | Reboot the device - updates device settings with any changes made here.\n";

    absolute_time_t power_5v_cooldown_time = get_absolute_time();

    while (1) {

        printf(menu_options);
        printf("\n\n\n----> ");
    
        char response_buf[2];
    
        get_user_input_hal(response_buf, 2);

        printf("\n\n");
    
        char selection = response_buf[0] |= (1 << 5);   // Force lowercase
    
        switch (selection) {
            case 'c':
                // Overwrite the space allocated for the configuration data with 0xff
                printf("Clearing the config...");
                mxl23l3233f_erase_sector(&context_flash_0, FLASH_ADDR_CONFIG);
                printf("DONE\n");
                break;
            case 's':
                node_config_t new_config = get_setup_data();
                printf("Writing new config...");
                mxl23l3233f_erase_sector(&context_flash_0, FLASH_ADDR_CONFIG);
                write_system_config_data(&context_flash_0, &new_config);
                printf("DONE\n");
                break;
            case 'p':
                node_config_t cfg_buf;
                read_system_config_data(&context_flash_0, &cfg_buf);
                print_node_configuration(&cfg_buf);
                break;
            case 'e':
            case 'd':
                printf("Waiting for power cooldown...");
                while (get_absolute_time() < power_5v_cooldown_time) {}
                printf("DONE\n");

                printf("Setting 5V Rail %s...", selection == 'e' ? "ON" : "OFF");
                gpio_write_hal(EN_5V, selection == 'e' ? GPIO_HIGH : GPIO_LOW);
                printf("DONE");

                power_5v_cooldown_time = make_timeout_time_ms(POWER_5V_COOLDOWN_DURATION_MS);
                break;
            case 'i':
                uint8_t i2c_address_buf[0xff];
                uint8_t qty_available_addresses = 0;
                memset(i2c_address_buf, 0x00, 0xff);

                printf("Scanning I2C Bus...");
                i2c_get_available_addresses_hal(&context_i2c_1, i2c_address_buf, 0xff, &qty_available_addresses);
                printf("DONE\n");
                
                printf("Available I2C Addresses:\t");
                for (uint8_t * ptr = i2c_address_buf; (ptr - i2c_address_buf) < qty_available_addresses; ptr++) {
                    printf("%02x\t", *ptr);
                }
                printf("\n");
                break;
            case 'r':
                // Reboot the device. Force this option instead of exit so changes always get saved.
                reboot();
                break;
            default:
                printf("Invalid Option: %c", selection);
                break;              
        }

        printf("\n\n\n");
    }
}

void system_setup() {
    init_usb_console_hal();

    wait_for_usb_console_connection_hal();

    print_banner();

    lwqms_post_err_codes_t post_result = power_on_self_test();

    switch (post_result) {
        case POST_OK:
            printf("POST Successful!\n\n");
            for (int k = 0; k < 6; k++) {
                gpio_toggle_hal(STATUS_LED);
                sleep_ms(250);
            }
            break;
        case POST_ERR_NO_CONFIG_EXISTS:
            err_raise(ERR_POST_FAIL, ERR_SEV_NONFATAL, "Could not find configuration data!", "power_on_self_test");
            gpio_write_hal(ERR_LED, GPIO_HIGH);

            // Get a new config from the user
            node_config_t new_config = get_setup_data();
            if (write_system_config_data(&context_flash_0, &new_config) == 0) {
                printf("System restarting with updated configuration...\n\n\n");
                sleep_ms(250);
                reboot();
            }
            while (1);
        default:
            err_raise(ERR_POST_FAIL, ERR_SEV_FATAL, "Failed to POST! Error Code = %d", "power_on_self_test");
            while (1);
    }

    absolute_time_t startup_time = make_timeout_time_ms(3000);

    printf("To interrupt normal startup and enter the startup menu, press 'm'...\n");

    while (get_absolute_time() < startup_time) {
        if ((usb_console_getchar_timeout_us_hal(1000) | (1 << 5)) == 'm') {
            startup_menu();
        }
    }
}

const lwqms_pkt_payload_t test_payload = {
    .message = "Hallo!"
};

lwqms_pkt_t test_tx_pkt = {
    .pkt_id = 1,
    .dest_id = 2,
    .src_id = 3,
    .packet_type = LWQMS_PACKET_TYPE_MESSAGE,
    .payload = test_payload
};

lora_setup_t lora_phy_setup = {
    .hw = &context_radio_0,
    .mod_setting = &prototyping_mod_params,
    .operation_timeout_ms = 10000,
    .pa_setting = &sx1262_22dBm_pa_params,
    .pkt_setting = &prototyping_pkt_params,
    .ramp_time = SX126X_RAMP_200_US,
    .rx_interrupt_setting = &prototyping_irq_masks,
    .tx_interrupt_setting = &prototyping_irq_masks,
    .txPower = 22,
    .sync_word = LWQMS_SYNC_WORD,
};

int main()
{
    //system_setup();

    init_usb_console_hal();

    wait_for_usb_console_connection_hal();

    print_banner();

    initialize_gpio();

    printf("READY\n\n");

    uint8_t txBuf[LWQMS_PKT_LEN_MAX];
    
    if (!lwqms_pkt_encode(&test_tx_pkt, txBuf, LWQMS_PKT_LEN_MAX)) { printf("BAD FORMAT!"); while (1) {}}
    
    lora_pkt_t txPkt;
    memcpy(txPkt.buf, txBuf, LWQMS_PKT_LEN_MAX);
    txPkt.len = LWQMS_PKT_LEN_MAX;

    while (1) {
        #ifndef RX
        usb_console_write_hal("To transmit a packet, press 't'.\n");
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        rdt3_0_transmit((rdt_packet_t)(&txPkt), LWQMS_PKT_LEN_MAX, &lora_phy_setup);
        #else 
        usb_console_write_hal("Ready to receive a packet!\n");

        lwqms_pkt_t rxPacket;

        rdt3_0_receive((rdt_packet_t)(&rxPacket), sizeof(lwqms_pkt_t), &lora_phy_setup);

        printf("Received packet (outer layer):\n\n");
        hexdump(rxPacket.payload.message, LWQMS_PKT_LEN_MAX, 0x00);
        printf("\n\n");
        #endif
    }
}



/* --- EOF ------------------------------------------------------------------ */