int main()
{
    init_usb_console_hal();

    // Wait for the USB console to be opened on the host PC
    wait_for_usb_console_connection_hal();

    sleep_ms(100);

    print_banner();

    usb_console_write_hal("Initializing Hardware...");
    sx126x_initialize_hardware_context(&radio_0);

    //gpio_setup_hal(context_irq_txInit.pin, false);
    gpio_setup_hal(err_led, true);
    gpio_write_hal(err_led, false);

    gpio_irq_attach_hal(&context_irq_txInit);

    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio...");
    sx126x_radio_setup(&radio_0);    
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up interrupts...");
    sx126x_interrupt_setup(&radio_0);
    usb_console_write_hal("DONE\n");

    usb_console_write_hal("Setting up the radio for a transmit operation...");
    
    lora_init_tx(   &radio_0, 
                    &sx1262_14dBm_pa_params, 
                    &prototyping_mod_params,
                    14, 
                    SX126X_RAMP_200_US,
                    LWQMS_SYNC_WORD
                );

    usb_console_write_hal("DONE\n\n\n");

    gpio_write_hal(err_led, true);

    char * txBuf = "Four score and seven years ago our fathers brought forth on this continent a new nation.";

    while (true) {

        usb_console_write_hal("To transmit a packet, press 't'.\n");

        // Wait for input
        while ((usb_console_getchar_hal() | 0x20) != 't') {}  // t, case insensitive
        //while (!tx_go) {}
        //tx_go = false;

        usb_console_write_hal("Sending Packet...");

        // Transmit a packet
        lora_tx(&radio_0,
                &prototyping_irq_masks,
                &prototyping_pkt_params,
                txBuf,
                89
        );

        usb_console_write_hal("DONE\n");

        usb_console_write_hal("Waiting for interrupt...");
        while (!sx126x_check_for_interrupt()) {}
        usb_console_write_hal("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_TX_DONE) != 0) {
            usb_console_write_hal("TX Success!");
        }
        else if ((serviced_interrupts & SX126X_IRQ_TIMEOUT) != 0) {
            usb_console_write_hal("ERROR: TIMEOUT!");
        }
        else {
            usb_console_write_hal("Bad operation!");
        }

        usb_console_write_hal("\n\n");
    }    
}