#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sm4_portable.h"

#ifndef DATA_LEN
#define DATA_LEN (16 * 1024)
#endif

#ifndef LOOP
#define LOOP 2000
#endif

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(void)
{
    static const uint8_t key[16] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10
    };
    static const uint8_t base_iv[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };

    uint8_t *plain = malloc(DATA_LEN);
    uint8_t *cipher = malloc(DATA_LEN);
    uint8_t *decrypted = malloc(DATA_LEN);
    uint8_t iv[SM4_BLOCK_SIZE];
    SM4_KEY ks;

    if (plain == NULL || cipher == NULL || decrypted == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    sm4_fill_pattern(plain, DATA_LEN);
    sm4_set_key(key, &ks);

    double start = now_seconds();
    for (int i = 0; i < LOOP; i++) {
        memcpy(iv, base_iv, SM4_BLOCK_SIZE);
        sm4_cbc_encrypt(plain, cipher, DATA_LEN, &ks, iv);
    }
    double elapsed = now_seconds() - start;

    memcpy(iv, base_iv, SM4_BLOCK_SIZE);
    sm4_cbc_decrypt(cipher, decrypted, DATA_LEN, &ks, iv);

    double total_bytes = (double)LOOP * (double)DATA_LEN;
    printf("version: original-byte-sbox\n");
    printf("data_len: %d bytes\n", DATA_LEN);
    printf("loop: %d\n", LOOP);
    printf("time: %.6f s\n", elapsed);
    printf("data: %.0f bytes\n", total_bytes);
    printf("throughput: %.2f MB/s\n", total_bytes / elapsed / 1000000.0);
    printf("throughput: %.2f MiB/s\n", total_bytes / elapsed / 1024.0 / 1024.0);
    printf("verify: %s\n", memcmp(plain, decrypted, DATA_LEN) == 0 ? "ok" : "failed");
    printf("cipher_checksum: %08x\n", sm4_checksum(cipher, DATA_LEN));

    free(plain);
    free(cipher);
    free(decrypted);
    return 0;
}
