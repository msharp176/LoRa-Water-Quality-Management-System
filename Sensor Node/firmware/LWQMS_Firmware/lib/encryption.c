/******************************************************************************************************************** 
*   
*   @file encryption.c 
*
*   @brief Function definitions for encryption operations on the LoRa Water Quality Management System Sensor Node
*
*   @author Matthew Sharp
*   
*   @remark Penn State Harrisburg Electrical Engineering Senior Capstone Design Course, Fall 2025
*
*********************************************************************************************************************/

#include "encryption.h"

/**
 * @brief Fills buffer with cryptographically secure random bytes.
 *
 * This is just a placeholder — replace with platform-appropriate implementation
 * (e.g., /dev/urandom, hardware RNG, or crypto library).
 */
static bool get_random_bytes(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buf[i] = rand() % 256;  // WARNING: not cryptographically secure!
    }
    return true;
}

// PKCS#7 Padding function
// Returns the padded length. Modifies the buffer in place.
size_t pkcs7_pad(uint8_t* buffer, size_t data_len) {
    uint8_t padding_len = AES_BLOCKLEN - (data_len % AES_BLOCKLEN);
    if (padding_len == 0) { // If data_len is already a multiple of block size
        padding_len = AES_BLOCKLEN; // Add a full block of padding
    }
    
    // Fill the padding bytes with the padding length value
    for (size_t i = 0; i < padding_len; ++i) {
        buffer[data_len + i] = padding_len;
    }
    return data_len + padding_len;
}

// PKCS#7 Unpadding function
// Returns the original data length. Does NOT modify buffer content.
size_t pkcs7_unpad(const uint8_t* buffer, size_t padded_len) {
    if (padded_len == 0) return 0; // Handle empty buffer case
    uint8_t padding_len = buffer[padded_len - 1];

    // Basic validation of padding (more robust checks might be needed for full security)
    if (padding_len == 0 || padding_len > AES_BLOCKLEN || padding_len > padded_len) {
        return padded_len; // Or error handling
    }
    
    // Check if all padding bytes have the expected value
    for (size_t i = 0; i < padding_len; ++i) {
        if (buffer[padded_len - 1 - i] != padding_len) {
            return padded_len; // Or error handling
        }
    }

    return padded_len - padding_len;
}

void create_iv(uint8_t * buf, uint8_t len) {
    
    for (int k = 0; k < len; k++) {
        buf[k] = rand() % 256;
    }
}

bool aes_128_encrypt(const uint8_t *key, size_t keylen,
                     const uint8_t *inbuf, size_t in_buflen,
                     uint8_t *outbuf, size_t *out_buflen) {
    if (!key || !inbuf || !outbuf || !out_buflen) return false;
    if (keylen != 16) return false;

    // Generate IV
    uint8_t iv[AES_BLOCKLEN];
    get_random_bytes(iv, AES_BLOCKLEN);

    // Copy IV into start of output
    memcpy(outbuf, iv, AES_BLOCKLEN);

    // Copy plaintext into output (after IV) and pad
    memcpy(outbuf + AES_BLOCKLEN, inbuf, in_buflen);
    size_t padded_len = pkcs7_pad(outbuf + AES_BLOCKLEN, in_buflen);

    // Encrypt in place (after IV)
    AES_ctx_t ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_encrypt_buffer(&ctx, outbuf + AES_BLOCKLEN, padded_len);

    *out_buflen = AES_BLOCKLEN + padded_len;
    return true;
}

bool aes_128_decrypt(const uint8_t *key, size_t keylen,
                     const uint8_t *inbuf, size_t in_buflen,
                     uint8_t *outbuf, size_t *out_buflen) {
    if (!key || !inbuf || !outbuf || !out_buflen) return false;
    if (keylen != 16) return false;
    if (in_buflen < AES_BLOCKLEN) return false;

    // Extract IV
    uint8_t iv[AES_BLOCKLEN];
    memcpy(iv, inbuf, AES_BLOCKLEN);

    size_t encrypted_len = in_buflen - AES_BLOCKLEN;

    // Copy ciphertext into output buffer (to decrypt in-place)
    memcpy(outbuf, inbuf + AES_BLOCKLEN, encrypted_len);

    AES_ctx_t ctx;
    AES_init_ctx_iv(&ctx, key, iv);
    AES_CBC_decrypt_buffer(&ctx, outbuf, encrypted_len);

    size_t unpadded_len = pkcs7_unpad(outbuf, encrypted_len);
    if (unpadded_len > encrypted_len) {
        return false; // invalid padding
    }

    *out_buflen = unpadded_len;
    return true;
}
