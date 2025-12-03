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
#include <stdio.h>

#define POWER_5V_COOLDOWN_DURATION_MS 10000
#define NODE_SLEEP_INTERVAL_MINS 11

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


static uint8_t packet_id;

void print_banner(void) {
    printf("\n\n-- LoRa Water Quality Management System Sensor Node --\n");
    printf("Version 1.0, compiled %s, %s\n\n", __DATE__, __TIME__);
}

void blink_status_ok()
{
    for (int k = 0; k < 6; k++)
    {
        gpio_toggle_hal(STATUS_LED);
        sleep_ms(250);
    }
}

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

sensor_acquisition_settings_t sen_acq_settings = {
    .analog_characteristic_turb = {
        .dc_offset_pos = 0,
        .dc_offset_neg = 3,             // Shift ~4.1V input to 1.1V
        .output_reference_offset = 0,
        .gain = 1.8                     // 1.98V max input. 
    },
    .analog_characteristic_temp = {
        .dc_offset_pos = 0,
        .dc_offset_neg = 0,
        .output_reference_offset = 0,
        .gain = 2.2                       
    },
    .analog_characteristic_pH = {   // Target pH region: 5-10 -> Vlo = 2.2V, Vhi = 2.9V, BUT output must always be positive. pH 0 = 1.5V --> No greater DC- offset than this.
        .dc_offset_pos = 0,
        .dc_offset_neg = 1.4,       // Input range = 0.8V --> 1.5V. Maximum allowable gain = ~1.5
        .output_reference_offset = 0,
        .gain = 1.3,
    }
};

union sdia_cal_memsaver_u {
    sdia_potentiometer_full_calibration_t cal;
    uint8_t buf[sizeof(sdia_potentiometer_full_calibration_t)];
};

void sdia_get_and_write_calibration() {
    union sdia_cal_memsaver_u cal;

    sdia_calibrate(&context_sdia_0, &(cal.cal));

    printf("Erasing Old Calibration Data...");
    mxl23l3233f_erase_32kb_block(&context_flash_0, FLASH_ADDR_SDIA_CAL_DATA_32K_BLOCK);
    printf("DONE\n");

    printf("Writing Calibration Data to Memory...");
    mxl23l3233f_write_data(&context_flash_0, cal.buf, sizeof(sdia_potentiometer_full_calibration_t), FLASH_ADDR_SDIA_CAL_DATA_32K_BLOCK * FLASH_BLOCK_32KB_SIZE);
    printf("DONE\n");
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
    "-> l | Clear the current software-defined instrumentation amplifier calibration data\n"
    "-> f | Calibrate the Software-Defined Instrumentation Amplifier.\n"
    "-> w | Print the current Software-Defined Instrumentation Amplifier Calibration Data to the Console.\n"
    "-> q | Acquire sensor telemetry autonomously\n"
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
                SET_5V_RAIL_STATUS(false);
                sleep_ms(POWER_5V_COOLDOWN_DURATION_MS);    // No smoke please!
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
                printf("Writing Configuration...");
                sdia_apply_wiper_setting(&context_sdia_0, &wiper_setting);
                printf("DONE\n\n");
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

                printf("\tOK\n\n");

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
                printf("ADC Voltage: %f V\n", voltage_raw);
                sdia_analog_characteristic_t analog_behavior;
                sdia_convert_wiper_setting(&context_sdia_0, &sdia_calibration, &wiper_setting, &analog_behavior);
                double processed_voltage = sdia_process_raw_voltage(voltage_raw, &analog_behavior);
                printf("\n\nCalculated Input Voltage (based on last used software-defined instrumentation amplifier configuration): \x1b[1m%f V\x1b[0m", processed_voltage);
                break;
            case 'l':
                // Overwrite the space allocated for the configuration data with 0xff
                printf("Clearing the calibration data...");
                mxl23l3233f_erase_32kb_block(&context_flash_0, FLASH_ADDR_SDIA_CAL_DATA_32K_BLOCK);
                printf("DONE\n");
                break;
            case 'f':
                SET_5V_RAIL_STATUS(true);
                sdia_get_and_write_calibration();
                break;
            case 'w':
                sdia_print_calibration(&sdia_calibration);
                break;
            case 'q':
                SET_5V_RAIL_STATUS(true);
                sensor_telemetry_t sensor_data;
                sensors_acquire_data(&context_sdia_0, &sdia_calibration, &sen_acq_settings, &sensor_data);
                printf("Telemetry: Turbidity = %f NTU, Temperature = %f C, pH = %f\n\n", sensor_data.turbidity_NTU, sensor_data.temperature_C, sensor_data.pH);
            default:
                printf("Invalid Option: %c", selection);
                break;              
        }

        printf("\n\n\n");
    }
}

