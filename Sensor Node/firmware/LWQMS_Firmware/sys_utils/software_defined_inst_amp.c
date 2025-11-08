/******************************************************************************************************************** 
*   
*   @file software_defined_inst_amp.c
*
*   @brief Interfacing Functions for the Software-Defined Instrumentation Amplifier for the LoRa Water Quality Management System
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "software_defined_inst_amp.h"

void sdia_print_wiper_setting(sdia_wiper_settings_t *setting) {
    printf("DC Positive Wiper Setting:\t %u\n", setting->dc_pos_wiper_setting);
    printf("DC Negative Wiper Setting:\t %u\n", setting->dc_neg_wiper_setting);
    printf("Gain Wiper Top Setting:\t\t %u\n", setting->gain_wiper_a_setting);
    printf("Gain Wiper Bottom Setting:\t %u\n", setting->gain_wiper_b_setting);
    printf("Output Reference Top Wiper Setting:\t %u\n", setting->ref_out_wiper_a_setting);
    printf("Output Reference Bottom Wiper Setting:\t %u\n", setting->ref_out_wiper_b_setting);
}

bool sdia_apply_wiper_setting(sdia_context_t *context, sdia_wiper_settings_t *setting) {
    
    bool wiper_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // 2. Write the input DC offset setting
            if (mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A, setting->dc_pos_wiper_setting) < 0) break;
            if (mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_B, setting->dc_neg_wiper_setting) < 0) break;
            
            printf("Wrote DC Offset!\n");
            
            // 1. Write the gain setting
            if (mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, setting->gain_wiper_a_setting) < 0) break;
            printf("Wrote Gain Top!\n");
            if (mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, setting->gain_wiper_b_setting) < 0) break;
            printf("Wrote Gain Bottom!\n");


            // 3. Write the output reference offset setting
            if (mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_A, setting->ref_out_wiper_a_setting) < 0) break;
            if (mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_B, setting->ref_out_wiper_b_setting) < 0) break;

            printf("Wrote Reference Levels!");

            wiper_ok = true;
            return wiper_ok;
        } while (0);
    }

    err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_FATAL, "Failed to write digipot wiper values!", "sdia_apply_wiper_setting");

    return false;
}

bool sdia_convert_wiper_setting(sdia_context_t *context, sdia_wiper_settings_t *setting_in, sdia_analog_setting_t *analog_characteristic) {

    // RGain = (RAW0 + RAW1) * Rs
    uint16_t RAW0_gain = (context->context_digipot_gain->total_steps - 1) - setting_in->gain_wiper_a_setting;
    uint16_t RAW1_gain = (context->context_digipot_gain->total_steps - 1) - setting_in->gain_wiper_b_setting;
    double r_gain = (RAW0_gain + RAW1_gain) * context->context_digipot_gain->resistance_per_step;

    // Set the gain value
    analog_characteristic->gain = 1 + ((2 * INST_AMP_R0) / r_gain);

    // The DC offset is simply a voltage divider!
    analog_characteristic->dc_offset_pos = (setting_in->dc_pos_wiper_setting / (context->context_digipot_dc_offset->total_steps - 1)) * DC_OFFSET_POT_INPUT_VOLTAGE;
    analog_characteristic->dc_offset_neg = (setting_in->dc_neg_wiper_setting / (context->context_digipot_dc_offset->total_steps - 1)) * DC_OFFSET_POT_INPUT_VOLTAGE;

    // The reference voltage = V+ * (RWB1 / (RAW0 + RWB1))
    uint16_t RAW0_ref = (context->context_digipot_output_reference->total_steps - 1) - setting_in->ref_out_wiper_a_setting;
    uint16_t RWB1_ref = setting_in->ref_out_wiper_b_setting;

    analog_characteristic->output_reference_offset = OUTPUT_REFERENCE_POT_INPUT_VOLTAGE * (RWB1_ref / (RAW0_ref + RWB1_ref));

    return true;
}

bool sdia_read_raw(sdia_context_t *context, double *voltage) {

    bool reading_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            if (!mcp3425_get_measurement(context->context_adc, voltage)) break;

            reading_ok = true;
        } while (0);

        if (reading_ok) return true;
    }

    err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_FATAL, "Communications failure with ADC!", "sdia_read_raw");

    return false;
}

double sdia_process_raw_voltage(double raw_voltage, sdia_analog_setting_t *analog_characteristic) {

    // Vout = (Vin + Voffset+ - Voffset-) * Gain + Vref --> Let Vin = Vin+ - Vin-

    double Vin_with_offsets = (raw_voltage - analog_characteristic->output_reference_offset) / analog_characteristic->gain;

    return Vin_with_offsets - analog_characteristic->dc_offset_pos + analog_characteristic->dc_offset_neg;
}

bool sdia_acquire(sdia_context_t *context, sdia_sensor_types_t sensor_input, sdia_wiper_settings_t *wiper_setting, double *reading) {

    // 1. Write the digipot settings to the system
    sdia_apply_wiper_setting(context, wiper_setting);

    // 2. Enable the correct sensor input
    tmux1309_set_output(context->context_mux, sensor_input);

    // 3. Allow ~10ms time for stability
    sleep_ms(10);

    // 4. Take a measurement
    double raw_reading = 0;
    sdia_read_raw(context, &raw_reading);

    // 5. Get the analog equivalent of the wiper setting
    sdia_analog_setting_t analog_behavior;
    sdia_convert_wiper_setting(context, wiper_setting, &analog_behavior);

    // 6. Obtain the original input voltage
    *reading = sdia_process_raw_voltage(raw_reading, &analog_behavior);

    return true;
}

#define RAISE_CAL_I2C_ADC_ERR() err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_FATAL, "Failed to read measurement from ADC!", "sdia_calibrate")

bool sdia_calibrate(sdia_context_t *context, sdia_potentiometer_full_calibration_t *full_calibration_buffer) {

    printf("\n\n-- Software-Defined Instrumentation Amplifier Calibration --\n");
    printf("PLEASE NOTE: This process requires a DMM and access to the physical PCB to complete.\n");
    
    char selectionBuf[2];
    memset(selectionBuf, 0x00, sizeof(selectionBuf));
    
    do {
        printf("Are you sure you wish to continue? (y/n):\t");
        get_user_input_hal(selectionBuf, 2);
        printf("\n");

        selectionBuf[0] |= (1 << 5);   // Make response lowercase.

        if ((selectionBuf[0] != 'y') && (selectionBuf[0] != 'n')) {
            printf("Bad response!\n");
        }
    } while ((selectionBuf[0] != 'y') && (selectionBuf[0] != 'n'));

    if (selectionBuf[0] == 'n') return false;

    // Initialize the ADC
    mcp3425_init(context->context_adc, MCP3425_SPS_15_16BITS, MCP3425_PGA_1, false);

    // 1. Manual portion of the process - set the DC positive wiper to 0.1V
    printf("\n\nPlease begin by tying the Extra input to GND and setting the DC Positive Offset Voltage (TP 15) to 0.1V. A guess has been made on the wiper for you.\n");
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A, 5);

    memset(selectionBuf, 0x00, sizeof(selectionBuf));

    do {
        printf("To increment the wiper, press 'i'. To decrement, press 'd'. If the setting is OK, press 'y':\t");
        get_user_input_hal(selectionBuf, 2);
        printf("\n");

        selectionBuf[0] |= (1 << 5);   // Make response lowercase.

        switch (selectionBuf[0]) {
            case 'y':
                break;
            case 'i':
                printf("incrementing...\n");
                mcp4651_increment_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A);
                break;
            case 'd':
                printf("decrementing...\n");
                mcp4651_decrement_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A);
                break;
            default:
                printf("Bad Input: %c\n", selectionBuf[0]);
                break;
        }
    } while ((selectionBuf[0] != 'y'));

    char measurementBuf[10];
    memset(measurementBuf, 0x00, sizeof(measurementBuf));
    double dc_pos_cal_value = 0;

    do {
        printf("\n\nPlease enter the exact DC Positive Offset Voltage measured on the DMM:\t");
        get_user_input_hal(measurementBuf, sizeof(measurementBuf));
        printf("\n");

        dc_pos_cal_value = atof(measurementBuf);

        if (dc_pos_cal_value == 0) {
            printf("Error parsing input!\n");
        }
    } while (dc_pos_cal_value == 0);

    printf("\n\nBeginning calibration procedure based on measurement %f at wiper position %u", dc_pos_cal_value, context->context_digipot_dc_offset->wiper_position_a);

    /* BEGIN AUTOMATIC CALIBRATION */

    // Zero out the reference and negative DC offset
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_B, 0x00);
    mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_BOTH, 0x00);

    // Calibrate the top half of the gain pot - set the bottom wiper to a short circuit and set the top wiper to full resistance.
    mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, 0xff);
    mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, 0x00);

    // Enable input 3 on the mux
    tmux1309_set_output(context->context_mux, SDIA_SENSOR_TYPE_EXTRA);

    // Start by measuring the full wiper resistance. Then, increment the wiper and calculate the resistances from there
    double voltage_reading_buf = 0; // Stores raw voltage readings off of the ADC
    double gain_buf = 0;            // Stores raw gain values
    double gain_resistance_buf = 0; // Stores the resistance from the A terminal to the wiper
    double rwb_buf = 0;

    for (int k = 0; k < 0x100; k++) {
        // 1. Read the measurement from the ADC
        if (!mcp3425_get_measurement(context->context_adc, &voltage_reading_buf)) RAISE_CAL_I2C_ADC_ERR();

        if (voltage_reading_buf > 2.04) break;

        // 2. Calculate the measured gain
        gain_buf = voltage_reading_buf / dc_pos_cal_value;
        gain_resistance_buf = (2 * INST_AMP_R0) / (gain_buf - 1);

        // If we are on the first iteration, set up all of our baseline vars
        if (k == 0) {
            context->context_digipot_gain->base_resistance_a = gain_resistance_buf;
            printf("The full wiper resistance for the top gain pot is %f Ohms, with a gain of %f.\n", gain_resistance_buf, gain_buf);
        }

        // 3. Calculate the RWB from the measured gain resistance and save it to the calibration profile.
        rwb_buf = context->context_digipot_gain->base_resistance_a - gain_resistance_buf;
        full_calibration_buffer->GainUpper_calibration[k].r_wb = rwb_buf;

        printf("Step = %d, Voltage = %f, Gain = %f, RAW = %f, RWB = %f\n", k, voltage_reading_buf, gain_buf, gain_resistance_buf, rwb_buf);

        // 4. Increment the wiper
        mcp4651_increment_wiper(context->context_digipot_gain, MCP4651_WIPER_A);
    }

    printf("DONE\n");
}