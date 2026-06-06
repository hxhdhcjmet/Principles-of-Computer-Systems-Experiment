/*
 * Scheme 1: SM4 Default Loop (Baseline)
 *
 * This is the standard SM4 implementation using a 32-round for-loop.
 * It serves as the performance BASELINE for speedup comparisons.
 *
 * Key characteristics:
 *   - 32-round for-loop with branch at each iteration
 *   - Sequential XOR chain: X1 ^ X2 ^ X3 ^ rk[i]
 *   - Standard Feistel shift: X0=X1, X1=X2, X2=X3, X3=new
 *
 * Bugs fixed from original:
 *   1. L function: (in>>6) corrected to (in>>30) for ROL-2 decomposition
 *   2. Key expansion: now uses correct L' (not L) per GB/T 32907-2016
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Encryption — default loop (32 iterations with branch)
 * ========================================================================== */
static void sm4EncryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X[4];
    sm4LoadBlock(in, X);

    for (int i = 0; i < 32; i++) {
        uint32_t tmp   = X[1] ^ X[2] ^ X[3] ^ rk[i];
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X[0] ^ t;
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = newX0;
    }

    sm4StoreBlock(X, out);
}

/* ==========================================================================
 * Decryption — same structure as encryption, round keys reversed
 * ========================================================================== */
static void sm4DecryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X[4];
    sm4LoadBlock(in, X);

    for (int i = 31; i >= 0; i--) {
        uint32_t tmp   = X[1] ^ X[2] ^ X[3] ^ rk[i];
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X[0] ^ t;
        X[0] = X[1];
        X[1] = X[2];
        X[2] = X[3];
        X[3] = newX0;
    }

    sm4StoreBlock(X, out);
}

/* ==========================================================================
 * Correctness self-test — encrypts and decrypts the standard test vector
 * ========================================================================== */
static int verifyCorrectness(void)
{
    uint32_t rk[32];
    uint8_t cipher[16];
    uint8_t plain[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    sm4EncryptBlock(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] sm4_default_loop: encryption test vector mismatch\n");
        return 0;
    }

    /* Decryption — should recover original plaintext */
    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_default_loop: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_default_loop: test vector correct\n");
    return 1;
}

/* ==========================================================================
 * Benchmark harness — encrypts data_size bytes, repeats `repeat` times
 *
 * Returns elapsed time in MICROSECONDS.
 * Caller computes throughput = (data_size * repeat) / elapsed / 1e6 * MB/s
 * ========================================================================== */
uint64_t bench_default_encrypt(const uint32_t rk[32],
                               size_t data_size,
                               int repeat)
{
    uint8_t *data = (uint8_t *)malloc(data_size);
    uint8_t *out  = (uint8_t *)malloc(data_size);
    if (!data || !out) { free(data); free(out); return 0; }
    memset(data, 0xAA, data_size);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int r = 0; r < repeat; r++) {
        for (size_t i = 0; i < data_size; i += 16) {
            sm4EncryptBlock(rk, data + i, out + i);
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &end);

    free(data);
    free(out);

    uint64_t elapsed_us = (uint64_t)(end.tv_sec  - start.tv_sec)  * 1000000ULL
                        + (uint64_t)(end.tv_nsec - start.tv_nsec) / 1000ULL;
    return elapsed_us;
}

/* ==========================================================================
 * Standalone main — used for independent testing / debugging.
 * Skipped when compiled as part of the unified benchmark (SM4_AS_LIBRARY).
 * ========================================================================== */
#ifndef SM4_AS_LIBRARY
int main(void)
{
    uint32_t rk[32];
    sm4KeyExpand(SM4_TEST_KEY, rk);

    /* 1. Correctness check */
    if (!verifyCorrectness()) return 1;

    /* 2. Quick performance test */
    size_t data_size = 1024 * 1024;   /* 1 MB */
    int    repeat    = 10;

    uint64_t elapsed = bench_default_encrypt(rk, data_size, repeat);
    double total_bytes = (double)data_size * repeat;
    double throughput  = total_bytes / ((double)elapsed / 1e6) / (1024.0 * 1024.0);

    printf("SM4 Default Loop — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
