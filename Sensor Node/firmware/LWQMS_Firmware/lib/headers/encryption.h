/******************************************************************************************************************** 
*   
*   @file encryption.h 
*
*   @brief Header File for encryption operations on the LoRa Water Quality Management System Sensor Node
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#ifndef ENCRYPTION_H
#define ENCRYPTION_H

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Dependencies

#include "aes.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Types

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------------------------------
// Function Definitions

size_t pkcs7_pad(uint8_t* buffer, size_t data_len);

size_t pkcs7_unpad(const uint8_t* buffer, size_t padded_len);

/**
 * @brief Encrypt data using AES-128 in CBC mode with PKCS#7 padding.
 *
 * @param key         Pointer to a 16-byte AES key.
 * @param keylen      Length of the key in bytes (must be 16 for AES-128).
 * @param inbuf       Pointer to the plaintext buffer.
 * @param in_buflen   Length of the plaintext buffer in bytes.
 * @param outbuf      Pointer to the caller-provided output buffer.
 *                    Must be at least AES_BLOCKLEN + in_buflen + AES_BLOCKLEN bytes
 *                    (for IV + ciphertext + worst-case padding).
 * @param out_buflen  Pointer to a variable that will receive the actual
 *                    length of data written into outbuf.
 *
 * @returns true on success, false on failure (e.g., invalid key length).
 */
bool aes_128_encrypt(const uint8_t *key, size_t keylen,
                     const uint8_t *inbuf, size_t in_buflen,
                     uint8_t *outbuf, size_t *out_buflen);

/**
 * @brief Decrypt data using AES-128 in CBC mode with PKCS#7 unpadding.
 *
 * @param key         Pointer to a 16-byte AES key.
 * @param keylen      Length of the key in bytes (must be 16 for AES-128).
 * @param inbuf       Pointer to the buffer containing IV + ciphertext.
 * @param in_buflen   Length of the input buffer in bytes.
 * @param outbuf      Pointer to the caller-provided output buffer.
 *                    Must be at least in_buflen bytes (padding will shrink it).
 * @param out_buflen  Pointer to a variable that will receive the actual
 *                    length of decrypted plaintext.
 *
 * @returns true on success, false on failure (e.g., invalid key length, bad padding).
 */
bool aes_128_decrypt(const uint8_t *key, size_t keylen,
                     const uint8_t *inbuf, size_t in_buflen,
                     uint8_t *outbuf, size_t *out_buflen);


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------

#endif /* ENCRYPTION_H */

/* --- EOF ------------------------------------------------------------------ */