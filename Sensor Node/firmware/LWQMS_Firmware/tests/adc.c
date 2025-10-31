int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be connected
    wait_for_usb_console_connection_hal();

    sleep_ms(500);

    print_banner();

    // Initialize the gpio pins
    printf("Initializing hardware...");

    i2c_init_hal(&context_i2c_1);

    // Set to one-shot conversion mode, 16 bit resolution, and a PGA gain of 1.
    mcp3425_init(&context_adc_0, MCP3425_SPS_15_16BITS, MCP3425_PGA_1, false);

    bool configured_correctly = (!context_adc_0.continuous_conversion_mode_enabled) && (context_adc_0.sampling_rate == MCP3425_SPS_15_16BITS) && (context_adc_0.gain == MCP3425_PGA_1);

    if (!configured_correctly) {
        printf("Failed to configure ADC. Received one-shot, sampling, and gain values of: %d, %d, %d\n", context_adc_0.continuous_conversion_mode_enabled, context_adc_0.sampling_rate, context_adc_0.gain);
        while (1) {};   // Idle
    }
   
    printf("DONE\n");

    while (1) {
        
        printf("To take a reading, press 't'.\n");

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive

        double voltage = mcp3425_get_measurement(&context_adc_0);

        printf("Reading: %f\n\n", voltage);
    }

}