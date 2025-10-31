/*************************************************************************************
 * 
 * @file rdt3_hal.h
 * 
 * @brief Hardware Abstraction Layer for the Reliable Data Transfer 3.0 Transport Layer Implementation 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "rdt3_hal.h"

bool rdt3_0_tx_hal(rdt_packet_t pkt, void* physical_context) {
    // Cast the incoming generic physical context to an application-specific type. In this case, we will use lora_setup_t.
    lora_setup_t *setup = (lora_setup_t *)physical_context;

    // Cast the incoming generic packet data to an application-specific type. In this case, this is lora_pkt_t.
    lora_pkt_t *lora_pkt = (lora_pkt_t*)pkt;

    // Repetition is built into these functions, so we will just try to execute each one once.
    bool tx_ok = false;

    gpio_write_hal(TX_LED, GPIO_HIGH);

    do {
        // Initialize a transmit operation
        printf("\n\n\nInitializing Transmit Operation...");
        if (!lora_init_tx(setup->hw, setup->pa_setting, setup->mod_setting, setup->txPower, setup->ramp_time, ((node_config_t *)(setup->node_config))->sync_word)) break;
        printf("DONE\n");
    
        // Ensure the TX Done interrupt is set.
        setup->tx_interrupt_setting->dio1_mask |= SX126X_IRQ_TX_DONE;

        // Send the packet
        printf("Sending the packet...");
        if (!lora_tx(setup->hw, setup->tx_interrupt_setting, setup->pkt_setting, lora_pkt->buf, lora_pkt->len)) break;
        printf("DONE\n");

        // Wait for the TX Done Interrupt
        absolute_time_t tx_done_timeout_time = make_timeout_time_ms(setup->operation_timeout_ms);
        bool interrupt_found = false;
        
        printf("Waiting for interrupt...");
        while(get_absolute_time() < tx_done_timeout_time) {
            if (sx126x_check_for_interrupt())  {
                interrupt_found = true;
                break;
            }
        }

        if (!interrupt_found) {
            printf("FAIL\n");
            break;
        }
        printf("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_TX_DONE) > 0) {
            printf("TX Operation Successful!\n\n\n");
            tx_ok = true;
        }
        else {
            char err_msg[0x100];
            snprintf(err_msg, 0x100, "TX Error: IRQ Mask = %u", serviced_interrupts);
            err_raise(ERR_LORA_TIMEOUT, ERR_SEV_NONFATAL, err_msg, "rdt3_0_tx_hal");
        }

    } while (0);

    gpio_write_hal(TX_LED, false);
    return tx_ok;

}

bool rdt3_0_rx_hal(rdt_packet_t pkt, void* physical_context) {
    // Cast the incoming generic physical context to an application-specific type. In this case, we will use lora_setup_t.
    lora_setup_t *setup = (lora_setup_t *)physical_context;

    // Cast the incoming generic packet data to an application-specific type. In this case, this is lora_pkt_t.
    lora_pkt_t *lora_pkt = (lora_pkt_t*)pkt;

    bool rx_ok = false;

    gpio_write_hal(RX_LED, GPIO_HIGH);

    do {
        // Initialize a receive operation
        printf("Initializing a receive operation...");
        if (!lora_init_rx(setup->hw, setup->mod_setting, setup->pkt_setting)) break;
        printf("DONE\n");

        // Ensure the RX Done interrupt is set
        setup->rx_interrupt_setting->dio1_mask |= SX126X_IRQ_RX_DONE;
        
        // Set the radio in receive mode
        printf("Putting the radio in receive mode...");
        if (!lora_rx(setup->hw, setup->rx_interrupt_setting, ((node_config_t *)(setup->node_config))->sync_word, setup->operation_timeout_ms)) break;
        printf("DONE\n");

        // Wait for the RX Done Interrupt
        absolute_time_t rx_done_timeout_time = make_timeout_time_ms(setup->operation_timeout_ms);
        bool interrupt_found = false;

        printf("Waiting for interrupt...");
        while (get_absolute_time() < rx_done_timeout_time) {
            if (sx126x_check_for_interrupt()) {
                interrupt_found = true;
                break;
            }
        }

        if (!interrupt_found) {
            printf("FAIL\n");
            break;
        }

        printf("DONE\n");

        sx126x_irq_mask_t serviced_interrupts = sx126x_service_interrupts();

        if ((serviced_interrupts & SX126X_IRQ_RX_DONE) > 0) {
            printf("RX Operation Successful!\n");
            rx_ok = true;

            // Retrieve the packet data from the radio
            if (!lora_get_rx_data(setup->hw, lora_pkt->buf, &(lora_pkt->len))) break;  

            printf("Received Packet: \n");
            hexdump(lora_pkt->buf, lora_pkt->len, 0x00);
            printf("\n\n");
        }
        else {
            char err_msg[0x100];
            snprintf(err_msg, 0x100, "RX Error: IRQ Mask = %u", serviced_interrupts);
            err_raise(ERR_LORA_FAIL, ERR_SEV_NONFATAL, err_msg, "rdt3_0_rx_hal");
        }

      

    } while (0);

    gpio_write_hal(RX_LED, GPIO_LOW);

    return rx_ok;
}

rdt3_0_ack_t rdt3_0_process_ack_pkt_hal(rdt_packet_t ack_pkt, rdt_packet_t sent_pkt, void *phy_setup) {
    lora_setup_t *lora_setup = (lora_setup_t*)phy_setup;
    lora_pkt_t * lora_ack_pkt = (lora_pkt_t*)ack_pkt;
    uint16_t packet_ID;
    lwqms_pkt_t processed_ack_pkt;
    lwqms_pkt_decode(lora_ack_pkt->buf, lora_ack_pkt->len, &processed_ack_pkt);

    // Check the destination ID of the packet
    if (processed_ack_pkt.dest_id != (((node_config_t *)(lora_setup->node_config))->ID)) return RDT3_0_ACK_BAD_ID;

    lwqms_pkt_ack_status_t ack_status = lwqms_pkt_check_ack(&processed_ack_pkt, &packet_ID);
    
    if (ack_status == LWQMS_PKT_ACK_STATUS_NONE) return RDT3_0_ACK_ERR;

    // Check the packet ID was correct
    lwqms_pkt_t sent_lora_packet;
    
    if (!lwqms_pkt_decode(((lora_pkt_t *)sent_pkt)->buf, ((lora_pkt_t *)sent_pkt)->len, &sent_lora_packet)) return RDT3_0_ACK_ERR;

    if (packet_ID != sent_lora_packet.pkt_id) return RDT3_0_ACK_BAD_ID;

    // Once everything is validated, check the ack/nack status.
    return ack_status == LWQMS_PKT_ACK_STATUS_ACK ? RDT3_0_ACK : RDT3_0_NACK;
}

rdt3_0_ack_t rdt3_0_process_data_packet_hal(rdt_packet_t received_pkt, rdt_packet_t ack_pkt, void *phy_setup) {
    
    // For now, this will just return an ACK... implement error checking at a later date!

    lora_setup_t * lora_setup = (lora_setup_t *)phy_setup;

    // Decode the incoming packet and extract the packet ID
    lora_pkt_t * raw_received_pkt = (lora_pkt_t *)received_pkt;
    lora_pkt_t * raw_pkt_out = (lora_pkt_t *)ack_pkt;
    raw_pkt_out->len = LWQMS_PKT_LEN_MAX;

    lwqms_pkt_t processed_incoming_packet;
    lwqms_pkt_t outgoing_ack_pkt;

    lwqms_pkt_decode(raw_received_pkt->buf, raw_received_pkt->len, &processed_incoming_packet);

    // Check the destination ID
    if (processed_incoming_packet.dest_id != (((node_config_t *)(lora_setup->node_config))->ID)) return RDT3_0_ACK_BAD_ID;

    lwqms_generate_ack_packet(&processed_incoming_packet, LWQMS_PKT_ACK_STATUS_ACK, &outgoing_ack_pkt);
    
    lwqms_pkt_encode(&outgoing_ack_pkt, raw_pkt_out->buf, LWQMS_PKT_LEN_MAX);

    return RDT3_0_ACK;
}