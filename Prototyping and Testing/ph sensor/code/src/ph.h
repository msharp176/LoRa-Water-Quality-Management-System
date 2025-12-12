#ifndef PH_H_           // prevents multiple inclusions
#define PH_H_

#ifdef __cplusplus      // allows C++ compatibility
extern "C" {
#endif

// Default calibration values
#define PH_DEFAULT_MV_AT_7  1500.0f     // 1500 mV at pH of 7
#define PH_DEFAULT_MV_AT_4  2032.44f    // 2032.44 mV at pH of 4

// Structure to hold pH calibration data
typedef struct {
    float mv_at_7;
    float mv_at_4;
} ph_t;

// set default calibration values
void  ph_init_default(ph_t* ph);

// Set the millivolts reading for pH 7 calibration
void  ph_set_cal_pH7_mv(ph_t* ph, float mv);

// Set the millivolts reading for pH 4 calibration
void  ph_set_cal_pH4_mv(ph_t* ph, float mv);

// Convert millivolts to pH value using the calibration data
float ph_from_millivolts(const ph_t* ph, float mv);

#ifdef __cplusplus
}
#endif

#endif
