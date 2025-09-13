/*************************************************************************************
 * 
 * @file mxl23l3233f.c
 * 
 * @brief Driver for the MXL23L3233F 32MBit Serial NOR Flash IC with SPI Interface.
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#include "mxl23l3233f.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static int copy_with_offset(void *destination, size_t dest_len, size_t dest_offset, const void *source, size_t src_len, size_t src_offset, size_t num_bytes) {
    // Validate pointers
    if (!destination || !source) return -1;

    // Check that offsets are within bounds
    if (src_offset > src_len) return -2;
    if (dest_offset > dest_len) return -3;

    // Ensure we have enough space in source and destination
    if (src_offset + num_bytes > src_len) return -4;
    if (dest_offset + num_bytes > dest_len) return -5;

    // Perform the safe copy
    uint8_t *dest_ptr = (uint8_t *)destination + dest_offset;
    const uint8_t *src_ptr = (const uint8_t *)source + src_offset;

    memcpy(dest_ptr, src_ptr, num_bytes);

    return 0; // Success
}

/**
 * @brief Determines if all elements in the given array are equal to the target byte
 * 
 * @returns True if all elements are zero, false otherwise.
 */
static bool are_all_matching(uint8_t *arr, uint size, uint8_t target) {
    for (int i = 0; i < size; i++) {
        if (*(arr + i) != target) {
            return false; // Found a non-zero element
        }
    }
    return true; // All elements are zero
}

/**
 * @brief Performs an asymmetrical read operation on the SPI flash chip by writing the TX buffer to the SPI bus, simultaneously reading back activity
 * from the IC on that bus, then chopping off the leading zeroes sent by the chip while the command was still being sent, leaving only the useful data.
 */
static int mxl23l3233f_spi_transaction(mxl23l3233f_context_t *context, uint8_t *txBuf, size_t txLen, size_t cmd_len, uint8_t *data_buf) {
    // Allocate a full-sized temporary buffer on the stack
    uint8_t rxBuf[txLen];
    
    // Assert chip select low
    gpio_write_hal(context->cs, GPIO_LOW);

    // Perform the transaction
    int bytes = spi_rw_hal(context->spi_context, txBuf, rxBuf, txLen);

    // Assert chip select high
    gpio_write_hal(context->cs, GPIO_HIGH);

    uint data_len = txLen - cmd_len;

    if (data_len > 0) {
        // Remove leading cmd_len bytes from the RX buffer and save them to the data received buffer.
        if (copy_with_offset(data_buf, data_len, 0, rxBuf, txLen, cmd_len, data_len) < 0) {
            err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Invalid buffer sizes provided for SPI transaction", "spi_transaction");
            return -1;
        }
    }
    else {
        data_buf[0] = 0x00;
    }

    return bytes;
}

/**
 * @brief Checks if Serial Flash Chip Instance is undergoing a write or erase operation. 
 * 
 * @returns True if the Write In Progress (WIP) bit is set in the status register.
 */
static bool mxl23l3233f_is_busy(mxl23l3233f_context_t *context) {

    bool busy_check_ok = false;
    uint8_t status_reg;

    if (mxl23l3233f_read_status_register(context, &status_reg) < 0) return false;

    return status_reg & STATUS_REG_WIP > 0;
}

/**
 * @brief Waits for the given Serial Flash Instance to enter the idle state, indicating it is ready for a new write/erase operation.
 * 
 * @returns None
 */
static void mxl23l3233f_wait_for_chip_idle(mxl23l3233f_context_t *context) {
    while (mxl23l3233f_is_busy(context)) {};
}

/**
 * @brief Shared logic for erase operations for the Serial NOR Flash IC.
 */
