/*************************************************************************************
 * 
 * @file rdt3.c
 * 
 * @brief Implementation of the Reliable Data Transfer 3.0 Transport Layer for the LoRa Water Quality Management System 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "rdt3.h"

// In this implementation, rdt_context_t aliases a packet of type lora_packet_t.

rdt3_0_result_codes_t rdt3_0_transmit(rdt_packet_t pkt, size_t pkt_obj_size, void *physical_layer_setup) {
    
    bool tx_ok = false;

    rdt3_0_result_codes_t exit_code = RDT3_0_RESULT_CODES_ERR;

    rdt_packet_t rx_pkt = malloc(pkt_obj_size); // sorry, not sure how to get around this!

    for (int k = 0; k < RDT_RETRIES; k++) {
        do {
            // 1. Transmit the packet
            if (!rdt3_0_tx_hal(pkt, physical_layer_setup)) break;
            
            // 2. Wait for an ACK from the receiver
            receive_start:
            printf("Waiting for an acknowledge from the receiver...\n");
            if (!rdt3_0_rx_hal(rx_pkt, physical_layer_setup)) break;
            
            // 3. If an ACK is not received before the timeout, or a NACK is received, then we need to repeat.
            rdt3_0_ack_t retval = rdt3_0_process_ack_pkt_hal(rx_pkt, pkt, physical_layer_setup);
            switch (retval) {
                case RDT3_0_ACK:
                    tx_ok = true;
                    break;
                case RDT3_0_ACK_BAD_ID:
                    goto receive_start; // This could be thrown by the packet either having a bad packet ID or being for the wrong device.
                case RDT3_0_ACK_ERR:
                    err_raise(ERR_RDT3_0, ERR_SEV_NONFATAL, "Failed to process ACK/NACK packet!", "rdt3_0_transmit");
                    break;
                case RDT3_0_NACK:
                    break;
            }

            free(rx_pkt);

            if (tx_ok) {
                exit_code = RDT3_0_RESULT_CODES_OK;
                break;
            }
            else if ((retval == RDT3_0_NACK) && (k == RDT_RETRIES - 1)) {
                exit_code =  RDT3_0_RESULT_CODES_NACK;
                break;
            }

        } while (0);

        if (tx_ok) {
            break;
        }
    }

    free(rx_pkt);

    return exit_code;
}

rdt3_0_result_codes_t rdt3_0_receive(rdt_packet_t pkt, size_t pkt_obj_size, void *physical_layer_setup) {

    bool rx_ok = false;

    rdt3_0_result_codes_t exit_code = RDT3_0_RESULT_CODES_ERR;

    rdt_packet_t ack_pkt = malloc(pkt_obj_size);

    for (int k = 0; k < RDT_RETRIES; k++) {

        do {
            begin_receive:
            // 1. Receive a packet
            if (!rdt3_0_rx_hal(pkt, physical_layer_setup)) break;
    
            // Verify that this device is the intended receiver
            
            // 2. Send ACK/NACK
            
            // Create the ACK/NACK packet
            rdt3_0_ack_t packet_status = rdt3_0_process_data_packet_hal(pkt, ack_pkt, physical_layer_setup);
            switch (packet_status) {
                case RDT3_0_ACK_BAD_ID:
                    goto begin_receive; // The packet is not intended for this receiver. Do not do anything
                default:
                    /**
                     * switch-case structure set up for future implementation that might require multiple other branches.
                     * For now, the options are just BAD_ID, ACK, and NACK, so the only case where we wouldn't send a packet
                     * is if the packet was not intended for his receiver.
                     */
                    break;
            }

            // Send the packet
            if (!rdt3_0_tx_hal(ack_pkt, physical_layer_setup)) break;

            rx_ok = true;
        } while (0);

        if (rx_ok) {
            exit_code = RDT3_0_RESULT_CODES_OK;
            break;
        }
    }

    free(ack_pkt);

    return exit_code;

}