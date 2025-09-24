#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "encryption.h"

// Example 128-bit key (16 bytes)
static const uint8_t test_key[16] = {
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
};

void test_encrypt_decrypt(const char *message) {
    uint8_t ciphertext[255];    // max LoRa payload
    uint8_t decrypted[223];     // max plaintext

    size_t ciphertext_len = 0;
    size_t decrypted_len = 0;

    size_t msg_len = strlen(message);

    printf("Original (%zu bytes): %s\n", msg_len, message);

    // Encrypt
    bool enc_ok = aes_128_encrypt(test_key, sizeof(test_key),
                                  (const uint8_t*)message, msg_len,
                                  ciphertext, &ciphertext_len);

    if (!enc_ok) {
        printf("Encryption failed!\n");
        return;
    }

    printf("Ciphertext length: %zu bytes\n", ciphertext_len);

    // Decrypt
    bool dec_ok = aes_128_decrypt(test_key, sizeof(test_key),
                                  ciphertext, ciphertext_len,
                                  decrypted, &decrypted_len);

    if (!dec_ok) {
        printf("Decryption failed!\n");
        return;
    }

    // Null-terminate for printing (safe because decrypted_len ≤ 223)
    decrypted[decrypted_len] = '\0';

    printf("Decrypted (%zu bytes): %s\n", decrypted_len, decrypted);

    // Compare
    if ((decrypted_len == msg_len) &&
        (memcmp(message, decrypted, msg_len) == 0)) {
        printf("Round-trip success!\n\n");
    } else {
        printf("Round-trip mismatch!\n\n");
    }
}

int main(void) {
    test_encrypt_decrypt("Hello, LoRa world!");
    test_encrypt_decrypt("1234567890ABCDEF");   // exactly one block (16 B)
    test_encrypt_decrypt("This is a longer test message that should still fit inside the 223-byte plaintext limit...");

    // Edge case: maximum size (223 bytes)
    char maxbuf[224];
    for (int i = 0; i < 223; i++) {
        maxbuf[i] = 'A' + (i % 26);
    }
    maxbuf[223] = '\0';
    test_encrypt_decrypt(maxbuf);

    return 0;
}
