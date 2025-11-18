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
#include <math.h>

void sdia_print_wiper_setting(sdia_wiper_settings_t *setting) {
    printf("DC Positive Wiper Setting:\t %u\n", setting->dc_pos_wiper_setting);
    printf("DC Negative Wiper Setting:\t %u\n", setting->dc_neg_wiper_setting);
    printf("Gain Wiper Top Setting:\t\t %u\n", setting->gain_wiper_a_setting);
    printf("Gain Wiper Bottom Setting:\t %u\n", setting->gain_wiper_b_setting);
    printf("Output Reference Top Wiper Setting:\t %u\n", setting->ref_out_wiper_a_setting);
    printf("Output Reference Bottom Wiper Setting:\t %u\n", setting->ref_out_wiper_b_setting);
}

void sdia_print_analog_characteristic(sdia_analog_characteristic_t *characteristic) {
    printf("Positive DC Offset:\t%f\n", characteristic->dc_offset_pos);
    printf("Negative DC Offset:\t%f\n", characteristic->dc_offset_neg);
    printf("Gain:\t%f\n", characteristic->gain);
    printf("Output Reference Voltage:\t%f\n", characteristic->output_reference_offset);
}

bool sdia_apply_wiper_setting(sdia_context_t *context, sdia_wiper_settings_t *setting) {
    
    bool wiper_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            // 2. Write the input DC offset setting
            if (mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A, setting->dc_pos_wiper_setting) < 0) break;
            if (mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_B, setting->dc_neg_wiper_setting) < 0) break;
            
            //printf("Wrote DC Offset!\n");
            
            // 1. Write the gain setting
            if (mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, setting->gain_wiper_a_setting) < 0) break;
            //printf("Wrote Gain Top!\n");
            if (mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, setting->gain_wiper_b_setting) < 0) break;
            //printf("Wrote Gain Bottom!\n");


            // 3. Write the output reference offset setting
            if (mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_A, setting->ref_out_wiper_a_setting) < 0) break;
            if (mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_B, setting->ref_out_wiper_b_setting) < 0) break;

            //printf("Wrote Reference Levels!");

            wiper_ok = true;
            return wiper_ok;
        } while (0);
    }

    err_raise(ERR_I2C_TRANSACTION_FAIL, ERR_SEV_FATAL, "Failed to write digipot wiper values!", "sdia_apply_wiper_setting");

    return false;
}

