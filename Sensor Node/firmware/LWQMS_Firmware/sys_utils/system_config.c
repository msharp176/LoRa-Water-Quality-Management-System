/******************************************************************************************************************** 
*   
*   @file system_config.c
*
*   @brief System Configuration Data & Methods for the LoRa Water Quality Management System Sensor Node
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "system_config.h"
#include <stdint.h>
#include <errno.h>
#include <limits.h>

node_config_t sys_configuration;

static int compare_bytes(const void *a, const void *b) {
    return (*(uint8_t*)a - *(uint8_t*)b);
}

uint16_t string_to_uint16_t(const char *str, int base) {
    // Yeah yeah -1 is not technically unsigned, but its better than returning 0. Assumes the user doesn't enter UINT16_T_MAX - 1

    char *endptr;
    errno = 0; // Clear errno before the call

    unsigned long val = strtoul(str, &endptr, base);

    // Check for errors
    if (errno == ERANGE || val > UINT16_MAX) {
        // Value out of range for uint16_t
        return -1;
    }

    if (endptr == str) {
        // No digits found in the string
        return -1;
    }

    if (*endptr != '\0') {
        // Trailing characters after the number
        // Handle error if strict parsing is required
        return -1;
    }

    return (uint16_t)val;
}

void print_node_configuration(node_config_t *config) {
    printf("-->Node ID: %u\n", config->ID);
    printf("-->Lat/Long: %f/%f\n", config->latitude, config->longitude);
    printf("-->LoRa Sync Word: 0x%02x", config->sync_word);
    printf("\n\n");
}

int initialize_gpio() {

    bool init_ok = false;
    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            uint8_t output_pins[] = {STATUS_LED, TX_LED, RX_LED, ERR_LED, EN_5V};
            
            // Initialize the gpio pins and assert LOW on all.
            for (int k = 0; k < sizeof(output_pins); k++) {
                gpio_setup_hal(output_pins[k], true);
                gpio_write_hal(output_pins[k], GPIO_LOW);
            }
        
            // Initialize all SPI peripherals.
        
            if (spi_init_hal(&context_spi_0) < 0) break;
            sx126x_initialize_hardware_context(&context_radio_0);
            sx126x_radio_setup(&context_radio_0);
            sx126x_interrupt_setup(&context_radio_0);

            // Initialize the SPI flash chip
            gpio_setup_hal(context_flash_0.cs, true);
            gpio_write_hal(context_flash_0.cs, GPIO_HIGH);
        
            // Initialize the i2c bus
            if (i2c_init_hal(&context_i2c_1) < 0) break;
    
            // Initialize the TMUX1309
            tmux1309_init(&context_mux_0);
            
            init_ok = true;
    
        } while (0);
    }

    return init_ok ? 0 : -1;
}

int read_system_config_data(mxl23l3233f_context_t * flash_context, node_config_t * cfgBuf) {
    
    bool spi_ok = false;

    // Initialize a buffer to store the raw configuration data.
    uint8_t rxBuf[sizeof(node_config_t)];

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // Clear the memory
            memset(rxBuf, 0x00, sizeof(node_config_t));
        
            // Read the raw data
            if (mxl23l3233f_read_data(flash_context, rxBuf, sizeof(node_config_t), FLASH_ADDR_CONFIG) < 0) break;

            spi_ok = true;

        } while (0);
    }

    if (!spi_ok) return -2;

    // Check for cleared memory
    for (int k = 0; k < sizeof(node_config_t); k++) {
        if ((rxBuf[k] != 0x00) && (rxBuf[k] != 0xff)) {
            break;
        }

        if (k == sizeof(node_config_t) - 1) {
            // Clear config - special return code.
            return -1;
        }
    }

    // Copy the buffer to a node_config_t structure.
    memcpy(cfgBuf, rxBuf, sizeof(node_config_t));
    
    return 0;
}

int write_system_config_data(mxl23l3233f_context_t * flash_context, node_config_t * config) {
    
    bool config_write_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // Initialize a uint8_t buffer to store the raw bytes of the config data
            uint8_t txBuf[sizeof(node_config_t)];
            memcpy(txBuf, config, sizeof(node_config_t));
        
            mxl23l3233f_write_data(&context_flash_0, txBuf, sizeof(node_config_t), FLASH_ADDR_CONFIG);
        
            // Check that the data was written by reading it back.
            node_config_t rxBuf_structure;
            read_system_config_data(&context_flash_0, &rxBuf_structure);
        
            // Compare the two configs byte by byte
            uint8_t rxBuf[sizeof(node_config_t)];
            memcpy(rxBuf, &rxBuf_structure, sizeof(node_config_t));
        
            if (memcmp(rxBuf, txBuf, sizeof(node_config_t)) != 0) break;

            config_write_ok = true;
        } while (0);

        if (config_write_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_FATAL, "Failed to write new configuration to SPI flash chip!", "write_system_config_data");

    return -1;

}

node_config_t get_setup_data() {

    config_start:

    printf("\n\n--Node Configuration Setup--\n\n");

    printf("Node ID:\t");
    char idbuf[5];
    uint16_t node_id_uint16_t = 0;

    do {
        int id_len = get_user_input_hal(idbuf, 5);
        node_id_uint16_t = string_to_uint16_t(idbuf, 10);

        if (node_id_uint16_t == -1) {
            printf("Bad Node ID! The ID must be a positive integer greater than zero!\n");
        }
    } while (node_id_uint16_t == -1);
    
    printf("Latitude:\t");
    char latbuf[21];    // 20 chars + null term.
    int lat_len = get_user_input_hal(latbuf, 21);
    double latitude = atof(latbuf);
    
    printf("Longitude:\t");
    char longbuf[21];
    int long_len = get_user_input_hal(longbuf, 21);
    double longitude = atof(longbuf);

    printf("Sync word (2-digit hexadecimal):\t");
    char syncbuf[3];
    int sync_word_len = get_user_input_hal(syncbuf, 3);
    uint8_t sync_word;
    sscanf(syncbuf, "%hhx", &sync_word);

    node_config_t tmp_cfg = {
        .ID = node_id_uint16_t,
        .latitude = latitude,
        .longitude = longitude,
        .sync_word = sync_word
    };

    printf("\n\n\n--Received Configuration--\n");

    print_node_configuration(&tmp_cfg);
    
    char confirmationBuf[2];
    
    do {
        printf("Configuration OK? (y/n):");
        get_user_input_hal(confirmationBuf, 2);
        printf("\n");

        confirmationBuf[0] |= (1 << 5);   // Make response lowercase.

        if ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n')) {
            printf("Bad response!\n");
        }
    } while ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n'));

    if (confirmationBuf[0] == 'n') goto config_start;

    return tmp_cfg;
    
}

lwqms_pkt_t get_custom_packet() {

    packet_start:

    printf("-- Custom Packet Entry --\n\n");

    char destination_buf[11];
    uint16_t dest_id_uint16 = 0;
    do {
        printf("Destination Device ID:\t");
        int dest_len = get_user_input_hal(destination_buf, 11);
        dest_id_uint16 = string_to_uint16_t(destination_buf, 10);   // Base 10 === decimal.

        if (dest_id_uint16 == -1) {
            printf("BAD FORMAT! The destination ID must be a positive integer!\n");
        }

    } while (dest_id_uint16 == -1);

    uint16_t packet_id_uint16 = 0;
    char packet_id_buf[11];
    do {
        printf("Packet ID:\t");
        int packet_id_len = get_user_input_hal(packet_id_buf, 11);
        packet_id_uint16 = string_to_uint16_t(packet_id_buf, 10);
        
        if (packet_id_uint16 == -1) {
            printf("BAD FORMAT! The packet ID must be a positive integer!\n");
        }
    } while (packet_id_uint16 == -1);

    char packet_type_buf[2];
    lwqms_packet_types_t payload_type;
    packet_type_start:
    memset(packet_type_buf, 0x00, sizeof(packet_type_buf));
    printf("Send Telemetry or a Message? (t/m):");
    get_user_input_hal(packet_type_buf, sizeof(packet_type_buf));
    printf("\n");

    switch (packet_type_buf[0]) {
        case 't':
            payload_type = LWQMS_PACKET_TYPE_TELEMETRY;
            break;
        case 'm':
            payload_type = LWQMS_PACKET_TYPE_MESSAGE;
            break;
        default:
            printf("BAD INPUT!\n");
            goto packet_type_start;
    }

    lwqms_pkt_payload_t payload;
    
    char telem_buf[20];
    
    if (payload_type == LWQMS_PACKET_TYPE_TELEMETRY) {
        turb_start:
        printf("Turbidity measurement (must be > 0):\t");
        memset(telem_buf, 0x00, sizeof(telem_buf));
        get_user_input_hal(telem_buf, sizeof(telem_buf));
        float turb = atoff(telem_buf);
        if (turb <= 0) {
            printf("BAD INPUT!\n");
            goto turb_start;
        }

        temp_start:
        printf("Temperature measurement (must be > 0):\t");
        memset(telem_buf, 0x00, sizeof(telem_buf));
        get_user_input_hal(telem_buf, sizeof(telem_buf));
        float temp = atoff(telem_buf);
        if (temp <= 0) {
            printf("BAD INPUT!\n");
            goto temp_start;
        }

        pH_start:
        printf("pH measurement (must be > 0):\t");
        memset(telem_buf, 0x00, sizeof(telem_buf));
        get_user_input_hal(telem_buf, sizeof(telem_buf));
        float pH = atoff(telem_buf);
        if (pH <= 0) {
            printf("BAD INPUT!\n");
            goto pH_start;
        }

        payload.telemetry.turbidity_measurement = turb;
        payload.telemetry.temperature_measurement = temp;
        payload.telemetry.pH_measurement = pH;
    }
    else {
        char msg_buf[13];
        memset(msg_buf, 0x00, sizeof(msg_buf));
        printf("Packet Message Text:\t");
        int msg_len = get_user_input_hal(msg_buf, 13);
        memcpy(payload.message, msg_buf, sizeof(lwqms_pkt_payload_t));
    }
    

    lwqms_pkt_t packet_to_send = {
        .src_id = sys_configuration.ID,
        .dest_id = dest_id_uint16,
        .pkt_id = packet_id_uint16,
        .packet_type = payload_type,
        .payload = payload
    };

    printf("\n\n\n");
    lwqms_packet_display(&packet_to_send);
    
    char confirmationBuf[2];
    
    do {
        printf("Packet Data OK? (y/n):");
        get_user_input_hal(confirmationBuf, 2);
        printf("\n");

        confirmationBuf[0] |= (1 << 5);   // Make response lowercase.

        if ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n')) {
            printf("Bad response!\n");
        }
    } while ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n'));

    if (confirmationBuf[0] == 'n') goto packet_start;

    return packet_to_send;
}

#define GET_WIPER_SETTING(message, result_var)              \
    do {                                                     \
        char wiper_setting_buf[11];                          \
                                                            \
        do {                                                 \
            memset(wiper_setting_buf, 0x00, sizeof(wiper_setting_buf)); \
            printf(message);                                 \
            get_user_input_hal(wiper_setting_buf, 11);       \
            result_var = string_to_uint16_t(wiper_setting_buf, 10); \
                                                            \
            if (result_var > 256) {                          \
                printf("BAD WIPER Setting! The wiper setting should be a positive integer between 0 and 256.\n"); \
            }                                                \
        } while (result_var > 256);                          \
    } while (0);


sdia_wiper_settings_t get_wiper_setting() {

    /*
    typedef struct sdia_wiper_settings_s {
        uint16_t dc_pos_wiper_setting;
        uint16_t dc_neg_wiper_setting;
        uint16_t gain_wiper_a_setting;
        uint16_t gain_wiper_b_setting;
        uint16_t ref_out_wiper_a_setting;
        uint16_t ref_out_wiper_b_setting;
    } sdia_wiper_settings_t;
    */

    wiper_start:

    sdia_wiper_settings_t wiper_setting;

    GET_WIPER_SETTING("DC Positive Wiper Setting:\t", wiper_setting.dc_pos_wiper_setting);
    GET_WIPER_SETTING("DC Negative Wiper Setting:\t", wiper_setting.dc_neg_wiper_setting);
    GET_WIPER_SETTING("Gain Wiper Top Setting:\t\t", wiper_setting.gain_wiper_a_setting);
    GET_WIPER_SETTING("Gain Wiper Bottom Setting:\t", wiper_setting.gain_wiper_b_setting);
    GET_WIPER_SETTING("Output Reference Top Wiper Setting:\t", wiper_setting.ref_out_wiper_a_setting);
    GET_WIPER_SETTING("Output Reference Bottom Wiper Setting:\t", wiper_setting.ref_out_wiper_b_setting);

    sdia_print_wiper_setting(&wiper_setting);

    char confirmationBuf[2];
    
    do {
        printf("Wiper Setting OK? (y/n):");
        get_user_input_hal(confirmationBuf, 2);
        printf("\n");

        confirmationBuf[0] |= (1 << 5);   // Make response lowercase.

        if ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n')) {
            printf("Bad response!\n");
        }
    } while ((confirmationBuf[0] != 'y') && (confirmationBuf[0] != 'n'));

    if (confirmationBuf[0] == 'n') goto wiper_start;

    return wiper_setting;

}

