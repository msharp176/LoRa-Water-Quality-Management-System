/******************************************************************************************************************** 
*   
*   @file software_defined_inst_amp.c
*
*   @brief Custom Types & Interfacing Function Declarations for the Software-Defined Instrumentation Amplifier for the LoRa Water Quality Management System
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef SOFTWARE_DEFINED_INST_AMP_H
#define SOFTWARE_DEFINED_INST_AMP_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#include "mcp4651.h"    // Digipot
#include "tmux1309.h"   // mux
#include "mcp3425.h"    // ADC

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Defines

#define DC_OFFSET_POT_INPUT_VOLTAGE 5
#define OUTPUT_REFERENCE_POT_INPUT_VOLTAGE 5
#define INST_AMP_R0 10000

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

typedef enum sdia_sensor_types_s {
    SDIA_SENSOR_TYPE_TURBIDITY = 0,
    SDIA_SENSOR_TYPE_PH = 1,
    SDIA_SENSOR_TYPE_TEMPERATURE = 2,
    SDIA_SENSOR_TYPE_EXTRA = 3,
} sdia_sensor_types_t;

// Raw `uint16_t` wiper settings to be written to the digital potentiometers directly
typedef struct sdia_wiper_settings_s {
    uint16_t dc_pos_wiper_setting;
    uint16_t dc_neg_wiper_setting;
    uint16_t gain_wiper_a_setting;
    uint16_t gain_wiper_b_setting;
    uint16_t ref_out_wiper_a_setting;
    uint16_t ref_out_wiper_b_setting;
} sdia_wiper_settings_t;

// Gain and DC offset values describing the analog behavior of the Software-Defined Instrumentation Amplifier
typedef struct sdia_analog_setting_s {
    double gain;
    double dc_offset_pos;
    double dc_offset_neg;
    double output_reference_offset;
} sdia_analog_characteristic_t;

/**
 * @brief Calibration data for a single calibration point for a Digital Potentiometer in the Software-Defined Instrumentation amplifier.
 */
typedef union sdia_potentiometer_cal_data_u {
    double r_wb;        // Resistance from the wiper position to the B terminal of the pot.
    double v_wb;        // Voltage output between the wiper and the B terminal of the pot.
} sdia_potentiometer_cal_data_t;

typedef enum sdia_potentiometer_cal_data_types_e {
    RESISTANCE = 0,
    VOLTAGE = 1
} sdia_potentiometer_cal_data_types_t;

/**
 * @brief Complete Calibration Data for Each Digital Potentiometer on the Software-Defined Instrumentation Amplifier.
 */
typedef struct sdia_potentiometer_full_calibration_s {
    sdia_potentiometer_cal_data_t DCPos_calibration[MCP4651_MAX_WIPER_INDEX + 1];
    sdia_potentiometer_cal_data_t DCNeg_calibration[MCP4651_MAX_WIPER_INDEX + 1];
    sdia_potentiometer_cal_data_t GainUpper_calibration[MCP4651_MAX_WIPER_INDEX + 1];
    sdia_potentiometer_cal_data_t GainLower_calibration[MCP4651_MAX_WIPER_INDEX + 1];
    sdia_potentiometer_cal_data_t RefUpper_calibration[MCP4651_MAX_WIPER_INDEX + 1];
    sdia_potentiometer_cal_data_t RefLower_calibration[MCP4651_MAX_WIPER_INDEX + 1];
} sdia_potentiometer_full_calibration_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Declarations 

/**
 * @brief Applies the given wiper setting to the software defined instrumentation amplifier
 * 
 * @param context: Hardware Implementation Information
 * @param setting: The wiper settings to be written
 * 
 * @returns Operation status
 */
bool sdia_apply_wiper_setting(sdia_context_t *context, sdia_wiper_settings_t *setting);

