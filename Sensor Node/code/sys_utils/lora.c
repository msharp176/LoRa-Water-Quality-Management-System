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

#include "lora.h"

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

#pragma region Initialize Radio

sx126x_status_t sx126x_radio_setup(const void* context) {

    bool init_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {

        do {
            // 1. Perform a reset of the radio module
            if (sx126x_hal_reset(context) != SX126X_STATUS_OK)                          break;
        
            //printf("1");

            // 2. Wakeup the radio
            if (sx126x_hal_wakeup(context) != SX126X_STATUS_OK)                         break;
        
            //printf("2");

            // 3. Set the regulator mode
            if (sx126x_set_reg_mode(context, SX126X_REG_MODE_DCDC) != SX126X_STATUS_OK) break;
        
            //printf("3");

            // 4. Set DIO2 to control the RF switch
            if (sx126x_set_dio2_as_rf_sw_ctrl(context, true) != SX126X_STATUS_OK)       break;

            //printf("4");

            // 5. Set DIO3 to control the TCXO
            if (sx126x_set_dio3_as_tcxo_ctrl(context, SX126X_TCXO_CTRL_1_7V, SX126X_TCXO_TIMEOUT) != SX126X_STATUS_OK)  break;

            //printf("5");

            // 6. Calibrate the radio
            if (sx126x_cal(context, SX126X_CAL_ALL) != SX126X_STATUS_OK)                break;

            //printf("6");

            init_ok = true;

        } while (0);

        if (init_ok) {
            return SX126X_STATUS_OK;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X module during radio initialization", "sx126x_hal_initialize_radio");

    return SX126X_STATUS_ERROR;

}

#pragma endregion

#pragma region Power Management

bool lora_enter_sleep_mode(const sx126x_context_t* radio_context, bool use_warm_start) {
    
    sx126x_sleep_cfgs_t sleep_mode = use_warm_start ? SX126X_SLEEP_CFG_WARM_START : SX126X_SLEEP_CFG_COLD_START;
    
    for (int k = 0; k < COMMS_RETRIES; k++) {
        bool sleep_ok = false;
        do {
            if (sx126x_set_sleep(radio_context, sleep_mode) != SX126X_STATUS_OK) break;

            // block for 500us
            sleep_us(500);

            // Wait for the BUSY line to be asserted HIGH.
            if (poll_radio_busy(radio_context, GPIO_HIGH) != SX126X_STATUS_OK) break;
            
            sleep_ok = true;

        } while (0);

        if (sleep_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X module during sleep mode initialization", "lora_enter_sleep_mode");

    return false;

}

#pragma endregion

#pragma region TX Init

bool lora_init_tx(  const sx126x_context_t* radio_context, 
                    sx126x_pa_cfg_params_t* power_amplifier_config,
                    sx126x_mod_params_lora_t* lora_modulation_parameters, 
                    int8_t  txPower,
                    sx126x_ramp_time_t ramp_time,
                    uint8_t sync_word
) {
    set_lora_ldro_val(lora_modulation_parameters);  // Set the Low Data Rate Optimization value by editing the given struct in-place.
    
    // Write all the configuration data
    for (int k = 0; k < COMMS_RETRIES; k++) {
        
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
            if (sx126x_set_pa_cfg(radio_context, power_amplifier_config) != SX126X_STATUS_OK)                               break;
            
            // 5. Configure the transmission parameters
            if (sx126x_set_tx_params(radio_context, txPower, ramp_time) != SX126X_STATUS_OK)                                break; // Set to use a 200 us Ramp Time
            
            // 6. Initialize the radio r/w buffers
            if (sx126x_set_buffer_base_address(radio_context, radio_context->tx_buf_start, radio_context->rx_buf_start) != SX126X_STATUS_OK)      break; // The lower half of the buffer will be tx data and the upper half of the buffer will be rx data

            // 7. Configure the LoRa modulation parameters (spreading factor, bandwidth, error correction)
            if (sx126x_set_lora_mod_params(radio_context, lora_modulation_parameters) != SX126X_STATUS_OK)                  break;

            // 8. Set the SYNC word - helps all devices in dedicated LoRa network understand the message may be for them
            if (sx126x_set_lora_sync_word(radio_context, sync_word) != SX126X_STATUS_OK)                                    break;

            // Read back packet type
            if (sx126x_get_pkt_type(radio_context, &readback_pkt_type) != SX126X_STATUS_OK)                                 break;

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
                sx126x_dio_irq_masks_t* radio_interrupt_cfg,
                sx126x_pkt_params_lora_t* lora_packet_parameters,
                uint8_t *buf, 
                uint8_t len
) {
    for (int k = 0; k < COMMS_RETRIES; k++) {

        bool tx_ok = false;

        do {
            // 1. Configure the LoRa packet
            lora_packet_parameters->pld_len_in_bytes = len;
            if (sx126x_set_lora_pkt_params(radio_context, lora_packet_parameters) != SX126X_STATUS_OK) break;

            // 2. Write the data to be transmitted into the TX buffer
            if (sx126x_write_buffer(radio_context, 0x00, buf, len) != SX126X_STATUS_OK) break;

            // 3. Configure the interrupts
            if (sx126x_set_dio_irq_params(  radio_context, 
                                            radio_interrupt_cfg->system_mask, 
                                            radio_interrupt_cfg->dio1_mask, 
                                            radio_interrupt_cfg->dio2_mask, 
                                            radio_interrupt_cfg->dio3_mask  ) != SX126X_STATUS_OK) break;
            
            // 4. Transmit the data
            if (sx126x_set_tx(radio_context, 10000) != SX126X_STATUS_OK) break;

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

    sx126x_pkt_type_t readback_pkt_type;

    for (int k = 0; k < COMMS_RETRIES; k++) {

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

            // Read back packet type
            if (sx126x_get_pkt_type(radio_context, &readback_pkt_type) != SX126X_STATUS_OK)                                 break;

            if (readback_pkt_type != SX126X_PKT_TYPE_LORA) break;       

            init_ok = true;

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
                uint8_t sync_word,
                uint32_t timeout_ms
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

    for (int k = 0; k < COMMS_RETRIES; k++) {

        do {

            // 1. Configure the interrupts
            if (sx126x_set_dio_irq_params(  radio_context, 
                                            radio_interrupt_cfg->system_mask, 
                                            radio_interrupt_cfg->dio1_mask, 
                                            radio_interrupt_cfg->dio2_mask, 
                                            radio_interrupt_cfg->dio3_mask) != SX126X_STATUS_OK)                  break;

            // 2. Set the SYNC word - helps all devices in dedicated LoRa network understand the message may be for them
            if (sx126x_set_lora_sync_word(radio_context, sync_word) != SX126X_STATUS_OK)              break;

            // 3. Set the radio in Receive mode
            if (sx126x_set_rx(radio_context, timeout_ms) != SX126X_STATUS_OK)                          break;

            rx_ok = true;

        } while (0);

        if (rx_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_REBOOT, "SPI Communications failure with SX126X during packet reception", "lora_rx");

    return false;
}

bool lora_get_rx_data(sx126x_context_t* radio_context, uint8_t * rxBuf, uint8_t * rxLen) {

    bool data_retrieval_ok = false;

    for (int k = 0; k < COMMS_RETRIES; k++) {
        do {
            sx126x_rx_buffer_status_t bufStatus;

            if (sx126x_get_rx_buffer_status(radio_context, &bufStatus) != SX126X_STATUS_OK) break;

            if (sx126x_read_buffer(radio_context, bufStatus.buffer_start_pointer, rxBuf, bufStatus.pld_len_in_bytes) != SX126X_STATUS_OK) break;

            data_retrieval_ok = true;

            *rxLen = bufStatus.pld_len_in_bytes;

        } while (0);

        if (data_retrieval_ok) {
            return true;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "SPI Communications failure with SX126X during packet retrieval", "lora_get_rx_data");

    return false;
}

#pragma endregion