void system_setup() {
    init_usb_console_hal();

    //wait_for_usb_console_connection_hal();

    print_banner();

    lwqms_post_err_codes_t post_result = power_on_self_test();

    uint32_t novo_contents[MCU_POWMAN_NOVO_ELEMENTS];
    size_t novo_contents_len;

    if (check_for_power_saving_mode_boot(novo_contents, MCU_POWMAN_NOVO_ELEMENTS, &novo_contents_len)) {
        printf("Dormant state boot detected!\n");
        packet_id = novo_contents[0];
        for (int status = 1; status <= 0; --status) {
            for (uint8_t k = ERR_LED; k <= STATUS_LED; --k) {
                gpio_write_hal(k, status);
                sleep_ms(250);
            }
        }
    } else {
        printf("Power cycle boot detected!\n");
        packet_id = 1;
    }

    packet_id = check_for_power_saving_mode_boot(novo_contents, MCU_POWMAN_NOVO_ELEMENTS, &novo_contents_len) ? novo_contents[0] : 1;

    switch (post_result) {
        case POST_BYPASS:
            printf("POST Bypassed due to previous pass before dormant state.\n\n");
            blink_status_ok();
            break;
        case POST_OK:
            printf("POST Successful!\n\n");
            blink_status_ok();
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
        case POST_ERR_NO_SDIA_CALIBRATION:
            err_raise(ERR_POST_FAIL, ERR_SEV_NONFATAL, "Could not find SDIA calibration data!", "power_on_self_test");
            gpio_write_hal(ERR_LED, GPIO_HIGH);

            // Get a new calibration
            sdia_get_and_write_calibration();
            printf("Waiting for powe cooldown time...");
            gpio_write_hal(EN_5V, GPIO_LOW);
            sleep_ms(POWER_5V_COOLDOWN_DURATION_MS);
            printf("DONE\n");
            printf("Rebooting...");
            sleep_ms(250);
            reboot();
        default:
            err_raise(ERR_POST_FAIL, ERR_SEV_FATAL, "Failed to POST! Error Code = %d", "power_on_self_test");
            while (1);
    }

    absolute_time_t startup_time = make_timeout_time_ms(3000);

    if (is_usb_console_connected_hal()) {
        printf("To interrupt normal startup and enter the startup menu, press 'm'...\n");
    
        while (get_absolute_time() < startup_time) {
            if ((usb_console_getchar_timeout_us_hal(1000) | (1 << 5)) == 'm') { // Force user input to be lowercase
                startup_menu();
            }
        }
    }
}

static rp2350_power_mgmt_setting_t mcu_all_on = {
    .domains = {
        .swcore_enable = true,
        .xip_enable = true,
        .sram0_enable = true,
        .sram1_enable = true
    }
};

int main()
{
    system_setup();

    // FSM
    lwqms_fsm_t state = LWQMS_FSM_STATE_SAMPLE;

    sensor_telemetry_t sensor_data;

    while(1) {
        /*
        printf("Advance FSM? (y) Next State = 0x%08x\n", state);
        while ((usb_console_getchar_hal() | (1 << 5)) != 'y');
        printf("advancing!\n");
        */
        switch (state) {
            case LWQMS_FSM_STATE_SAMPLE:
                /**
                 * In the SAMPLE state, the sensor node obtains analog voltage readings from each sensor, converts them to their relevant measurements,
                 * and saves them to RAM. The next state is the TRANSMIT state.
                 */
                sensors_acquire_data(&context_sdia_0, &sdia_calibration, &sen_acq_settings, &sensor_data);
                printf("Telemetry: Turbidity = %f NTU, Temperature = %f C, pH = %f\n\n", sensor_data.turbidity_NTU, sensor_data.temperature_C, sensor_data.pH);
                gpio_write_hal(EN_5V, GPIO_LOW);
                state = LWQMS_FSM_STATE_TRANSMIT;
                break;
            case LWQMS_FSM_STATE_TRANSMIT:
                /**
                 * In the TRANSMIT state, the sensor node converts the telemetry data to a transmittable packet, and uses the LoRa modulator to send the data
                 * to the gateway. Having completed its duty, the next state is the DORMANT state.
                 */
                lwqms_pkt_t telem_packet = {
                    .src_id = sys_configuration.ID,
                    .dest_id = sys_configuration.gateway_ID,
                    .packet_type = LWQMS_PACKET_TYPE_TELEMETRY,
                    .pkt_id = packet_id,
                    .payload = {    // Payload is a union type that spans both a char[12] and 3 x floats (4 bytes)
                        .telemetry = {
                            .turbidity_measurement = sensor_data.turbidity_NTU,
                            .temperature_measurement = sensor_data.temperature_C,
                            .pH_measurement = sensor_data.pH
                        }
                    }
                };

                lora_pkt_t tx_pkt = {
                    .len = LWQMS_PKT_LEN_MAX
                };

                lwqms_pkt_encode(&telem_packet, tx_pkt.buf, tx_pkt.len);

                printf("Transmitting packet...\n\n");
                rdt3_0_transmit(&tx_pkt, sizeof(lora_pkt_t), &lora_phy_setup);
                printf("\n\n-- Transmit Operation Complete --\n");
                packet_id++;
                state = LWQMS_FSM_STATE_DORMANT;
                break;
            case LWQMS_FSM_STATE_DORMANT:
                /**
                 * In the DORMANT state, the sensor node deactivates any 5V peripherals (although this function is technically performed 
                 * after sensor acquisition is complete), puts the LoRa modulator in its deep sleep mode, and enters its MCU's lowest power
                 * state. After waking up, the MCU will have a cold reset, making the next state the SAMPLE state.
                 */
                uint32_t novo_mem_contents[1] = {packet_id};
                enter_power_saving_mode(&power_mgmt_dormant_state, &context_radio_0, 60 * 1000 * NODE_SLEEP_INTERVAL_MINS, novo_mem_contents, 1);
                state = LWQMS_FSM_STATE_RESET;
                break;
            default:
                state = LWQMS_FSM_STATE_SAMPLE;
                break;
        }
    }
}



/* --- EOF ------------------------------------------------------------------ */