/*
 * Scheme 3: SM4 Reduced Instruction Dependency
 *
 * Optimization: break the sequential XOR dependency chain in the round function
 * to expose instruction-level parallelism (ILP).
 *
 * Original (sequential XOR chain, 3 dependent XORs):
 *     tmp = X[1] ^ X[2] ^ X[3] ^ rk[i];
 *
 * Optimized (2 independent XORs + 1 merge):
 *     tmp1 = X[1] ^ X[2];       // independent
 *     tmp2 = X[3] ^ rk[i];      // independent (no dependency on tmp1)
 *     tmp  = tmp1 ^ tmp2;       // merge
 *
 * Additionally, intermediate writes to X[] use temporaries to avoid forcing
 * the compiler to emit store-then-reload sequences:
 *     X[0] = X[1];  →  temp statement
 *     X[1] = X[2];
 *     X[2] = X[3];
 *     X[3] = newX0;
 *
 * Reason: simple in-order RISC-V cores cannot pipeline dependent instructions
 * effectively. Breaking the dependency allows the CPU to issue tmp1 and tmp2
 * in the same cycle (or close cycles), hiding latency.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Encryption — reduced dependency version
 * ========================================================================== */
static void sm4EncryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X0, X1, X2, X3;
    {
        uint32_t arr[4];
        sm4LoadBlock(in, arr);
        X0 = arr[0]; X1 = arr[1]; X2 = arr[2]; X3 = arr[3];
    }

    for (int i = 0; i < 32; i++) {
        /* Break the XOR chain: X1^X2 and X3^rk are independent */
        uint32_t tmp1  = X1 ^ X2;
        uint32_t tmp2  = X3 ^ rk[i];
        uint32_t tmp   = tmp1 ^ tmp2;
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X0 ^ t;

        /* Feistel rotation with temporaries */
        uint32_t prevX1 = X1;
        uint32_t prevX2 = X2;
        X0 = prevX1;
        X1 = prevX2;
        X2 = X3;
        X3 = newX0;
    }

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Decryption — reduced dependency with reversed round keys
 * ========================================================================== */
static void sm4DecryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X0, X1, X2, X3;
    {
        uint32_t arr[4];
        sm4LoadBlock(in, arr);
        X0 = arr[0]; X1 = arr[1]; X2 = arr[2]; X3 = arr[3];
    }

    for (int i = 31; i >= 0; i--) {
        uint32_t tmp1  = X1 ^ X2;
        uint32_t tmp2  = X3 ^ rk[i];
        uint32_t tmp   = tmp1 ^ tmp2;
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X0 ^ t;

        uint32_t prevX1 = X1;
        uint32_t prevX2 = X2;
        X0 = prevX1;
        X1 = prevX2;
        X2 = X3;
        X3 = newX0;
    }

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Correctness self-test
 * ========================================================================== */
static int verifyCorrectness(void)
{
    uint32_t rk[32];
    uint8_t cipher[16];
    uint8_t plain[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    sm4EncryptBlock(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] sm4_reduced_dep: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_reduced_dep: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_reduced_dep: test vector correct\n");
    return 1;
}

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_reduceddep_encrypt(const uint32_t rk[32],
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

    int64_t sec_diff  = end.tv_sec  - start.tv_sec;
    int64_t nsec_diff = end.tv_nsec - start.tv_nsec;
    if (nsec_diff < 0) { sec_diff -= 1; nsec_diff += 1000000000L; }
    return (uint64_t)sec_diff * 1000000ULL + (uint64_t)nsec_diff / 1000ULL;
}

/* ==========================================================================
 * Standalone main — skipped when SM4_AS_LIBRARY is defined
 * ========================================================================== */
#ifndef SM4_AS_LIBRARY
int main(void)
{
    uint32_t rk[32];
    sm4KeyExpand(SM4_TEST_KEY, rk);

    if (!verifyCorrectness()) return 1;

    size_t data_size = 1024 * 1024;
    int    repeat    = 10;

    uint64_t elapsed = bench_reduceddep_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 Reduced Dependency — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