lwqms_post_err_codes_t power_on_self_test() {

    lwqms_post_err_codes_t return_code = POST_OK;

    do {
        // 1. Initialize the GPIO pins, SPI bus, and I2C Bus
        printf("-- Begin POST --\n");
        printf("Initializing GPIO...");
        if (initialize_gpio() < 0) {
            return_code = POST_ERR_GPIO_INIT; 
            break;
        }
        printf("DONE\n");


        // 2. Read in the system configuration from the SPI flash chip.
        printf("Obtaining Configuration Data...");
        int config_result = read_system_config_data(&context_flash_0, &sys_configuration);
        if (config_result == -1) {
            return_code = POST_ERR_NO_CONFIG_EXISTS;   
            break;
        }
        else if (config_result == -2) {
            return_code = POST_ERR_SPI_FLASH_FAIL; 
            break;
        }
        printf("DONE\n");


        // 3. Initialize the radio
        printf("Testing Communication with Radio...");
        if (sx126x_radio_setup(&context_radio_0) != SX126X_STATUS_OK) {
            return_code = POST_ERR_RADIO_INIT_FAIL;
            break;
        }
        sx126x_interrupt_setup(&context_radio_0);
        printf("DONE\n");


        // 4. Enable the 5V rail and check for i2c connectivity
        printf("Initializing 5V Rail...");
        gpio_write_hal(EN_5V, GPIO_HIGH);
        sleep_ms(100);
        printf("DONE\n");

        // Poll the I2C Bus
        uint8_t i2c_addresses[0xff];
        uint8_t qty_available_addresses = 0x00;
        printf("Polling I2C Bus...");
        if (i2c_get_available_addresses_hal(&context_i2c_1, i2c_addresses, 0xff, &qty_available_addresses) < 0) {
            return_code = POST_ERR_I2C_COMMS_FAIL;
            break;
        }
        printf("DONE - Found %d available devices at addresses: ", qty_available_addresses);
        for (int k = 0; k < qty_available_addresses; k++) {
            printf("0x%02x\t", i2c_addresses[k]);
        }
        printf("\n");

        // Check for the target devices to be found in the system.
        uint8_t target_addresses[] = {context_digipot_gain.addr, context_digipot_offset.addr, context_digipot_reference.addr, context_adc_0.addr};
        bool addresses_found[sizeof(target_addresses)];
        
        if (qty_available_addresses != sizeof(target_addresses)) {
            return_code = POST_ERR_I2C_DEVICE_NOT_DETECTED;
            break;
        }

        // Sort the target addresses in case any of them change down the road
        qsort(target_addresses, sizeof(target_addresses), sizeof(uint8_t), compare_bytes);

        if (memcmp(target_addresses, i2c_addresses, sizeof(target_addresses)) != 0) {
            return_code = POST_ERR_I2C_DEVICE_NOT_DETECTED;
            break;
        }

    } while (0);

    // Leave the 5V rail on if it has already been turned on... hopefully lets everything settle out an minimize magic smoke???
    if (gpio_read_hal(EN_5V)) {
        printf("Leaving 5V rail on for 10s...");
        sleep_ms(10000);
        printf("DONE\n");
        
        // Disable the 5V rail
        gpio_write_hal(EN_5V, GPIO_LOW);
    }

    
    return return_code;
}