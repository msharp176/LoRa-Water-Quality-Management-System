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

#include <stdio.h>

#define SET_5V_RAIL_STATUS(is_enabled)                                \
    do {                                                              \
        /* Check if the EN_5V pin is already in the desired state */   \
        if (gpio_read_hal(EN_5V) != (is_enabled ? GPIO_HIGH : GPIO_LOW)) { \
            printf("Waiting for power cooldown...");                  \
            while (get_absolute_time() < power_5v_cooldown_time) {}   \
            printf("DONE\n");                                          \
                                                                  \
            printf("Setting 5V Rail %s...", (is_enabled ? "ON" : "OFF")); \
            gpio_write_hal(EN_5V, (is_enabled ? GPIO_HIGH : GPIO_LOW)); \
            printf("DONE\n");                                         \
        }                                                             \
                                                                    \
        power_5v_cooldown_time = make_timeout_time_ms(POWER_5V_COOLDOWN_DURATION_MS); \
                                                                                        \
        /* Perform an I2C Scan ???? No clue why but it fixes issues with comms on the bus */ \
        uint8_t buf[0xff]; \ 
        uint8_t bufidx; \
        i2c_get_available_addresses_hal(&context_i2c_1, buf, 0xff, &bufidx);                      \
    } while (0)


//#define LWQMS_DEDICATED_RECEIVER

void print_banner(void) {
    printf("\n\n-- LoRa Water Quality Management System Sensor Node --\n");
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
    "-> t | Transmit a packet\n"
    "-> a | Configure the software-defined instrumentation amplifier\n"
    "-> m | Take a measurement using the ADC.\n"
    "-> r | Reboot the device - updates device settings with any changes made here.\n";
    
    absolute_time_t power_5v_cooldown_time = get_absolute_time();

    sdia_wiper_settings_t wiper_setting = { // Initialize all wipers at the midpoint.
        .dc_neg_wiper_setting = 0x80,
        .dc_pos_wiper_setting = 0x80,
        .gain_wiper_a_setting = 0x80,
        .gain_wiper_b_setting = 0x80,
        .ref_out_wiper_a_setting = 0x80,
        .ref_out_wiper_b_setting = 0x80
    };

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
                SET_5V_RAIL_STATUS(selection == 'e');
                if (selection == 'd') {
                    // Reset the potentiometer positions as they will re-initialize to the mid-point upon 5V power cycle.
                    wiper_setting.dc_neg_wiper_setting = 0x80;
                    wiper_setting.dc_pos_wiper_setting = 0x80;
                    wiper_setting.gain_wiper_a_setting = 0x80;
                    wiper_setting.gain_wiper_b_setting = 0x80;
                    wiper_setting.ref_out_wiper_a_setting = 0x80;
                    wiper_setting.ref_out_wiper_b_setting = 0x80;
                }
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
            case 't':
                lwqms_pkt_t packet = get_custom_packet();
                lora_pkt_t tx_pkt;                
                tx_pkt.len = LWQMS_PKT_LEN_MAX;
                lwqms_pkt_encode(&packet, tx_pkt.buf, tx_pkt.len);
                printf("Sending packet...\n\n");
                rdt3_0_transmit(&tx_pkt, sizeof(lora_pkt_t), &lora_phy_setup);
                printf("\n\n-- Transmit Operation Complete --\n");
                break;
            case 'a':
                wiper_setting = get_wiper_setting();
                SET_5V_RAIL_STATUS(true);
                printf("\n\nWriting Configuration...");
                sdia_apply_wiper_setting(&context_sdia_0, &wiper_setting);
                printf("DONE\n");
                break;
            case 'm':
                uint16_t input_selection;

                do {
                    printf("Poll from which input? [0-3]: \t");
                    char inputBuf[2];
                    get_user_input_hal(inputBuf, 2);
                    input_selection = string_to_uint16_t(inputBuf, 10);
                    if (input_selection > 3) {
                        printf("Bad Input!\n");
                    }
                } while (input_selection > 3);

                SET_5V_RAIL_STATUS(true);
                printf("Setting the input...");
                tmux1309_set_output(&context_mux_0, input_selection);
                printf("DONE\n");
                printf("Setting up ADC...");
                mcp3425_init(&context_adc_0, MCP3425_SPS_15_16BITS, MCP3425_PGA_1, false);
                printf("DONE\n");
                printf("Polling ADC...");
                double voltage_raw = 0;
                sdia_read_raw(&context_sdia_0, &voltage_raw);
                printf("DONE\n");
                printf("Raw Input Voltage: %f\n", voltage_raw);
                printf("Last used wiper setting:\n");
                sdia_print_wiper_setting(&wiper_setting);
                sdia_analog_characteristic_t analog_behavior;
                sdia_convert_wiper_setting(&context_sdia_0, &wiper_setting, &analog_behavior);
                double processed_voltage = sdia_process_raw_voltage(voltage_raw, &analog_behavior);
                printf("\n\nCalculated Input Voltage: %f", processed_voltage);
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

node_config_t tx_config = {
  .ID = 3,
  .sync_word = 0x42,
  .latitude = 40.2732,
  .longitude = 76.8867
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
    .node_config = &sys_configuration,
};



int main()
{
    system_setup();

    absolute_time_t power_5v_cooldown_time = get_absolute_time();
    SET_5V_RAIL_STATUS(true);

    sdia_potentiometer_full_calibration_t cal;

    sdia_calibrate(&context_sdia_0, &cal);

    while(1);

    while (1) {
        #ifdef LWQMS_DEDICATED_RECEIVER
        usb_console_write_hal("Ready to receive a packet!\n");

        lora_pkt_t rxPacket;
        rdt3_0_receive((rdt_packet_t)(&rxPacket), sizeof(lwqms_pkt_t), &dedicated_receiver_setup);
        
        lwqms_pkt_t processed_packet;
        lwqms_pkt_decode(rxPacket.buf, rxPacket.len, &processed_packet);

        printf("Received packet (outer layer):\n\n");
        hexdump(processed_packet.payload.message, sizeof(lwqms_pkt_payload_t), 0x00);
        printf("\n\n");

        #else
        #endif
    }
}



/* --- EOF ------------------------------------------------------------------ */