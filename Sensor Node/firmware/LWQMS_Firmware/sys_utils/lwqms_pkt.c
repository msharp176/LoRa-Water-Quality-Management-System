/*************************************************************************************
 * 
 * @file lwqms_pkt.c
 * 
 * @brief Packet Encoding/Decoding Operations for the LoRa Water Quality Management System 
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "lwqms_pkt.h"
#include "hal.h"

/**
 * Use macros to keep byte-important copies portable.
 */

#define COPY_FROM_BUF(dst_struct, field, buf, offset) \
    memcpy(&(dst_struct)->field, (buf) + (offset), sizeof((dst_struct)->field)); \
    (offset) += sizeof((dst_struct)->field)

#define COPY_TO_BUF(buf, offset, src_struct, field) \
    memcpy((buf) + (offset), &(src_struct)->field, sizeof((src_struct)->field)); \
    (offset) += sizeof((src_struct)->field)


bool lwqms_pkt_encode(lwqms_pkt_t *pkt_in, uint8_t *buf, size_t buflen) {
    if (buflen < LWQMS_PKT_LEN_MAX) return false;
    
    // Fix alignment issue that was cutting off our data --> lwqms_pkt_t structure contains 19 bytes of information,
    // but was aligned by the compiler to use 24 bytes...

    /**
     * typedef struct lwqms_pkt_s {
        uint16_t pkt_id;
        uint16_t dest_id;
        uint16_t src_id;
        lwqms_pkt_payload_t payload;
        uint8_t packet_type;   // Will be explicitly cast to uint8_t to ensure we have only 1 byte taken up here
    } lwqms_pkt_t; 
    */

    uint offset;

    COPY_TO_BUF(buf, offset, pkt_in, pkt_id);
    COPY_TO_BUF(buf, offset, pkt_in, dest_id);
    COPY_TO_BUF(buf, offset, pkt_in, src_id);
    COPY_TO_BUF(buf, offset, pkt_in, payload);
    COPY_TO_BUF(buf, offset, pkt_in, packet_type);

    return true;
}

bool lwqms_pkt_decode(uint8_t *buf, size_t buflen, lwqms_pkt_t *pkt_out) {
    if (buflen < LWQMS_PKT_LEN_MAX) return false;

    uint offset = 0;

    COPY_FROM_BUF(pkt_out, pkt_id, buf, offset);
    COPY_FROM_BUF(pkt_out, dest_id, buf, offset);
    COPY_FROM_BUF(pkt_out, src_id, buf, offset);
    COPY_FROM_BUF(pkt_out, payload, buf, offset);
    COPY_FROM_BUF(pkt_out, packet_type, buf, offset);

    return true;
}

lwqms_pkt_ack_status_t lwqms_pkt_check_ack(lwqms_pkt_t *pkt, uint16_t* packet_ID) {
    if (pkt->packet_type != LWQMS_PACKET_TYPE_MESSAGE) return LWQMS_PKT_ACK_STATUS_NONE;

    // Extract the corresponding packet ID
    memcpy(packet_ID, pkt->payload.message + 4, 2);     

    if (strncmp((pkt->payload.message), ACK_INDICATOR, 4) == 0) {
        return LWQMS_PKT_ACK_STATUS_ACK;
    }
    else if (strncmp(pkt->payload.message, NACK_INDICATOR, 4) == 0) {
        return LWQMS_PKT_ACK_STATUS_NACK;
    }
    else {
        return LWQMS_PKT_ACK_STATUS_NONE;
    }

}

bool lwqms_generate_ack_packet(lwqms_pkt_t *packet_to_ack, lwqms_pkt_ack_status_t ack_status, lwqms_pkt_t *pkt) {
    memset(pkt->payload.message, 0x00, sizeof(lwqms_pkt_payload_t));

    memcpy(pkt->payload.message, ack_status == LWQMS_PKT_ACK_STATUS_ACK ? ACK_INDICATOR : NACK_INDICATOR, 4);
    memcpy(pkt->payload.message + 4, &(packet_to_ack->pkt_id), 2);

    pkt->pkt_id = packet_to_ack->pkt_id;
    pkt->src_id = packet_to_ack->dest_id;
    pkt->dest_id = packet_to_ack->src_id;
    pkt->packet_type = LWQMS_PACKET_TYPE_MESSAGE;

    return true;
}

extern void hexdump(const uint8_t *data, size_t length, size_t start_offset);

void lwqms_packet_display(lwqms_pkt_t *pkt) {
    printf("-->Packet ID: %d\n", pkt->pkt_id);
    printf("-->Destination ID: %d\n", pkt->dest_id);
    printf("-->Source ID: %d\n", pkt->src_id);
    printf("-->Packet Type: %s\n", pkt->packet_type == LWQMS_PACKET_TYPE_TELEMETRY ? "Telemetry" : "Message");
    
    printf("Payload:\n");
    hexdump(pkt->payload.message, sizeof(lwqms_pkt_payload_t), 0x00);
}