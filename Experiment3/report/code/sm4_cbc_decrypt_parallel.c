#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sm4_portable.h"

#ifndef DATA_LEN
#define DATA_LEN (1024 * 1024)
#endif

#ifndef THREADS
#define THREADS 4
#endif

#ifndef LOOP
#define LOOP 100
#endif

typedef struct {
    const SM4_KEY *ks;
    const uint8_t *cipher;
    uint8_t *plain;
    const uint8_t *iv;
    size_t start_block;
    size_t end_block;
} decrypt_arg;

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void *decrypt_worker(void *ptr)
{
    decrypt_arg *arg = (decrypt_arg *)ptr;
    uint8_t block[SM4_BLOCK_SIZE];

    for (size_t b = arg->start_block; b < arg->end_block; b++) {
        const uint8_t *cur = arg->cipher + b * SM4_BLOCK_SIZE;
        const uint8_t *prev = (b == 0) ? arg->iv : cur - SM4_BLOCK_SIZE;
        sm4_decrypt_block(cur, block, arg->ks);
        sm4_xor_block(arg->plain + b * SM4_BLOCK_SIZE, block, prev);
    }

    return NULL;
}

static int cbc_decrypt_parallel(const uint8_t *cipher, uint8_t *plain, size_t len,
                                const SM4_KEY *ks, const uint8_t iv[SM4_BLOCK_SIZE])
{
    pthread_t threads[THREADS];
    decrypt_arg args[THREADS];
    size_t blocks = len / SM4_BLOCK_SIZE;
    size_t next_block = 0;

    if (len % SM4_BLOCK_SIZE != 0) {
        return 0;
    }

    for (int i = 0; i < THREADS; i++) {
        size_t left = blocks - next_block;
        size_t take = left / (size_t)(THREADS - i);
        args[i].ks = ks;
        args[i].cipher = cipher;
        args[i].plain = plain;
        args[i].iv = iv;
        args[i].start_block = next_block;
        args[i].end_block = next_block + take;
        next_block += take;
        if (pthread_create(&threads[i], NULL, decrypt_worker, &args[i]) != 0) {
            return 0;
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 1;
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
    memcpy(iv, base_iv, SM4_BLOCK_SIZE);
    sm4_cbc_encrypt(plain, cipher, DATA_LEN, &ks, iv);

    double start = now_seconds();
    for (int i = 0; i < LOOP; i++) {
        cbc_decrypt_parallel(cipher, decrypted, DATA_LEN, &ks, base_iv);
    }
    double elapsed = now_seconds() - start;

    double total_bytes = (double)LOOP * (double)DATA_LEN;
    printf("version: cbc-decrypt-parallel-byte-sbox\n");
    printf("threads: %d\n", THREADS);
    printf("data_len: %d bytes\n", DATA_LEN);
    printf("loop: %d\n", LOOP);
    printf("time: %.6f s\n", elapsed);
    printf("data: %.0f bytes\n", total_bytes);
    printf("throughput: %.2f MB/s\n", total_bytes / elapsed / 1000000.0);
    printf("throughput: %.2f MiB/s\n", total_bytes / elapsed / 1024.0 / 1024.0);
    printf("verify: %s\n", memcmp(plain, decrypted, DATA_LEN) == 0 ? "ok" : "failed");
    printf("plain_checksum: %08x\n", sm4_checksum(decrypted, DATA_LEN));

    free(plain);
    free(cipher);
    free(decrypted);
    return 0;
}
