/******************************************************************************************************************** 
*   
*   @file lora.c
*
*   @brief LoRa physical layer communications implementation using Semtech SX126X mfg driver and custom-designed raspberry pi pico HAL
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "main.h"
#include "sx126x.h"
#include "lora.h"
#include "errs.h"
#include "sx126x_hal.h"

#pragma region Helper Methods

void set_lora_ldro_val(sx126x_mod_params_lora_t* modulation_params) {
    
    // Temporary function definition to select the value of LDRO. Ideally, the LoRa symbol time should be calculated
    // and compared to the threshold time of 16.38 ms. If greater, set this bit to one.

    // TODO: Perform LoRa symbol time calculation

    uint8_t ldro;

    switch (modulation_params->sf) {
        case SX126X_LORA_SF11:
            ldro = (modulation_params->bw == SX126X_LORA_BW_125) ? LORA_LDRO_ON : LORA_LDRO_OFF;
            break;
        case SX126X_LORA_SF12:
            ldro = (modulation_params->bw == SX126X_LORA_BW_125 || modulation_params->bw == SX126X_LORA_BW_250) ? LORA_LDRO_ON : LORA_LDRO_OFF;
        default:
            ldro = LORA_LDRO_OFF;
    };

    modulation_params->ldro = ldro;

}

#pragma endregion

#pragma region TX Init

bool lora_init_tx(  const sx126x_context_t* radio_context, 
                    sx126x_pa_cfg_params_t* power_amplifier_config,
                    int8_t  txPower,
                    sx126x_ramp_time_t ramp_time
) {
    /**
     * Per the datasheet, to setup the sx126x for TX mode:
     *  1. Put the chip into standby mode with SetStandby
     *  2. Define the protocol with SetPacketType
     *  3. Define the RF frequency with SetRfFrequency
     *  4. Define the power amplifier configuration with SetPaConfig
     *  5. Define output power and ramping time with SetTxParams
     *  6. Define the data payload storage base address using SetBufferBaseAddress
     */
    
    // Write all the configuration data
    for (int k = 0; k < SPI_RETRIES; k++) {
        
        bool init_ok = false;
        sx126x_pkt_type_t readback_pkt_type;
        
        do {    // The DO-WHILE(0) structure gives the program a block of code that can be exited upon error.

            // 1. Put the radio into XTAL Oscillator Standby mode
            if (sx126x_set_standby(radio_context, SX126X_STANDBY_CFG_XOSC) != SX126X_STATUS_OK)                             break; // The WaveShare LoRa module uses a 32MHz TXCO
            
            // 2. Configure the packet type to LoRa
            if (sx126x_set_pkt_type(radio_context, SX126X_PKT_TYPE_LORA) != SX126X_STATUS_OK)                               break; // Use LoRa packets
            
            // 3. Set the RF frequency of the radio to the legal band within the USA
            if (sx126x_set_rf_freq(radio_context, LORA_FREQ_NORTH_AMERICA) != SX126X_STATUS_OK)                             break; // Use the North American Standard LoRa Frequency (915 MHz)
            
            // 4. Configure the Power Amplifier
            if (sx126x_set_pa_cfg(radio_context, power_amplifier_config) != SX126X_STATUS_OK)                               break; // Configure to lowest power mode for now
            
            // 5. Configure the transmission parameters
            if (sx126x_set_tx_params(radio_context, txPower, ramp_time) != SX126X_STATUS_OK)                                break; // Set to use a 200 us Ramp Time
            
            // 6. Initialize the radio r/w buffers
            if (sx126x_set_buffer_base_address(radio_context, LORA_TX_BUF_BASE, LORA_RX_BUF_BASE) != SX126X_STATUS_OK)      break; // The lower half of the buffer will be tx data and the upper half of the buffer will be rx data

            // Read back packet type
            if (sx126x_get_pkt_type(radio_context, &readback_pkt_type) != SX126X_STATUS_OK) break;
            if (readback_pkt_type != SX126X_PKT_TYPE_LORA) break;

            

            init_ok = true; // Only once everything is confirmed to be set up correctly will the init_ok flag be set to true.

        } while (0);

        if (init_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X module during TX setup", "lora_tx_init");

    return false;
}

#pragma endregion

#pragma region TX

bool lora_tx(   const sx126x_context_t* radio_context, 
                sx126x_mod_params_lora_t* lora_modulation_parameters, 
                sx126x_pkt_params_lora_t* lora_packet_parameters,
                sx126x_dio_irq_masks_t* radio_interrupt_cfg,
                uint8_t *buf, 
                uint8_t len
) {
    /**
     * Per the datasheet, once the sx126x is setup using lora_init_tx(), a transmission can be sent as follows:
     *  1. Transfer the payload into the write buffer using WriteBuffer
     *  2. Define the modulation parameters using SetModulationParams
     *  3. Define the frame format to be used with SetPacketParams
     *  4. Setup interrupt on tx complete using SetDioIrqParams - TxDone IRQ source mapped to DIO[1/2/3]
     *  5. Define the sync word
     *  6. Put the sx126x in transmit mode
     *  7. Wait for IRQ or timeout
     *  8. Clear IRQ TxDone Flag
     */

    set_lora_ldro_val(lora_modulation_parameters);  // Set the Low Data Rate Optimization value by editing the given struct in-place.

    for (int k = 0; k < SPI_RETRIES; k++) {

        bool tx_ok = false;

        do {
            // 1. Write the data to be transmitted into the TX buffer
            if (sx126x_write_buffer(radio_context, 0x00, buf, len) != SX126X_STATUS_OK)                     break;

            // 2. Configure the LoRa modulation parameters (spreading factor, bandwidth, error correction)
            if (sx126x_set_lora_mod_params(radio_context, lora_modulation_parameters) != SX126X_STATUS_OK)  break;

            // 3. Configure the LoRa packet
            if (sx126x_set_lora_pkt_params(radio_context, lora_packet_parameters) != SX126X_STATUS_OK)      break;

            // 4. Configure the interrupts
            if (sx126x_set_dio_irq_params(  radio_context, 
                                            radio_interrupt_cfg->system_mask, 
                                            radio_interrupt_cfg->dio1_mask, 
                                            radio_interrupt_cfg->dio2_mask, 
                                            radio_interrupt_cfg->dio3_mask  ) != SX126X_STATUS_OK)          break;

            // 5. Set the SYNC word - helps all devices in dedicated LoRa network understand the message may be for them
            if (sx126x_set_lora_sync_word(radio_context, LWQMS_SYNC_WORD) != SX126X_STATUS_OK)              break;
            
            // 6. Transmit the data
            if (sx126x_set_tx(radio_context, SX126X_MAX_TIMEOUT_IN_MS) != SX126X_STATUS_OK)                 break;

            tx_ok = true;

        } while (0);

        if (tx_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X during packet transmission", "lora_tx");

}

#pragma endregion

#pragma region RX Init

bool lora_init_rx(sx126x_context_t* radio_context,
                sx126x_mod_params_lora_t* lora_modulation_parameters, 
                sx126x_pkt_params_lora_t* lora_packet_parameters
) {
    /**
     * Per the datasheet, setting up the sx126x in receive mode works as follows:
     *  1. Put the chip into standby mode using SetStandby
     *  2. Set the packet type to LoRa usingSetPacketType
     *  3. Define the RF frequency using SetRfFrequency
     *  4. Set the Rx buffer base address using SetBufferBaseAddress
     *  5. Define the modulation parameter using SetModulationParams
     *  6. Define the frame format to be used using SetPacketParams
     */

    set_lora_ldro_val(lora_modulation_parameters);  // Set the Low Data Rate Optimization value by editing the given struct in-place.

    for (int k = 0; k < SPI_RETRIES; k++) {

        bool init_ok = false;

        do {

            // 1. Put the radio into XTAL Oscillator Standby mode
            if (sx126x_set_standby(radio_context, SX126X_STANDBY_CFG_XOSC) != SX126X_STATUS_OK)                             break; // The WaveShare LoRa module uses a 32MHz TXCO
            
            // 2. Configure the packet type to LoRa
            if (sx126x_set_pkt_type(radio_context, SX126X_PKT_TYPE_LORA) != SX126X_STATUS_OK)                               break; // Use LoRa packets
            
            // 3. Set the RF frequency of the radio to the legal band within the USA
            if (sx126x_set_rf_freq(radio_context, LORA_FREQ_NORTH_AMERICA) != SX126X_STATUS_OK)                             break; // Use the North American Standard LoRa Frequency (915 MHz)
            
            // 4. Initialize the radio r/w buffers
            if (sx126x_set_buffer_base_address(radio_context, LORA_TX_BUF_BASE, LORA_RX_BUF_BASE) != SX126X_STATUS_OK)      break; // The lower half of the buffer will be tx data and the upper half of the buffer will be rx data

            // 5. Configure the LoRa modulation parameters (spreading factor, bandwidth, error correction)
            if (sx126x_set_lora_mod_params(radio_context, lora_modulation_parameters) != SX126X_STATUS_OK)                  break;

            // 6. Configure the LoRa packet
            if (sx126x_set_lora_pkt_params(radio_context, lora_packet_parameters) != SX126X_STATUS_OK)                      break;


        } while (0);

        if (init_ok) {
            return true;
        }

    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI transaction failure with SX126X during RX initialization", "lora_init_rx");

    return false;

}

#pragma endregion

#pragma region RX

bool lora_rx(   sx126x_context_t* radio_context,
                sx126x_dio_irq_masks_t* radio_interrupt_cfg,
                uint8_t *buf, 
                uint32_t len
) {
    /**
     * Per the datasheet, once the radio has been setup using lora_init_rx(), we can receive a packet as follows:
     *  1. Setup interrupt on rx complete using SetDioIrqParams - RxDone IRQ source mapped to DIO[1/2/3]
     *  2. Define the sync word
     *  3. Call SetRx to put the receiver in rx mode
     *  4. Wait for RxDone IRQ or Timeout
     *  5. On RxDone irq: Call GetIrqStatus to check the CRC
     *  6. Put the chip back into standby if only one packet expected 
     *  7. Clear the interrupt using ClearIrqStatus
     */

    bool rx_ok = false;

    for (int k = 0; k < SPI_RETRIES; k++) {

        do {

            // 1. Configure the interrupts
            if (sx126x_set_dio_irq_params(  radio_context, 
                                            radio_interrupt_cfg->system_mask, 
                                            radio_interrupt_cfg->dio1_mask, 
                                            radio_interrupt_cfg->dio2_mask, 
                                            radio_interrupt_cfg->dio3_mask) != SX126X_STATUS_OK)                  break;

            // 2. Set the SYNC word - helps all devices in dedicated LoRa network understand the message may be for them
            if (sx126x_set_lora_sync_word(radio_context, LWQMS_SYNC_WORD) != SX126X_STATUS_OK)              break;

            // 3. Set the radio in Receive mode
            if (sx126x_set_rx(radio_context, LORA_TIMEOUT_MS) != SX126X_STATUS_OK)                          break;

            rx_ok = true;

        } while (0);

        if (rx_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X during packet reception", "lora_rx");
}

#pragma endregion