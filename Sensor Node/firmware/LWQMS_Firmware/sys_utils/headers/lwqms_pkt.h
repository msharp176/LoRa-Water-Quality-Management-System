/*************************************************************************************
 * 
 * @file lwqms_pkt.h
 * 
 * @brief Packet Encoding/Decoding Operations for the LoRa Water Quality Management System 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#ifndef LWQMS_PKT_H
#define LWQMS_PKT_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#include <string.h>
#include <stdio.h>
#include <pico/stdlib.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Definitions

#define LWQMS_PKT_LEN_MAX 19    // Works out to 370ms of on air time.

#define ACK_INDICATOR "ACK_"
#define NACK_INDICATOR "NACK"

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types 

/**
 * @brief Different types of packets that the sensor node can send: telemetry or a message
 */
typedef enum lwqms_packet_types_e {
    LWQMS_PACKET_TYPE_TELEMETRY = 0,
    LWQMS_PACKET_TYPE_MESSAGE = 1
} lwqms_packet_types_t;

/**
 * @brief Structure to encode the three measured sensor values as floating point numbers.
 * Enables easy collapse to a uint8_t array for transmission.
 */
typedef struct lwqms_telemetry_s {
    float turbidity_measurement;
    float temperature_measurement;
    float pH_measurement;
} lwqms_telemetry_t;

/**
 * @brief Defines the ultimate payload contained in the packet. By using a union type, we can represent both
 * payload types using the same C type and same memory locations.
 */
typedef union lwqms_pkt_payload_u {
    lwqms_telemetry_t telemetry;
    char message[sizeof(lwqms_telemetry_t)];
} lwqms_pkt_payload_t;

// The packet will occupy 19 bytes, and the payload will either be a 12 byte telemetry data payload, or a 12 byte string message!!

/**
 * Packet Structure:
 * 
 * 2 byte packet ID
 * 2 byte destination ID
 * 2 byte source ID
 * 1 Byte Packet Type (Telemetry/Message)
 * 4 byte raw float x 3 (turb, temp, pH)
 * 
 * = 19 bytes total
 */

typedef struct lwqms_pkt_s {
    uint16_t pkt_id;
    uint16_t dest_id;
    uint16_t src_id;
    lwqms_pkt_payload_t payload;
    uint8_t packet_type;   // Will be explicitly cast to uint8_t to ensure we have only 1 byte taken up here
} lwqms_pkt_t; 

typedef enum lwqms_pkt_ack_status_e {
    LWQMS_PKT_ACK_STATUS_ACK = 0,
    LWQMS_PKT_ACK_STATUS_NACK = -1,
    LWQMS_PKT_ACK_STATUS_NONE = -2
} lwqms_pkt_ack_status_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Declarations

/**
 * @brief Uses the given structure to create a byte array to transmit over the radio
 * 
 * @param pkt_data Packet data structure containing information to send over the radio
 * @param buf Buffer to which the converted packet will be stored
 * @param buflen Sanity check to ensure the destination buffer is appropriately sized (>= LWQMS_PKT_LEN_MAX)
 * 
 * @returns Operation Status
 */
bool lwqms_pkt_encode(lwqms_pkt_t *pkt_data, uint8_t *buf, size_t buflen);

/**
 * @brief Takes a raw input buffer and writes it to an organized data structure
 * 
 * @param buf: Buffer containing raw packet data
 * @param buflen: Sanity check to ensure the incoming buffer is appropriately sized (>= LWQMS_PKT_LEN_MAX)
 * @param pkt_out: Output data structure containing 
 */
bool lwqms_pkt_decode(uint8_t *buf, size_t buflen, lwqms_pkt_t *pkt_out);

/**
 * @brief Checks if the packet received is an ACK or NACK
 * 
 * @param pkt Packet to check
 * @param packet_ID Buffer to write the ID of the packet for the NACK/ACK
 * 
 * @returns Packet ACK status
 */
lwqms_pkt_ack_status_t lwqms_pkt_check_ack(lwqms_pkt_t *pkt, uint16_t *packet_ID);

/**
 * @brief Produces an ACK/NACK packet for the given packet ID
 * 
 * @param packet_ID The packet to send the ACK/NACK status for
 * @param ack_status The ACK/NACK status of the packet
 * @param pkt Buffer to write the output packet
 * 
 * @returns True for a successful packet creation.
 */
bool lwqms_generate_ack_packet(lwqms_pkt_t *pkt_to_ack, lwqms_pkt_ack_status_t ack_status, lwqms_pkt_t *pkt);

/**
 * @brief Displays the contents of a packet.
 * 
 * @param pkt: The packet to display
 * 
 * @returns None
 */
void lwqms_packet_display(lwqms_pkt_t *pkt);
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------


#endif  /* LWQMS_PKT_H */