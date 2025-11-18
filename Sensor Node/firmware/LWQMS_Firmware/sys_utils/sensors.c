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

}

/**
 * @brief Converts the RTD sensor voltage to a useable temperature
 * 
 * @param sensor_voltage: The ORIGINAL sensor voltage before the instrumentation amplifier
 * 
 * @returns Temperature Measurement, in degrees celsius
 */
static double convert_rtd_voltage_to_temp(double sensor_voltage) {

    // See section 2.4.11 of capstone report for formula derivation
    return (-RTD_A + sqrt(pow(RTD_A, 2) - ((4 * RTD_B) * (1 - ((47 * sensor_voltage) / (5 - sensor_voltage)))))) / (2 * RTD_B);   
}

/**
 * @brief Converts the pH sensor voltage to a useable pH measurement
 * 
 * @param sensor_voltage: The ORIGINAL sensor voltage before the instrumentation amplifier
 * 
 * @returns pH measurement
 */
static double convert_ph_voltage_to_ph(double sensor_voltage) {

}

bool acquire_data(sdia_context_t *sdia_context, sdia_potentiometer_full_calibration_t *sdia_cal, sensor_acquisition_settings_t *acquisition_settings, sensor_telemetry_t *telem_buf) {

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

    // Convert to corresponding measurements
    telem_buf->turbidity_NTU = convert_turb_voltage_to_turbidity(turb_voltage);
    telem_buf->temperature_C = convert_rtd_voltage_to_temp(rtd_voltage);
    telem_buf->pH = convert_ph_voltage_to_ph(pH_voltage);

    return true;
    
}