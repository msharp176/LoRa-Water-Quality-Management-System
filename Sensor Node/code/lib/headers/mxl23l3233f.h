/*************************************************************************************
 * 
 * @file mxl23l3233f.h
 * 
 * @brief Header file for the MXL23L3233F 32MBit Serial NOR Flash IC with SPI Interface Driver.
 * 
 * @author Matthew Sharp
 * 
 * @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
 *
 * ************************************************************************************/

#ifndef MXL23L3233F_H
#define MXL23L3233F_H

#include "hardware.h"
#include "hal.h"


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Defines 

#define FLASH_COMMS_RETRIES 5

#define FLASH_PAGE_SIZE          0x100      ///< Flash page size (256 bytes)
#define FLASH_SECTOR_SIZE       0x1000      ///< Flash sector size (4KB)
#define FLASH_BLOCK_32KB_SIZE   0x8000      ///< Flash 32KB block size
#define FLASH_BLOCK_64KB_SIZE  0x10000      ///< Flash 64KB block size

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Enums 

/**
 * @brief Flash command opcodes for the MXL23L3233F Serial NOR Flash IC.
 */
typedef enum mxl23l3233f_commands_e {
    CMD_READ =                      0x03,
    CMD_FAST_READ =                 0x0b,
    CMD_2READ =                     0xbb,
    CMD_DREAD =                     0x3b,
    CMD_4READ =                     0xeb,
    CMD_QREAD =                     0x6b,
    CMD_WRITE_ENABLE =              0x06,
    CMD_WRITE_DISABLE =             0x04,
    CMD_READ_STATUS_REG =           0x05,
    CMD_READ_CFG_REG =              0x15,
    CMD_WRITE_STATUS_REG =          0x01,
    CMD_QUAD_PG_PRG =               0x38,
    CMD_SECTOR_ERASE =              0x20,
    CMD_BLOCK_ERASE_32KB =          0x52,
    CMD_BLOCK_ERASE_64KB =          0xd8,
    CMD_CHIP_ERASE =                0xc7,
    CMD_PAGE_PROGRAM =              0x02,
    CMD_DEEP_POWER_DOWN =           0xb9,
    CMD_DEEP_POWER_DOWN_RELEASE =   0xab,
    CMD_SUSPEND_PROGRAM_ERASE =     0x75,
    CMD_RESUME_PROGRAM_ERASE =      0x7a,
    CMD_READ_JEDEC_ID =             0x9f,
    CMD_READ_ELECTRONIC_ID =        0xab,
    CMD_READ_MFG_DEV_ID =           0x90,
    CMD_ENTER_SEC_OTP =             0xb1,
    CMD_EXIT_SEC_OTP =              0xc1,
    CMD_READ_SEC_REG =              0x2b,
    CMD_WRITE_SEC_REG =             0x2f,
    CMD_RESET_EN =                  0x66,
    CMD_RESET_MEM =                 0x99,
    CMD_READ_SFDP_MODE =            0x5a,
    CMD_SET_BURST_LEN =             0xc0,
    CMD_NOP =                       0x00
} mxl23l3233f_commands_t;

/**
 * @brief Bit definitions for the Status Register.
 */
typedef enum mxl23l3233f_status_reg_e {
    STATUS_REG_SRWD =   (1 << 7),
    STATUS_REG_QE =     (1 << 6),
    STATUS_REG_BP3 =    (1 << 5),
    STATUS_REG_BP2 =    (1 << 4),
    STATUS_REG_BP1 =    (1 << 3),
    STATUS_REG_BP0 =    (1 << 2),
    STATUS_REG_WEL =    (1 << 1),
    STATUS_REG_WIP =    (1 << 0)
} mxl23l3233f_status_reg_t;

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

/**
 * @brief Prints a formatted hexdump of the provided binary data buffer.
 *
 * @param data         Pointer to the buffer containing data to dump.
 * @param length       Number of bytes to print.
 * @param start_offset Starting offset displayed on the left of the dump.
 *
 * @return None.
 */
