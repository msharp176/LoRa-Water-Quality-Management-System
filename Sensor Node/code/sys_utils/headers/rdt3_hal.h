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

#ifndef RDT3_0_HAL_H
#define RDT3_0_HAL_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#include "lora.h"
#include "lwqms_pkt.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions 

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Enums 

typedef enum rdt3_0_ack_e {
    RDT3_0_ACK = 0,
    RDT3_0_NACK = -1,
    RDT3_0_ACK_ERR = -2,
    RDT3_0_ACK_BAD_ID = -3,
} rdt3_0_ack_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types 

typedef void * rdt_packet_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Declarations 

/**
 * @brief Sends a packet for the RDT 3.0 Transport Layer using Physical Hardware
 * 
 * @param pkt The packet to send
 * @param physical_context Object containing data about the hardware which will be sending the packet
 * 
 * @returns Operation Result Code
 */
bool rdt3_0_tx_hal(rdt_packet_t pkt, void* physical_context);

/**
 * @brief Receives a packet for the RDT3.0 Transport Layer using Physical Hardware
 * 
 * @param pkt Empty buffer to which the received packet will be written
 * @param physical_context Object containing data about the hardware which will be sending the packet
 * 
 * @returns Operation Result Code
 */
bool rdt3_0_rx_hal(rdt_packet_t pkt, void* physical_context);

/**
 * @brief Checks the ACK status of the received ACK packet to determine if the package was received without error.
 * 
 * @param ack_pkt The packet recieved containing the ACK/NACK status
 * @param sent_pkt The packet that was sent that the ACK packet corresponds to
 * @param phy_setup Hardware configuration data.
 * 
 * @returns ACK status
 */
rdt3_0_ack_t rdt3_0_process_ack_pkt_hal(rdt_packet_t ack_pkt, rdt_packet_t sent_pkt, void *phy_setup);

/**
 * @brief Checks the received packet for error and generates a corresponding ACK/NACK packet
 * 
 * @param received_pkt The packet which was received
 * @param ack_pkt Buffer to which the resulting ACK packet will be written.
 * @param phy_setup Hardware configuration data
 * 
 * @returns Packet generation status.
 */
rdt3_0_ack_t rdt3_0_process_data_packet_hal(rdt_packet_t received_pkt, rdt_packet_t ack_pkt, void *phy_setup);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


#endif /* RDT3_0_HAL_H */