bool sdia_convert_wiper_setting(sdia_context_t *context, sdia_potentiometer_full_calibration_t *cal, sdia_wiper_settings_t *setting_in, sdia_analog_characteristic_t *analog_characteristic) {

    analog_characteristic->dc_offset_pos = cal->DCPos_calibration[setting_in->dc_pos_wiper_setting].v_wb;
    analog_characteristic->dc_offset_neg = cal->DCNeg_calibration[setting_in->dc_neg_wiper_setting].v_wb;
    
    double gain_resistor_top = context->context_digipot_gain->base_resistance_a - cal->GainUpper_calibration[setting_in->gain_wiper_a_setting].r_wb;
    double gain_resistor_bot = context->context_digipot_gain->base_resistance_b - cal->GainLower_calibration[setting_in->gain_wiper_b_setting].r_wb;
    double gain_resistor = gain_resistor_top + gain_resistor_bot;

    analog_characteristic->gain = ((2 * INST_AMP_R0) / gain_resistor) + 1;

    analog_characteristic->output_reference_offset = cal->RefLower_calibration[setting_in->ref_out_wiper_b_setting].v_wb;

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

bool sdia_get_wiper_setting_from_analog_characteristic( sdia_context_t *context,
                                                        sdia_analog_characteristic_t *desired_analog_characteristic, // The design specification to shoot for
                                                        sdia_potentiometer_full_calibration_t *cal,           // Calibration data
                                                        sdia_wiper_settings_t *setting,                       // Corresponding wiper settings
                                                        sdia_analog_characteristic_t *actual_analog_characteristic)  // The actual analog behavior of the amplifier given the wiper setting
{
    
    setting->ref_out_wiper_a_setting = 0x80; // Set the top wiper to midpoint - see reference characterization excel file.
    
    if (desired_analog_characteristic->output_reference_offset > cal->RefLower_calibration[MCP4651_MAX_WIPER_INDEX].v_wb) {
        char err_msg[255];
        snprintf(err_msg, 100, "Bad reference offset! The DC output reference offset must not exceed volts - the maximum supported by the digipot.\n", cal->RefLower_calibration[MCP4651_MAX_WIPER_INDEX].v_wb);
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, err_msg, "sdia_get_wiper_setting_from_analog_characteristic");
        return false;
    }
    
    // 1. Get the desired DC offset wiper values
    bool pos_found = false;
    bool neg_found = false;
    bool ref_found = false;

    for (int k = 1; k <= MCP4651_MAX_WIPER_INDEX; k++) {
        // Iterate through each list and find the closest entry
        // When we have gone past the desired analog characteristic value, decide if this step or the previous one is closer, and choose that one.
        if ((!pos_found) && (cal->DCPos_calibration[k].v_wb > desired_analog_characteristic->dc_offset_pos)) {
            double err_curr = cal->DCPos_calibration[k].v_wb - desired_analog_characteristic->dc_offset_pos;
            double err_prev = desired_analog_characteristic->dc_offset_pos - cal->DCPos_calibration[k - 1].v_wb;

            setting->dc_pos_wiper_setting = (err_prev < err_curr) ? k - 1 : k;

            pos_found = true;
        }

        if ((!neg_found) && (cal->DCNeg_calibration[k].v_wb > desired_analog_characteristic->dc_offset_neg)) {
            double err_curr = cal->DCNeg_calibration[k].v_wb - desired_analog_characteristic->dc_offset_neg;
            double err_prev = desired_analog_characteristic->dc_offset_neg - cal->DCNeg_calibration[k - 1].v_wb;

            setting->dc_neg_wiper_setting = (err_prev < err_curr) ? k - 1 : k;

            neg_found = true;
        }

        if ((!ref_found) && (cal->RefLower_calibration[k].v_wb > desired_analog_characteristic->output_reference_offset)) {
            double err_curr = cal->RefLower_calibration[k].v_wb - desired_analog_characteristic->output_reference_offset;
            double err_prev = desired_analog_characteristic->output_reference_offset - cal->RefLower_calibration[k - 1].v_wb;

            setting->ref_out_wiper_b_setting = (err_prev < err_curr) ? k - 1 : k;

            ref_found = true;
        }
    }

    if (!(pos_found && neg_found && ref_found)) return false;

    // 2. Get the desired gain value.
    double target_gain_resistor = (2 * INST_AMP_R0) / (desired_analog_characteristic->gain - 1);
    double target_remaining_resistance = (context->context_digipot_gain->base_resistance_a + context->context_digipot_gain->base_resistance_b) - target_gain_resistor;


    // Search both lists for a combination that meets the target resistance
    int g_top_idx = 0;
    int g_bot_idx = MCP4651_MAX_WIPER_INDEX;
    double min_err = __DBL_MAX__;
    double err_buf = __DBL_MAX__;
    double remaining_resistance_buf = 0;

    while ((g_top_idx < MCP4651_MAX_WIPER_INDEX) && (g_bot_idx >= 0)) {

        remaining_resistance_buf = cal->GainUpper_calibration[g_top_idx].r_wb + cal->GainLower_calibration[g_bot_idx].r_wb;

        err_buf = fabs(remaining_resistance_buf - target_remaining_resistance);

        if (err_buf < min_err) {
            min_err = err_buf;
            setting->gain_wiper_a_setting = g_top_idx;
            setting->gain_wiper_b_setting = g_bot_idx;
        }

        if (remaining_resistance_buf > target_remaining_resistance) {
            g_bot_idx--;    // If the sum is too large, decrease the larger resistor value
        }
        else {
            g_top_idx++;    // If the sum is too small, increase the smaller resistor value
        }
    }

    // Convert the given wiper setting to its equivalent analog characteristic.
    return sdia_convert_wiper_setting(context, cal, setting, actual_analog_characteristic);
}

double sdia_process_raw_voltage(double raw_voltage, sdia_analog_characteristic_t *analog_characteristic) {

    // Vout = (Vin + Voffset+ - Voffset-) * Gain + Vref --> Let Vin = Vin+ - Vin-

    double Vin_with_offsets = (raw_voltage - analog_characteristic->output_reference_offset) / analog_characteristic->gain;

    return Vin_with_offsets - analog_characteristic->dc_offset_pos + analog_characteristic->dc_offset_neg;
}