void hexdump(const uint8_t *data, size_t length, size_t start_offset);

/**
 * @brief Reads the JEDEC ID (manufacturer & device ID) from the Serial NOR Flash IC.
 *
 * @param context   Pointer to flash context.
 * @param IDBuf     Pointer to buffer to store JEDEC ID.
 * @param idLen     Length of the ID buffer.
 *
 * @return 0 if successful, -1 if an error occurs.
 */
int mxl23l3233f_read_jedec_id(mxl23l3233f_context_t *context, uint8_t *IDBuf, size_t idLen);

/**
 * @brief Programs up to one page (256 bytes) into the flash IC.
 *
 * @param context        Pointer to flash context.
 * @param page_contents  Pointer to buffer holding data to be programmed.
 * @param data_len       Number of bytes to program (must not cross a page boundary).
 * @param address        Starting 24-bit memory address.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_page_program(mxl23l3233f_context_t *context, uint8_t *page_contents, size_t data_len, uint32_t address);

/**
 * @brief Writes an arbitrary-length data buffer to flash, automatically splitting across page boundaries.
 *
 * @param context   Pointer to flash context.
 * @param txBuf     Pointer to buffer containing data to write.
 * @param txLen     Total number of bytes to write.
 * @param address   Starting memory address for write.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_write_data(mxl23l3233f_context_t *context, uint8_t *txBuf, size_t txLen, uint32_t address);

/**
 * @brief Reads data from the flash IC starting at a given address.
 *
 * @param context   Pointer to flash context.
 * @param rxBuf     Pointer to buffer to store read data.
 * @param rxLen     Number of bytes to read.
 * @param address   Starting memory address to read from.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_read_data(mxl23l3233f_context_t *context, uint8_t *rxBuf, size_t rxLen, uint32_t address);

/**
 * @brief Erases a 4KB sector of flash memory.
 *
 * @param context       Pointer to flash context.
 * @param sector_index  Index of sector to erase (0-based).
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_erase_sector(mxl23l3233f_context_t *context, uint32_t sector_index);

/**
 * @brief Erases a 32KB block of flash memory.
 *
 * @param context      Pointer to flash context.
 * @param block_index  Index of 32KB block to erase.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_erase_32kb_block(mxl23l3233f_context_t *context, uint32_t block_index);

/**
 * @brief Erases a 64KB block of flash memory.
 *
 * @param context      Pointer to flash context.
 * @param block_index  Index of 64KB block to erase.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_erase_64kb_block(mxl23l3233f_context_t *context, uint32_t block_index);

/**
 * @brief Performs a full chip erase, clearing the entire flash memory.
 *
 * @param context   Pointer to flash context.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_chip_erase(mxl23l3233f_context_t *context);

/**
 * @brief Puts the flash IC into deep power-down mode to reduce power consumption.
 *
 * @param context   Pointer to flash context.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_deep_power_down(mxl23l3233f_context_t *context);

/**
 * @brief Wakes the flash IC from deep power-down mode.
 *
 * @param context   Pointer to flash context.
 *
 * @return The electronic signature (chip ID) on success, or -1 on failure.
 */
int mxl23l3233f_deep_power_down_release(mxl23l3233f_context_t *context);

/**
 * @brief Enables write operations on the flash IC by setting the Write Enable Latch (WEL).
 *
 * @param context   Pointer to flash context.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_write_enable(mxl23l3233f_context_t *context);

/**
 * @brief Disables write operations on the flash IC by clearing the Write Enable Latch (WEL).
 *
 * @param context   Pointer to flash context.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_write_disable(mxl23l3233f_context_t *context);

/**
 * @brief Reads the status register from the flash IC.
 *
 * @param context          Pointer to flash context.
 * @param status_register  Pointer to a variable to store the status register value.
 *
 * @return 0 if successful, -1 on failure.
 */
int mxl23l3233f_read_status_register(mxl23l3233f_context_t *context, uint8_t *status_register);

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* MXL23L3233F_H */

/* --- EOF ------------------------------------------------------------------ */