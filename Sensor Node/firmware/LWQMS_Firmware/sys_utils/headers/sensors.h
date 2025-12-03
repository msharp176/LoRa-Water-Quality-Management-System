/******************************************************************************************************************** 
*   
*   @file sensors.h
*
*   @brief Definitions & Declarations of Sensor Voltage Characteristics and Data Acquisition Phase Methods for the LoRa Water Quality Management System Sensor Node
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef SENSORS_H
#define SENSORS_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#include <math.h>
#include "software_defined_inst_amp.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions 

#define RTD_R0 100
#define RTD_A 3.9083e-3
#define RTD_B -5.775e-7
#define RTD_C -4.183e-12
#define RTD_V_ADJ (-0.003)

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types 

/**
 * @brief Telemetry Data as Produced by the Software-Defined Instrumentation Amplifier
 */
typedef struct sensor_telemetry_s {
    double turbidity_NTU;
    double temperature_C;
    double pH;
} sensor_telemetry_t;

typedef struct sensor_acquisition_settings_s {
    sdia_analog_characteristic_t analog_characteristic_turb;
    sdia_analog_characteristic_t analog_characteristic_temp;
    sdia_analog_characteristic_t analog_characteristic_pH;
} sensor_acquisition_settings_t;
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

/**
 * @brief Acquires sensor telemetry data from all three sensors on the Sensor Node
 * 
 * @param sdia_context: Software-Defined Instrumentation Amplifier Hardware Implementation Information
 * @param sdia_cal: Full Software-Defined Instrumentation Amplifier Calibration Data
 * @param acquisition_settings: Analog Characteristics for the Software-Defined Instrumentation Amplifier to use during the signal acquisition phase
 * @param telem_buf: Buffer to write final telemetry data, in the corresponding units.
 * 
 * @returns Operation Success/Failure
 */
bool sensors_acquire_data(sdia_context_t *sdia_context, sdia_potentiometer_full_calibration_t *sdia_cal, sensor_acquisition_settings_t *acquisition_settings, sensor_telemetry_t *telem_buf);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* SENSORS_H */