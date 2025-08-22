#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "ph.h"


#define PH_ADC_GPIO         26      // Use GPIO26 for ADC0
#define PH_ADC_INPUT         0      // ADC channel 0
#define ADC_VREF_MV    3300.0f      // Pico ADC reference is ~3.3V
#define BAUD            115200
#define PRINT_EVERY_MS    1000      // print every 1 second
#define SAMPLES_AVG         32      // average this many samples to smooth noise

// Read ADC many times and return the average in millivolts
static float read_adc_millivolts(void) {
    uint32_t sum = 0;
    for (int i = 0; i < SAMPLES_AVG; ++i) {
        sum += adc_read();               
        sleep_us(200);                   
    }
    float raw_avg = (float)sum / (float)SAMPLES_AVG; // average the raw ADC values
    float mv = (raw_avg * ADC_VREF_MV) / 4095.0f;    // convert to millivolts
    return mv;
}


int main() {
    stdio_init_all();
    sleep_ms(1000); 

    // Welcome message
    printf("\n=== Pico pH Reader ===\n");
    printf("Commands:\n");
    printf("  help  : show commands\n");
    printf("  c7    : set pH7 calibration using the CURRENT voltage\n");
    printf("  c4    : set pH4 calibration using the CURRENT voltage\n");
    printf("  show  : print current calibration values\n");
    printf("Tip: Place probe in pH 7 buffer, wait stable, type 'c7'. Rinse, pH 4, 'c4'.\n\n");

    // ph structure to hold calibration
    ph_t ph;
    // Init library with defaults so it works before calibration
    ph_init_default(&ph);

    // ADC setup
    adc_init();
    adc_gpio_init(PH_ADC_GPIO);
    adc_select_input(PH_ADC_INPUT);

    // Input buffer
    char line[32];
    int pos = 0;

    // Stores timestamp for next print (current time + 1 second)
    absolute_time_t next_print = make_timeout_time_ms(PRINT_EVERY_MS);

    while (true) {
        // ---- Non-blocking input ----
        int ch = getchar_timeout_us(0);     // grabs a character if available
        
        if (ch >= 0) {                      // errors are negative, so >= 0 means valid char
            if (ch == '\r' || ch == '\n') { // Enter key pressed
                if (pos > 0) {
                    line[pos] = '\0';       // add null terminator
                    
                    if (strcmp(line, "help") == 0) {
                        printf("Commands: help | c7 | c4 | show\n");
                    } 
                    else if (strcmp(line, "c7") == 0) {
                        float mv = read_adc_millivolts();
                        ph_set_cal_pH7_mv(&ph, mv);
                        printf("[OK] Set pH7 at %.2f mV.\n", mv);
                    } 
                    else if (strcmp(line, "c4") == 0) {
                        float mv = read_adc_millivolts();
                        ph_set_cal_pH4_mv(&ph, mv);
                        printf("[OK] Set pH4 at %.2f mV.\n", mv);
                    } 
                    else if (strcmp(line, "show") == 0) {
                        printf("Calibration: pH7=%.2f mV, pH4=%.2f mV\n", ph.mv_at_7, ph.mv_at_4);
                    } 
                    else {
                        printf("Unknown. Try: help\n");
                    }
                    pos = 0;
                }
            } 
            else if (pos + 1 < (int)sizeof(line)) { 
                line[pos] = (char)ch; 
                pos++;
            }
        }

        // ---- Periodic reading & print ----
        if (absolute_time_diff_us(get_absolute_time(), next_print) <= 0) {  // checks if 1 second passed 
            next_print = delayed_by_ms(next_print, PRINT_EVERY_MS);         // next_print = current time + 1 second

            float mv = read_adc_millivolts();           // read and average voltage
            float pH = ph_from_millivolts(&ph, mv);     // convert voltage to pH 
            printf("V=%.2f mV  pH=%.2f\n", mv, pH);
        }
        
        sleep_ms(10);  
    }
    return 0;
}