static int mx23l3233_erase_generic(mxl23l3233f_context_t *context, uint8_t command, uint32_t address, uint32_t size) {
    
    uint8_t buflen = 4; // Command byte + 3 address bytes
    uint8_t txBuf[buflen];
    uint8_t rxBuf[buflen];

    // Setup the command sequence
    txBuf[0] = command;
    txBuf[1] = (address >> 16) & 0xff;
    txBuf[2] = (address >> 8) & 0xff;
    txBuf[3] = address & 0xff;

    uint8_t test_page[size];

    bool erase_ok = false;

    for (int k = 0; k < SPI_RETRIES; k++) {
        do {
            // Wait for the chip to be ready for an erase operation
            mxl23l3233f_wait_for_chip_idle(context);
            
            // Set Write Enable Latch
            if (mxl23l3233f_write_enable(context) < 0) break;

            // Transmit the chip erase command
            if (mxl23l3233f_spi_transaction(context, txBuf, buflen, buflen, rxBuf) < 0) break;

            // Wait for the erase operation to complete
            mxl23l3233f_wait_for_chip_idle(context);

            // Read the first page of the memory to see if it has been cleared.
            if (mxl23l3233f_read_data(context, test_page, size, address) < 0) break;
            
            // Check that all bits have been set to 1.
            if (!are_all_matching(test_page, size, 0xff)) break;

            // Clear Write Enable Latch
            if (mxl23l3233f_write_disable(context) < 0) break;

            erase_ok = true;
        } while (0);

        if (erase_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to perform erasure on Serial NOR Flash!", "mxl23l3233f_erase_generic");
    return -1;
}

void hexdump(const uint8_t *data, size_t length, size_t start_offset) {
    const size_t bytes_per_line = 16; // Standard hexdump width

    for (size_t i = 0; i < length; i += bytes_per_line) {
        // Print the offset on the left
        printf("%08zx  ", start_offset + i);

        // Print hex bytes for this line
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length)
                printf("%02x ", *(data + i + j)); // Pointer-based indexing
            else
                printf("   "); // Padding for short lines
        }

        // Space between hex and ASCII
        printf(" ");

        // Print ASCII representation
        for (size_t j = 0; j < bytes_per_line; j++) {
            if (i + j < length) {
                uint8_t c = *(data + i + j);
                printf("%c", (c >= 32 && c <= 126) ? c : '.');
            }
        }

        printf("\n");
    }
}

int mxl23l3233f_read_jedec_id(mxl23l3233f_context_t *context, uint8_t *IDBuf, size_t idLen) {
    
    // We will need to carefully document everything with these functions as these could produce
    // memory leaks if we aren't careful. The caller of these functions (since heap allocation
    // is a no-no for embedded) will have to guarantee that all the buffers are the correct length.

    // The SPI buffer will contain all of the useful information (idLen long) plus 
    //one throwaway 0x00 at the beginning while the command byte is being transmitted.
    uint buflen = idLen + 1;

    uint8_t txBuf[buflen];

    // Pre-initialize the TX buffer to all zeroes to eliminate any garbage being put on the SPI bus.
    memset(txBuf, 0x00, buflen);

    // Set the command in position 1 of the TX buffer
    txBuf[0] = CMD_READ_JEDEC_ID;

    mxl23l3233f_spi_transaction(context, txBuf, buflen, 1, IDBuf);

    return 0;
}