/**
 * @brief Converts the provided wiper setting to an analog equivalent, with gain and DC offsets
 * 
 * @param context: Hardware Implenetation Information
 * @param cal: Software-Defined Instrumentation Amplifier Calibration Data
 * @param setting_in: The raw wiper setting to be converted
 * @param analog_equivalent: Buffer to store the resulting equivalent analog gain and offset settings.
 * 
 * @returns Conversion operation status
 */
bool sdia_convert_wiper_setting(sdia_context_t *context, 
    sdia_potentiometer_full_calibration_t *cal, sdia_wiper_settings_t *setting_in, sdia_analog_characteristic_t *analog_characteristic);

/**
 * @brief Reads the raw input voltage to the ADC
 * 
 * @param context: Hardware Implementation Information
 * @param voltage: Buffer to store the voltage reading
 * 
 * @returns Operation status 
 */
bool sdia_read_raw(sdia_context_t *context, double *voltage);

/**
 * @brief Reads the raw voltage from the selected sensor input using the given wiper setting, then calculates the original input voltage using the given wiper configuration
 * 
 * @param context: Hardware Implementation Information
 * @param cal: Software-Defined Instrumentation Amplifier Calibration Data
 * @param sensor_input: The sensor to read the voltage from
 * @param analog_setting: The analog setting to apply to the software-defined instrumentation amplifier before reading back the voltage
 * @param reading: Buffer to store the final adjusted reading.
 * 
 * @returns Operation status
 */
bool sdia_acquire(sdia_context_t *context, sdia_potentiometer_full_calibration_t *cal, sdia_sensor_types_t sensor_input, sdia_analog_characteristic_t *analog_setting, double *reading);

/**
 * @brief Prints the wiper setting to the console.
 * 
 * @param setting: Setting to print
 * 
 * @returns None
 */
void sdia_print_wiper_setting(sdia_wiper_settings_t *setting);

/**
 * @brief Prints the analog characteristic to the console.
 * 
 * @param characteristic: Analog Characteristic to Display
 * 
 * @returns None
 */
void sdia_print_analog_characteristic(sdia_analog_characteristic_t *characteristic);

/**
 * @brief Uses the provided raw input voltage and wiper settings to calculate the input voltage to the Software-Defined Instrumentation Amplifier
 * 
 * @param raw_voltage: The raw input voltage reported by the ADC
 * @param analog_characteristic: The current analog characteristic of the amplifier
 * 
 * @returns the calculated input voltage to the amplifier
 */
double sdia_process_raw_voltage(double raw_voltage, sdia_analog_characteristic_t *analog_characteristic);

/**
 * @brief Converts a desired analog characteristic of the software-defined instrumentation amplifier to physical wiper settings
 * 
 * @param context: Hardware Implementation Information
 * @param desired_analog_characteristic: The target analog characteristic of the amplifier
 * @param cal: Software-Defined Instrumentation Amplifier Calibration Data
 * @param setting: Buffer to hold the wiper setting corresponding to the target analog characteristic
 * @param actual_analog_characteristic: The actual analog characteristic corresponding to the wiper selections made by this method
 * 
 * @returns True for successful conversion, false for error.
 */
bool sdia_get_wiper_setting_from_analog_characteristic( sdia_context_t *context, sdia_analog_characteristic_t *desired_analog_characteristic, sdia_potentiometer_full_calibration_t *cal, sdia_wiper_settings_t *setting, sdia_analog_characteristic_t *actual_analog_characteristic);

/**
 * @brief Performs a full calibration of the software-defined instrumentation amplifier
 * 
 * @param context: Hardware Implementation Information
 * @param full_calibration_buffer: Buffer to store the completed calibration data
 * 
 * @returns Operation Success Status
 */
bool sdia_calibrate(sdia_context_t *context, sdia_potentiometer_full_calibration_t *full_calibration_buffer);

/**
 * @brief Prints the full software-defined instrumentation amplifier calibration data to the console in a CSV-ready format.
 * 
 * @param cal: Calibration data to display
 * 
 * @returns None
 */
void sdia_print_calibration(sdia_potentiometer_full_calibration_t *cal);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* SOFTWARE_DEFINED_INST_AMP_H */