bool sdia_acquire(sdia_context_t *context, sdia_potentiometer_full_calibration_t *cal, sdia_sensor_types_t sensor_input, sdia_analog_characteristic_t *analog_setting, double *reading) {

    sdia_wiper_settings_t wiper_setting;
    sdia_analog_characteristic_t actual_analog_characteristic;
    
    bool acquisition_ok = false;
    
    do {
        // 1. Get the digipot settings corresponding to the desired analog characteristics of the amplifier
        if (!sdia_get_wiper_setting_from_analog_characteristic(context, analog_setting, cal, &wiper_setting, &actual_analog_characteristic)) break;

        // 2. Write the wiper setting to the system
        if (!sdia_apply_wiper_setting(context, &wiper_setting)) break;

        // 2. Enable the correct sensor input
        if (tmux1309_set_output(context->context_mux, sensor_input) < 0) break;

        // 3. Allow ~10ms time for stability
        sleep_ms(10);

        // 4. Take a measurement
        double raw_reading = 0;
        if (!sdia_read_raw(context, &raw_reading)) break;

        // 5. Get the analog equivalent of the wiper setting
        sdia_analog_characteristic_t analog_behavior;
        if (!sdia_convert_wiper_setting(context, cal, &wiper_setting, &analog_behavior)) break;

        // 6. Obtain the original input voltage
        *reading = sdia_process_raw_voltage(raw_reading, &analog_behavior);

        acquisition_ok = true;
    } while (0);

    return acquisition_ok;
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
        
    // Zero out the reference and negative DC offset
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_B, 0x00);
    mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_BOTH, 0x00);
    
    printf("\n\nPlease begin by tying the Extra input to GND and setting the DC Positive Offset Voltage (TP 15) to 0.1V. A guess has been made on the wiper for you.\n");
    
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_A, 5);

    // Enable input 3 on the mux
    tmux1309_set_output(context->context_mux, SDIA_SENSOR_TYPE_EXTRA);

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

    printf("\n\nBeginning calibration procedure based on measurement %f at wiper position %u\n\n", dc_pos_cal_value, context->context_digipot_dc_offset->wiper_position_a);

    /* BEGIN AUTOMATIC CALIBRATION */

    // 1. Gain Resistor 

    printf("Calibrating Gain Resistor...\n");

    #pragma region GAIN_CAL

    // Start by measuring the full wiper resistance. Then, increment the wiper and calculate the resistances from there
    double voltage_reading_buf = 0; // Stores raw voltage readings off of the ADC
    double gain_buf = 0;            // Stores raw gain values
    double gain_resistance_buf = 0; // Stores the resistance from the A terminal to the wiper
    double rwb_buf = 0;

    // Gain Resistor Writing Pointers
    double *wiper_base_resistance_ptr;

    
    for (int wiper = MCP4651_WIPER_A; wiper <= MCP4651_WIPER_B; wiper++) {
        // Assign all of the relevant pointers
        switch (wiper) {
            case MCP4651_WIPER_A:
                wiper_base_resistance_ptr = &(context->context_digipot_gain->base_resistance_a);
                // Calibrate the top half of the gain pot - set the bottom wiper to a short circuit and set the top wiper to full resistance.
                mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, 0x00);
                mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, MCP4651_MAX_WIPER_INDEX);
                break;
            case MCP4651_WIPER_B:
                wiper_base_resistance_ptr = &(context->context_digipot_gain->base_resistance_b);
                // Calibrate the bottom half of the gain pot - set the top wiper to a short circuit and set the bottom wiper to full resistance.
                mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, MCP4651_MAX_WIPER_INDEX);
                mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, 0x00);
                break;
            default:
                break;
        }

        // Test the selected wiper
        int k;
        for (k = 0; k <= MCP4651_MAX_WIPER_INDEX; k++) {
            // 1. Read the measurement from the ADC
            if (!mcp3425_get_measurement(context->context_adc, &voltage_reading_buf)) RAISE_CAL_I2C_ADC_ERR();

            if (voltage_reading_buf >= (MCP3425_MAX_VIN - 0.08)) break;

            // 2. Calculate the measured gain
            gain_buf = voltage_reading_buf / dc_pos_cal_value;
            gain_resistance_buf = (2 * INST_AMP_R0) / (gain_buf - 1);

            // If we are on the first iteration, set up all of our baseline vars
            if (k == 0) {
                *wiper_base_resistance_ptr = gain_resistance_buf;
                printf("The full wiper resistance for gain pot %d is %f Ohms, with a gain of %f.\n", wiper, gain_resistance_buf, gain_buf);
            }

            // 3. Calculate the RWB from the measured gain resistance and save it to the calibration profile.
            rwb_buf = *wiper_base_resistance_ptr - gain_resistance_buf;
            
            switch (wiper) {
                case MCP4651_WIPER_A:
                    full_calibration_buffer->GainUpper_calibration[k].r_wb = rwb_buf;
                    break;
                case MCP4651_WIPER_B:
                    full_calibration_buffer->GainLower_calibration[k].r_wb = rwb_buf;
                    break;
            }

            printf("Step = %d, Voltage = %f, Gain = %f, RAW = %f, RWB = %f\n", k, voltage_reading_buf, gain_buf, gain_resistance_buf, rwb_buf);

            // 4. Increment the wiper
            mcp4651_increment_wiper(context->context_digipot_gain, wiper);
        }

        // Extrapolate from the average resistance step
        double avg_resistance_remaining = (*wiper_base_resistance_ptr - rwb_buf) / (MCP4651_MAX_WIPER_INDEX - k + 1);

        // Finish up by extrapolating the remaining points
        for (k = k; k <= MCP4651_MAX_WIPER_INDEX; k++) {
            switch (wiper) {
                case MCP4651_WIPER_A:
                    full_calibration_buffer->GainUpper_calibration[k].r_wb = full_calibration_buffer->GainUpper_calibration[k - 1].r_wb + avg_resistance_remaining;
                    printf("Step %d (Extrapolated) - RWB = %f\n", k, full_calibration_buffer->GainUpper_calibration[k].r_wb);
                    break;
                case MCP4651_WIPER_B:
                    full_calibration_buffer->GainLower_calibration[k].r_wb = full_calibration_buffer->GainLower_calibration[k - 1].r_wb + avg_resistance_remaining;
                    printf("Step %d (Extrapolated) - RWB = %f\n", k, full_calibration_buffer->GainLower_calibration[k].r_wb);
                    break;
                default:
                    break;
            }
        }

        printf("\n\n");
    }

    #pragma endregion /* GAIN_CAL */

    printf("DONE\n");

    // 2. Positive DC Offset Wiper

    printf("Calibrating DC Offset...\n");

    #pragma region DC_OFF_CAL

    // Set the DC negative wiper to 0, the DC positive wiper to 0, and the gain (upper, lower) = (0, 256) --> Approx. Gain = 1.2
    mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_BOTH, 0);
    mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_A, 0);
    mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_B, MCP4651_MAX_WIPER_INDEX);

    double dc_offset_gain = ((2 * INST_AMP_R0) / context->context_digipot_gain->base_resistance_a) + 1;
    double dc_pos_voltage = 0;
    double dc_neg_voltage = 0;

    memset(full_calibration_buffer->DCPos_calibration, 0, MCP4651_MAX_WIPER_INDEX);
    memset(full_calibration_buffer->DCNeg_calibration, 0, MCP4651_MAX_WIPER_INDEX);

    printf("The estimated DC offset gain is %f\n", dc_offset_gain);

    /* FSM */
    #define DC_OFF_CAL_SPOS 0
    #define DC_OFF_CAL_SNEG 1

    uint8_t state = DC_OFF_CAL_SPOS;

    // Start with both wipers at index 0.
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_BOTH, 0);

    double vin_dcoff;

    while (1) {
        if (!mcp3425_get_measurement(context->context_adc, &voltage_reading_buf)) RAISE_CAL_I2C_ADC_ERR();
        vin_dcoff = voltage_reading_buf / dc_offset_gain;
        
        switch (state) {
            case DC_OFF_CAL_SPOS:
                dc_pos_voltage = fabs(vin_dcoff + dc_neg_voltage);
                full_calibration_buffer->DCPos_calibration[context->context_digipot_dc_offset->wiper_position_a].v_wb = dc_pos_voltage;

                if (voltage_reading_buf >= (MCP3425_MAX_VIN - 0.08) || (context->context_digipot_dc_offset->wiper_position_a == MCP4651_MAX_WIPER_INDEX)) {
                    printf("\n-- SWITCHING WIPER --\n\n");
                    //while ((usb_console_getchar_hal() | 0x20) != 'g') {}  // t, case insensitive
                    state = DC_OFF_CAL_SNEG;
                }
                break;

            case DC_OFF_CAL_SNEG:
                dc_neg_voltage = fabs(vin_dcoff - dc_pos_voltage);
                full_calibration_buffer->DCNeg_calibration[context->context_digipot_dc_offset->wiper_position_b].v_wb = dc_neg_voltage;
                
                if ((voltage_reading_buf <= 0.08) || (context->context_digipot_dc_offset->wiper_position_b == MCP4651_MAX_WIPER_INDEX)) {
                    printf("\n-- SWITCHING WIPER --\n\n");
                    //while ((usb_console_getchar_hal() | 0x20) != 'g') {}  // t, case insensitive
                    state = DC_OFF_CAL_SPOS;
                }
                
                break;

            default:
                break;
        }

        printf("Pos Wiper = %d, Neg Wiper = %d, Raw Voltage = %f, Sum Voltage = %f, DC+ = %f, DC- = %f\n", 
            context->context_digipot_dc_offset->wiper_position_a, context->context_digipot_dc_offset->wiper_position_b,
            voltage_reading_buf, vin_dcoff, full_calibration_buffer->DCPos_calibration[context->context_digipot_dc_offset->wiper_position_a].v_wb, full_calibration_buffer->DCNeg_calibration[context->context_digipot_dc_offset->wiper_position_b].v_wb
        );

        if ((context->context_digipot_dc_offset->wiper_position_a == MCP4651_MAX_WIPER_INDEX) && (context->context_digipot_dc_offset->wiper_position_b == MCP4651_MAX_WIPER_INDEX)) break;    // Done!

        mcp4651_increment_wiper(context->context_digipot_dc_offset, state == DC_OFF_CAL_SPOS ? MCP4651_WIPER_A : MCP4651_WIPER_B);
    }

    #pragma endregion /* DC_OFF_CAL */

    printf("DONE\n");

    // 3. Output Reference Voltage

    printf("Calibrating DC output reference voltage...\n");

    #pragma region OUT_REF_CAL

    // Zero out the offsets
    mcp4651_set_wiper(context->context_digipot_dc_offset, MCP4651_WIPER_BOTH, 0x00);
    mcp4651_set_wiper(context->context_digipot_gain, MCP4651_WIPER_BOTH, 0x80);     // Set gain to midpoint - shouldn't matter
    // Increase the output reference until we see >=2.048V.

    for (int wiper = MCP4651_WIPER_A; wiper <= MCP4651_WIPER_B; wiper++) {
        
        // The wiper references will be computed with respect to the other wiper at its midpoint - most faithful representation to work from.
        
        switch (wiper) {
            case MCP4651_WIPER_A:
                mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_A, 0);
                mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_B, 0x80);
            case MCP4651_WIPER_B:
                mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_A, 0x80);
                mcp4651_set_wiper(context->context_digipot_output_reference, MCP4651_WIPER_B, 0);
            default:
                break;
        }
        
        int k;
        for (k = 0; k <= MCP4651_MAX_WIPER_INDEX; k++) {
            if (!mcp3425_get_measurement(context->context_adc, &voltage_reading_buf)) RAISE_CAL_I2C_ADC_ERR();
    
            printf("Step = %d - Voltage = %f\n", k, voltage_reading_buf);

            switch (wiper) {
                case MCP4651_WIPER_A:
                    full_calibration_buffer->RefUpper_calibration[k].v_wb = voltage_reading_buf;
                    break;
                case MCP4651_WIPER_B:
                    full_calibration_buffer->RefLower_calibration[k].v_wb = voltage_reading_buf;
                    break;
                default:
                    break;
            }

            if (voltage_reading_buf >= (MCP3425_MAX_VIN - 0.08)) break;

            mcp4651_increment_wiper(context->context_digipot_output_reference, wiper);
        }

        // Write remaining values as 2.048V, since that is the maximum we can read.

        for (k = k + 1; k <= MCP4651_MAX_WIPER_INDEX; k++) {
            printf("Step = %d - Voltage = %f (autoset)\n", k, MCP3425_MAX_VIN);
            switch (wiper) {
                case MCP4651_WIPER_A:
                    full_calibration_buffer->RefUpper_calibration[k].v_wb = MCP3425_MAX_VIN;
                    break;
                case MCP4651_WIPER_B:
                    full_calibration_buffer->RefLower_calibration[k].v_wb = MCP3425_MAX_VIN;
                    break;
                default:
                    break;
            }
        }

    }

    #pragma endregion /* OUT_REF_CAL */

    printf("DONE\n");

    return true;
}

void sdia_print_calibration(sdia_potentiometer_full_calibration_t *cal) {
    printf("Step, DC+, DC-, G+, G-, R+, R-:\n");

    for (int k = 0; k <= MCP4651_MAX_WIPER_INDEX; k++) {
        printf("%d, %f, %f, %f, %f, %f, %f\n", k, cal->DCPos_calibration[k].v_wb, cal->DCNeg_calibration[k].v_wb, cal->GainUpper_calibration[k].r_wb, cal->GainLower_calibration[k].r_wb, cal->RefUpper_calibration[k].v_wb, cal->RefLower_calibration[k].v_wb);
        fflush(stdout);
        sleep_us(500);
    }
}