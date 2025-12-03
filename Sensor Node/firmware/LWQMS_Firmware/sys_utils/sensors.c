/******************************************************************************************************************** 
*   
*   @file sensors.c
*
*   @brief Sensor Voltage Characteristics and Data Acquisition Phase for the LoRa Water Quality Management System Sensor Node
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "sensors.h"

/**
 * @brief Converts the turbidity sensor voltage to a useable turbidity measurement
 * 
 * @param sensor_voltage: The ORIGINAL sensor voltage before the instrumentation amplifier
 * 
 * @returns Turbidity Measurement, in NTU
 */
static double convert_turb_voltage_to_turbidity(double sensor_voltage) {

    // Turbidity sensor voltage cleanup - doing my best.
    //sensor_voltage += 0.76;
    if (sensor_voltage > 4.1188) {
        sensor_voltage = 4.1188;
    }
    printf("Sensor Voltage (adjusted), %f", sensor_voltage);
    // See SDIA Profile Excel File For Formula Characterization - Linear Regression Estimated from Testing Data
    double turb_est = -1462.9 * (sensor_voltage) + 6011.1;

    return turb_est < 0 ? 0 : turb_est;
}

/**
 * @brief Converts the RTD sensor voltage to a useable temperature
 * 
 * @param sensor_voltage: The ORIGINAL sensor voltage before the instrumentation amplifier
 * 
 * @returns Temperature Measurement, in degrees celsius
 */
static double convert_rtd_voltage_to_temp(double sensor_voltage) {

    // See excel sheet for conversion
    return (2515.7 * sensor_voltage) - 261.78;
}

/**
 * @brief Converts the pH sensor voltage to a useable pH measurement
 * 
 * @param sensor_voltage: The ORIGINAL sensor voltage before the instrumentation amplifier
 * 
 * @returns pH measurement
 */
static double convert_ph_voltage_to_ph(double sensor_voltage) {

    // See SDIA Profile Excel File for formula characterization - linear regression from testing data
    return (7.2492 * sensor_voltage) - 11.198;
}

bool sensors_acquire_data(sdia_context_t *sdia_context, sdia_potentiometer_full_calibration_t *sdia_cal, sensor_acquisition_settings_t *acquisition_settings, sensor_telemetry_t *telem_buf) {

    bool acquisition_ok = false;

    double turb_voltage, rtd_voltage, pH_voltage;

    // Get the raw voltage measurements from all 3 sensors.
    do {
        // Turbidity Measurement
        if (!sdia_acquire(sdia_context, sdia_cal, SDIA_SENSOR_TYPE_TURBIDITY, &(acquisition_settings->analog_characteristic_turb), &turb_voltage)) break;

        // Temperature Measurement
        if (!sdia_acquire(sdia_context, sdia_cal, SDIA_SENSOR_TYPE_TEMPERATURE, &(acquisition_settings->analog_characteristic_temp), &rtd_voltage)) break;

        // pH Measurement
        if (!sdia_acquire(sdia_context, sdia_cal, SDIA_SENSOR_TYPE_PH, &(acquisition_settings->analog_characteristic_pH), &pH_voltage)) break;

        acquisition_ok = true;

    } while (0);

    if (!acquisition_ok) return false;

    printf("Turbidity Voltage: %f, pH Voltage: %f, RTD Voltage (no adjustment): %f\n", turb_voltage, pH_voltage, rtd_voltage);

    // Convert to corresponding measurements
    telem_buf->turbidity_NTU = convert_turb_voltage_to_turbidity(turb_voltage);
    telem_buf->temperature_C = convert_rtd_voltage_to_temp(rtd_voltage);
    telem_buf->pH = convert_ph_voltage_to_ph(pH_voltage);

    return true;
    
}