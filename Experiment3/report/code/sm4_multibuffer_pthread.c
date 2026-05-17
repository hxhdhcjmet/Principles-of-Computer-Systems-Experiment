#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sm4_portable.h"

#ifndef DATA_LEN
#define DATA_LEN (16 * 1024)
#endif

#ifndef LOOP
#define LOOP 1000
#endif

#ifndef THREADS
#define THREADS 4
#endif

#ifdef USE_TABLE
#define VARIANT_NAME "multi-buffer-pthread+t-table"
#else
#define VARIANT_NAME "multi-buffer-pthread+byte-sbox"
#endif

typedef struct {
    int id;
    const SM4_KEY *ks;
    uint8_t *plain;
    uint8_t *cipher;
    uint32_t checksum;
} worker_arg;

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static void *encrypt_worker(void *ptr)
{
    worker_arg *arg = (worker_arg *)ptr;
    uint8_t base_iv[SM4_BLOCK_SIZE] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };
    uint8_t iv[SM4_BLOCK_SIZE];

    base_iv[15] ^= (uint8_t)arg->id;
    for (int i = 0; i < LOOP; i++) {
        memcpy(iv, base_iv, SM4_BLOCK_SIZE);
        sm4_cbc_encrypt(arg->plain, arg->cipher, DATA_LEN, arg->ks, iv);
    }

    arg->checksum = sm4_checksum(arg->cipher, DATA_LEN);
    return NULL;
}

int main(void)
{
    static const uint8_t key[16] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10
    };
    pthread_t threads[THREADS];
    worker_arg args[THREADS];
    SM4_KEY ks;
    uint8_t *plain = malloc((size_t)THREADS * DATA_LEN);
    uint8_t *cipher = malloc((size_t)THREADS * DATA_LEN);

    if (plain == NULL || cipher == NULL) {
        fprintf(stderr, "allocation failed\n");
        return 1;
    }

    sm4_fill_pattern(plain, (size_t)THREADS * DATA_LEN);
    sm4_set_key(key, &ks);

    double start = now_seconds();
    for (int i = 0; i < THREADS; i++) {
        args[i].id = i;
        args[i].ks = &ks;
        args[i].plain = plain + (size_t)i * DATA_LEN;
        args[i].cipher = cipher + (size_t)i * DATA_LEN;
        args[i].checksum = 0;
        if (pthread_create(&threads[i], NULL, encrypt_worker, &args[i]) != 0) {
            fprintf(stderr, "pthread_create failed\n");
            return 1;
        }
    }

    for (int i = 0; i < THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    double elapsed = now_seconds() - start;

    uint32_t checksum = 0;
    for (int i = 0; i < THREADS; i++) {
        checksum ^= args[i].checksum;
    }

    double total_bytes = (double)THREADS * (double)LOOP * (double)DATA_LEN;
    printf("version: %s\n", VARIANT_NAME);
    printf("threads: %d\n", THREADS);
    printf("data_len_per_thread: %d bytes\n", DATA_LEN);
    printf("loop_per_thread: %d\n", LOOP);
    printf("time: %.6f s\n", elapsed);
    printf("data: %.0f bytes\n", total_bytes);
    printf("throughput: %.2f MB/s\n", total_bytes / elapsed / 1000000.0);
    printf("throughput: %.2f MiB/s\n", total_bytes / elapsed / 1024.0 / 1024.0);
    printf("cipher_checksum: %08x\n", checksum);

    free(plain);
    free(cipher);
    return 0;
}
