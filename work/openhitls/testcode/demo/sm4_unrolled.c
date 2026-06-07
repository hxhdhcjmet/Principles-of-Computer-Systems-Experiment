/*
 * Scheme 2: SM4 Loop Unrolling
 *
 * The 32-round for-loop is fully unrolled into 32 sequential macro expansions.
 *
 * Optimization rationale:
 *   - Eliminates loop counter increment & comparison (32 branches removed)
 *   - Eliminates array indexing overhead on X[] — uses scalar registers
 *   - Enables better compiler instruction scheduling across round boundaries
 *   - Particularly beneficial on in-order RISC-V cores with short pipelines
 *
 * Trade-off: larger code size (~8× expansion per round)
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Encryption — fully unrolled 32 rounds
 *
 * Uses a macro to avoid copy-paste errors; the compiler expands each
 * ROUND(N) inline with a compile-time constant rk index.
 * ========================================================================== */
static void sm4EncryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X0, X1, X2, X3;
    uint32_t tmp, t, newX;

    /* Manual load to scalars for unrolled access */
    {
        uint32_t arr[4];
        sm4LoadBlock(in, arr);
        X0 = arr[0]; X1 = arr[1]; X2 = arr[2]; X3 = arr[3];
    }

#define SM4_UNROLLED_ROUND(N) do {         \
    tmp  = X1 ^ X2 ^ X3 ^ rk[N];           \
    t    = sm4T(tmp);                      \
    newX = X0 ^ t;                         \
    X0 = X1; X1 = X2; X2 = X3; X3 = newX;  \
} while(0)

    SM4_UNROLLED_ROUND(0);
    SM4_UNROLLED_ROUND(1);
    SM4_UNROLLED_ROUND(2);
    SM4_UNROLLED_ROUND(3);
    SM4_UNROLLED_ROUND(4);
    SM4_UNROLLED_ROUND(5);
    SM4_UNROLLED_ROUND(6);
    SM4_UNROLLED_ROUND(7);
    SM4_UNROLLED_ROUND(8);
    SM4_UNROLLED_ROUND(9);
    SM4_UNROLLED_ROUND(10);
    SM4_UNROLLED_ROUND(11);
    SM4_UNROLLED_ROUND(12);
    SM4_UNROLLED_ROUND(13);
    SM4_UNROLLED_ROUND(14);
    SM4_UNROLLED_ROUND(15);
    SM4_UNROLLED_ROUND(16);
    SM4_UNROLLED_ROUND(17);
    SM4_UNROLLED_ROUND(18);
    SM4_UNROLLED_ROUND(19);
    SM4_UNROLLED_ROUND(20);
    SM4_UNROLLED_ROUND(21);
    SM4_UNROLLED_ROUND(22);
    SM4_UNROLLED_ROUND(23);
    SM4_UNROLLED_ROUND(24);
    SM4_UNROLLED_ROUND(25);
    SM4_UNROLLED_ROUND(26);
    SM4_UNROLLED_ROUND(27);
    SM4_UNROLLED_ROUND(28);
    SM4_UNROLLED_ROUND(29);
    SM4_UNROLLED_ROUND(30);
    SM4_UNROLLED_ROUND(31);

#undef SM4_UNROLLED_ROUND

    /* Reverse transform & store */
    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Decryption — unrolled with reversed round-key order
 * ========================================================================== */
static void sm4DecryptBlock(const uint32_t rk[32],
                            const uint8_t in[16],
                            uint8_t out[16])
{
    uint32_t X0, X1, X2, X3;
    uint32_t tmp, t, newX;

    {
        uint32_t arr[4];
        sm4LoadBlock(in, arr);
        X0 = arr[0]; X1 = arr[1]; X2 = arr[2]; X3 = arr[3];
    }

#define SM4_UNROLLED_DEC_ROUND(N) do {     \
    tmp  = X1 ^ X2 ^ X3 ^ rk[N];            \
    t    = sm4T(tmp);                       \
    newX = X0 ^ t;                          \
    X0 = X1; X1 = X2; X2 = X3; X3 = newX;   \
} while(0)

    SM4_UNROLLED_DEC_ROUND(31);
    SM4_UNROLLED_DEC_ROUND(30);
    SM4_UNROLLED_DEC_ROUND(29);
    SM4_UNROLLED_DEC_ROUND(28);
    SM4_UNROLLED_DEC_ROUND(27);
    SM4_UNROLLED_DEC_ROUND(26);
    SM4_UNROLLED_DEC_ROUND(25);
    SM4_UNROLLED_DEC_ROUND(24);
    SM4_UNROLLED_DEC_ROUND(23);
    SM4_UNROLLED_DEC_ROUND(22);
    SM4_UNROLLED_DEC_ROUND(21);
    SM4_UNROLLED_DEC_ROUND(20);
    SM4_UNROLLED_DEC_ROUND(19);
    SM4_UNROLLED_DEC_ROUND(18);
    SM4_UNROLLED_DEC_ROUND(17);
    SM4_UNROLLED_DEC_ROUND(16);
    SM4_UNROLLED_DEC_ROUND(15);
    SM4_UNROLLED_DEC_ROUND(14);
    SM4_UNROLLED_DEC_ROUND(13);
    SM4_UNROLLED_DEC_ROUND(12);
    SM4_UNROLLED_DEC_ROUND(11);
    SM4_UNROLLED_DEC_ROUND(10);
    SM4_UNROLLED_DEC_ROUND(9);
    SM4_UNROLLED_DEC_ROUND(8);
    SM4_UNROLLED_DEC_ROUND(7);
    SM4_UNROLLED_DEC_ROUND(6);
    SM4_UNROLLED_DEC_ROUND(5);
    SM4_UNROLLED_DEC_ROUND(4);
    SM4_UNROLLED_DEC_ROUND(3);
    SM4_UNROLLED_DEC_ROUND(2);
    SM4_UNROLLED_DEC_ROUND(1);
    SM4_UNROLLED_DEC_ROUND(0);

#undef SM4_UNROLLED_DEC_ROUND

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

#ifndef SM4_AS_LIBRARY
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
        printf("[FAIL] sm4_unrolled: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_unrolled: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_unrolled: test vector correct\n");
    return 1;
}
#endif /* !SM4_AS_LIBRARY */

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_unrolled_encrypt(const uint32_t rk[32],
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

    uint64_t elapsed = bench_unrolled_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 Loop Unrolled — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
