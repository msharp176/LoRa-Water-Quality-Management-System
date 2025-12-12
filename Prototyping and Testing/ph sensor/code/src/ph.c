#include "ph.h"

// Calculate the model parameters k and b for the linear equation
static void get_model_kb(const ph_t* ph, float* k, float* b) {
    float x7 = (ph->mv_at_7 - 1500.0f) / 3.0f;
    float x4 = (ph->mv_at_4 - 1500.0f) / 3.0f;
    *k = (7.0f - 4.0f) / (x7 - x4);
    *b = 7.0f - (*k) * x7;
}

void ph_init_default(ph_t* ph) {
    ph->mv_at_7 = PH_DEFAULT_MV_AT_7;
    ph->mv_at_4 = PH_DEFAULT_MV_AT_4;
}

void ph_set_cal_pH7_mv(ph_t* ph, float mv) { 
    ph->mv_at_7 = mv; 
}

void ph_set_cal_pH4_mv(ph_t* ph, float mv) { 
    ph->mv_at_4 = mv; 
}

// Convert millivolts to pH value using the calibration data
// Uses a linear model: pH = k * (mv - 1500) / 3 + b
float ph_from_millivolts(const ph_t* ph, float mv) {
    float k, b;
    get_model_kb(ph, &k, &b);
    float x = (mv - 1500.0f) / 3.0f;
    return k * x + b;	// y = k*x + b
}
