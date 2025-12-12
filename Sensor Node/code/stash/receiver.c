/******************************************************************************************************************** 
*   
*   @file main.c
*
*   @brief Main Driver file for the LoRa Water Quality Management System Dedicated Receiver Firmware
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "main.h"

void print_banner(void) {
    printf("\n\n-- LoRa Water Quality Management System Receiver --\n");
    printf("Version 0.1, compiled %s, %s\n\n", __DATE__, __TIME__);
}

int receiver_setup(void) {
    /**
     * 1. Initialize GPIO
     * 2. Initialize SPI Bus
     * 3. Initialize Radio
     */

    init_usb_console_hal();

    wait_for_usb_console_connection_hal();

    print_banner();

    bool init_ok = false;
    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            uint8_t output_pins[] = {STATUS_LED, TX_LED, RX_LED, ERR_LED};
            
            // Initialize the gpio pins and assert LOW on all.
            for (int k = 0; k < sizeof(output_pins); k++) {
                gpio_setup_hal(output_pins[k], true);
                gpio_write_hal(output_pins[k], GPIO_LOW);
            }
        
            // Initialize all SPI peripherals.
            if (spi_init_hal(&context_spi_0) < 0) break;
            sx126x_initialize_hardware_context(&context_radio_0);
            sx126x_radio_setup(&context_radio_0);
            sx126x_interrupt_setup(&context_radio_0);
            
            init_ok = true;
    
        } while (0);
    }

    return init_ok ? 0 : -1;

}

// Dedicated Receiver Info
node_config_t rx_config = {
  .ID = 2,
  .sync_word = 0x42,
  .latitude = 40.2732,
  .longitude = 76.8867
};

lora_setup_t dedicated_receiver_setup = {
    .hw = &context_radio_0,
    .mod_setting = &prototyping_mod_params,
    .operation_timeout_ms = SX126X_RX_CONTINUOUS,
    .pa_setting = &sx1262_22dBm_pa_params,
    .pkt_setting = &prototyping_pkt_params,
    .ramp_time = SX126X_RAMP_200_US,
    .rx_interrupt_setting = &prototyping_irq_masks,
    .tx_interrupt_setting = &prototyping_irq_masks,
    .txPower = 22,
    .node_config = &rx_config,
};

int main(void) {

    if (receiver_setup() < 0) {
        err_raise(ERR_POST_FAIL, ERR_SEV_FATAL, "Failed to initialize!", "main");
    }

    printf("READY\n");

    while (1) {
        printf("RX MODE ENABLED\n");

        lora_pkt_t rxPacket;
        rdt3_0_receive((rdt_packet_t)(&rxPacket), sizeof(lwqms_pkt_t), &dedicated_receiver_setup);
        
        lwqms_pkt_t processed_packet;
        lwqms_pkt_decode(rxPacket.buf, rxPacket.len, &processed_packet);

        switch (processed_packet.packet_type) {
            case LWQMS_PACKET_TYPE_MESSAGE:
                printf("LWQMS_MSG[%d]: %s END", processed_packet.src_id, processed_packet.payload.message);
                break;
            case LWQMS_PACKET_TYPE_TELEMETRY:
                printf("LWQMS_PLD[%d]: %f %f %f END", processed_packet.src_id, processed_packet.payload.telemetry.turbidity_measurement, processed_packet.payload.telemetry.temperature_measurement, processed_packet.payload.telemetry.pH_measurement);
                break;
        }
        
        printf("\n\n");
    }
}