int mxl23l3233f_page_program(mxl23l3233f_context_t *context, uint8_t *page_contents, size_t data_len, uint32_t address) {

    // Check that the page can support the length of data requested.
    size_t remaining_page_size = FLASH_PAGE_SIZE - (address & 0xff);    // Each page is 256 bytes long, so the remaining number of bytes is 256 - last 8 bits of the address

    if (data_len > remaining_page_size) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "The requested page data size exceeds the amount that can be programmed to the page.", "mxl23l3233f_page_program");
        return -1;
    }

    uint cmd_len = 4;
    uint buflen = data_len + cmd_len; // 1 Command Byte + 3 Address Bytes

    uint8_t txBuf[buflen];
    uint8_t rxBuf[buflen];

    uint8_t data_readback_buf[data_len];

    // Setup the TX Buffer - Command - Address - Data
    txBuf[0] = CMD_PAGE_PROGRAM;
    txBuf[1] = (address >> 16) & 0xff;
    txBuf[2] = (address >> 8) & 0xff;
    txBuf[3] = address & 0xff;
    memcpy(&txBuf[cmd_len], page_contents, data_len);

    bool page_program_ok = false;

    for (int k = 0; k < SPI_RETRIES; k++) {
        do {
            // Ensure the chip is ready for a write/erase operation.
            mxl23l3233f_wait_for_chip_idle(context);

            // Set write enable
            if (mxl23l3233f_write_enable(context) < 0) break;

            // Write the data
            if (mxl23l3233f_spi_transaction(context, txBuf, buflen, cmd_len, rxBuf) < 0) break;

            // Wait for the write operation to be completed
            mxl23l3233f_wait_for_chip_idle(context);

            // Read back data
            if (mxl23l3233f_read_data(context, data_readback_buf, data_len, address) < 0) break;

            // Check that the data programmed matches the data sent.
            if (memcmp(page_contents, data_readback_buf, data_len) != 0) break;

            // Clear write enable
            if (mxl23l3233f_write_disable(context) < 0) break;

            page_program_ok = true;

        } while (0);

        if (page_program_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to program page!", "mxl23l3233f_page_program");
    return -1;    
}

int mxl23l3233f_write_data(mxl23l3233f_context_t *context, uint8_t *data, size_t data_len, uint32_t start_address) {
    
    /**
     * When writing data to the flash IC, we need to program 1 page at a time (256 bytes).
     * If we are programming a page starting at the middle, and we reach the end of the page, the data will
     * wrap around to the beginning of the page, meaning the data won't be where we thought we put it.
     * 
     * This requires careful management of memory addresses.
     */

    size_t bytes_remaining = data_len;
    uint32_t current_address = start_address;
    size_t data_offset = 0;

    while (bytes_remaining > 0) {
        // Calculate remaining space in the current page
        size_t page_offset = current_address & (FLASH_PAGE_SIZE - 1);
        size_t space_in_page = FLASH_PAGE_SIZE - page_offset;

        // Calculate how many bytes to write this iteration
        size_t bytes_to_write = (bytes_remaining < space_in_page) ? bytes_remaining : space_in_page;

        // Attempt to write this chunk
        if (mxl23l3233f_page_program(context, &data[data_offset], bytes_to_write,current_address) < 0) {
            // If a single page write fails, stop immediately
            err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Multi-page write failed!", "mxl23l3233f_write_data");
            return -1;
        }

        // Advance pointers for next iteration
        current_address += bytes_to_write;
        data_offset     += bytes_to_write;
        bytes_remaining -= bytes_to_write;
    }

    return 0; // Success
}

int mxl23l3233f_read_data(mxl23l3233f_context_t *context, uint8_t *rxBuf, size_t rxLen, uint32_t address) {

    // Can do standard read operation for f_SCL < 50MHz.
    size_t cmd_len = 4;
    size_t buflen = rxLen + cmd_len;  // The number of transmitted bytes will be 1 command byte + 3 address bytes + rxLen dummy zeroes.
    uint8_t txBuf[buflen];

    // Pre-initialize the transmit buffer to zeroes.
    memset(txBuf, 0x00, buflen);

    // Setup the command sequence
    txBuf[0] = CMD_READ;
    txBuf[1] = (address >> 16) & 0xff;
    txBuf[2] = (address >> 8) & 0xff;
    txBuf[3] = address & 0xff;

    bool read_ok = false;

    mxl23l3233f_wait_for_chip_idle(context);

    for (int k = 0; k < SPI_RETRIES; k++) {
        do {
            if (mxl23l3233f_spi_transaction(context, txBuf, buflen, cmd_len, rxBuf) < 0) break;
            read_ok = true;
        } while (0);
        
        if (read_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Read Data Failure from Serial NOR Flash IC", "mxl23l3233f_read_data");

    return -1;
}

int mxl23l3233f_erase_sector(mxl23l3233f_context_t *context, uint32_t sector_index) {

    /**
     * Erases a 4kB block on the memory IC. Sectors are numbered from 0->1023, and are 0x1000 long.
     * 
     * To control valid sector start addresses without using modulo, we will request instead the sector index (0->1023)
     * and then compute the equivalent memory address.
     */

    // Validate the sector index
    if (sector_index > (context->sectors - 1)) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Invalid sector address! The sector index can not be greater than the number of sectors on the memory device.", "mxl23l3233f_erase_sector");
        return -1;
    }
    
    uint32_t address = (sector_index << 12) & 0xffffff;  // Add 12 trailing zeroes to the sector index and chop it to 24 bits. 

    return mx23l3233_erase_generic(context, CMD_SECTOR_ERASE, address, FLASH_SECTOR_SIZE);
}

int mxl23l3233f_erase_32kb_block(mxl23l3233f_context_t *context, uint32_t block_index) {

    // Validate the block index
    if (block_index > (context->blocks_32kB - 1)) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Invalid 32KB block address! The block index can not be greater than the number of blocks on the memory device.", "mxl23l3233f_erase_32kb_block");
        return -1;
    }

    uint32_t address = (block_index * FLASH_BLOCK_32KB_SIZE) & 0xffffff;

    return mx23l3233_erase_generic(context, CMD_BLOCK_ERASE_32KB, address, FLASH_BLOCK_32KB_SIZE);
}

int mxl23l3233f_erase_64kb_block(mxl23l3233f_context_t *context, uint32_t block_index) {
    // Validate the block index
    if (block_index > (context->blocks_64kB - 1)) {
        err_raise(ERR_ARGUMENT, ERR_SEV_NONFATAL, "Invalid 64KB block address! The block index can not be greater than the number of blocks on the memory device.", "mxl23l3233f_erase_32kb_block");
        return -1;
    }

    uint32_t address = (block_index * FLASH_BLOCK_64KB_SIZE) & 0xffffff;

    return mx23l3233_erase_generic(context, CMD_BLOCK_ERASE_64KB, address, FLASH_BLOCK_64KB_SIZE);
}

int mxl23l3233f_chip_erase(mxl23l3233f_context_t *context) {

    uint8_t txBuf = CMD_CHIP_ERASE;
    uint8_t rxBuf;

    bool chip_erase_ok = false;

    uint8_t test_page[FLASH_PAGE_SIZE];

    
    for (int k = 0; k < SPI_RETRIES; k++) {
        do {
            // Wait for the chip to be ready for an erase operation
            mxl23l3233f_wait_for_chip_idle(context);
            
            // Set Write Enable Latch
            if (mxl23l3233f_write_enable(context) < 0) break;

            // Transmit the chip erase command
            if (mxl23l3233f_spi_transaction(context, &txBuf, 1, 1, &rxBuf) < 0) break;

            // Wait for the erase operation to complete
            mxl23l3233f_wait_for_chip_idle(context);

            // Read the first page of the memory to see if it has been cleared.
            if (mxl23l3233f_read_data(context, test_page, FLASH_PAGE_SIZE, 0) < 0) break;
            
            // Check that all bits have been set to 1.
            if (!are_all_matching(test_page, FLASH_PAGE_SIZE, 0xff)) break;

            // Clear Write Enable Latch
            if (mxl23l3233f_write_disable(context) < 0) break;

            chip_erase_ok = true;
        } while (0);

        if (chip_erase_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to perform a full chip erasure of the Serial NOR Flash!", "mxl23l3233f_chip_erase");
    return -1;
}

int mxl23l3233f_deep_power_down(mxl23l3233f_context_t *context) {

    uint8_t powerdown_txBuf = CMD_DEEP_POWER_DOWN;
    uint8_t powerdown_rxBuf;

    uint IDBufLen = 3;
    uint8_t IDBuf[IDBufLen];

    bool power_down_ok = false;

    // Wait for the chip to be ready to enter the deep power down mode
    mxl23l3233f_wait_for_chip_idle(context);

    for (int k = 0; k < FLASH_SPI_RETRIES; k++) {
        do {
            // Send the Deep Power Down Command
            if (mxl23l3233f_spi_transaction(context, &powerdown_txBuf, 1, 1, &powerdown_rxBuf) < 0) break;

            // Give the chip plenty of time to enter this mode - max 10us.
            sleep_us(30);

            // Attempt to read the JEDEC ID of the chip. The command should be ignored.
            if (mxl23l3233f_read_jedec_id(context, IDBuf, IDBufLen) < 0) break;

            // Check that the received ID buffer was all zeroes (no response from chip).
            if (!are_all_matching(IDBuf, IDBufLen, 0x00)) break;

            power_down_ok = true;
        } while (0);

        if (power_down_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to put serial NOR Flash IC into deep power down mode!", "mxl23l3233f_deep_power_down");

    return -1;
}

int mxl23l3233f_deep_power_down_release(mxl23l3233f_context_t *context) {

    uint buflen = 5;        // Command byte + 3 dummy bytes +  1 byte Electronic Signature Received from Chip
    uint8_t txBuf[buflen];
    uint8_t rxBuf;          // 1 Byte Electronic Signature

    memset(txBuf, 0x00, buflen);

    txBuf[0] = CMD_DEEP_POWER_DOWN_RELEASE;

    bool wakeup_ok = false;

    for (int k = 0; k < SPI_RETRIES; k++) {
        do {
            // Send the wakeup command plus 3 dummy bytes.
            if (mxl23l3233f_spi_transaction(context, txBuf, buflen, 4, &rxBuf) < 0) break;

            // Check for a response from the chip
            if (rxBuf == 0) break;

            wakeup_ok = true;
        } while (0);

        if (wakeup_ok) {
            return rxBuf;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_FATAL, "Failed to wake up Serial NOR Flash from Deep Sleep!", "mxl23l3233f_deep_power_down_release");

    return -1;
}

int mxl23l3233f_write_enable(mxl23l3233f_context_t *context) {
    
    uint8_t txBuf = CMD_WRITE_ENABLE;
    uint8_t rxBuf;

    bool write_enable_ok = false;

    for (int k = 0; k < FLASH_SPI_RETRIES; k++) {
        do {
            // Send the write enable command
            if (mxl23l3233f_spi_transaction(context, &txBuf, 1, 1, &rxBuf) < 0) break;
    
            uint8_t status_reg;

            // Read back the status register
            if (mxl23l3233f_read_status_register(context, &status_reg) < 0) break;

            // Check that the write enable bit was set
            if ((status_reg & STATUS_REG_WEL) <= 0) break;

            // If it has been set, the operation was successful.
            write_enable_ok = true;
    
        } while (0);

        if (write_enable_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "SPI Communications Failure with Serial NOR Flash!", "mxl23l3233f_write_enable");
    return -1;
}

int mxl23l3233f_write_disable(mxl23l3233f_context_t *context) {

    uint8_t txBuf = CMD_WRITE_DISABLE;
    uint8_t rxBuf;

    bool write_disable_ok = false;

    for (int k = 0; k < FLASH_SPI_RETRIES; k++) {
        do {
            if (mxl23l3233f_spi_transaction(context, &txBuf, 1, 1, &rxBuf) < 0) break;
    
            uint8_t status_reg;

            if (mxl23l3233f_read_status_register(context, &status_reg) < 0) break;

            if ((status_reg & STATUS_REG_WEL) > 0) break;

            write_disable_ok = true;
    
        } while (0);

        if (write_disable_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "SPI Communications Failure with Serial NOR Flash!", "mxl23l3233f_write_disable");
    return -1;
}

int mxl23l3233f_read_status_register(mxl23l3233f_context_t *context, uint8_t *status_register) {

    uint8_t txBuf[2] = { CMD_READ_STATUS_REG, 0x00 };
    
    bool rd_status_ok = false;

    for (int k = 0; k < FLASH_SPI_RETRIES; k++) {
        do {
            // Send the read status register command and read back the contents of the status register.
            if (mxl23l3233f_spi_transaction(context, txBuf, 2, 1, status_register) < 0) break;
            rd_status_ok = true;
        } while (0);

        if (rd_status_ok) {
            return 0;
        }
    }

    err_raise(ERR_SPI_TRANSACTION_FAIL, ERR_SEV_NONFATAL, "Failed to read the status register of the Serial NOR Flash", "mxl23l3233f_read_status_register");

    return -1;    
}

/* --- EOF ------------------------------------------------------------------ */