/*
 * Scheme 7: SM4 4-Table Merged XBox + Zbb ISA Extension
 *
 * Identical algorithm to sm4_xbox_merged.c (4 pre-shifted T-box tables),
 * compiled with -march=rv64gc_zba_zbb_zbc_zbs.
 *
 * This is the theoretically fastest software-only SM4 configuration on
 * this platform: the algorithmic optimum (XBox merged) combined with the
 * ISA optimum (Zbb for rev8/rori).
 *
 * Zbb benefits on top of XBox merged:
 *   - rev8:   accelerates sm4LoadBlock/sm4StoreBlock big-endian conversion
 *   - rori:   accelerates ROL() calls during XBox table initialization
 *   - General: minor codegen improvements in addressing, bitfield ops
 *
 * Comparison target: sm4_xbox_merged (same algorithm, baseline ISA).
 * Gap between zbb_optimized and default_loop  = ISA gains alone.
 * Gap between xbox_zbb       and xbox_merged = ISA's remaining headroom
 *                                              after algorithmic optimization.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Four pre-shifted T-box tables (same as sm4_xbox_merged.c)
 * ========================================================================== */
static uint32_t XBOX0[256];
static uint32_t XBOX1[256];
static uint32_t XBOX2[256];
static uint32_t XBOX3[256];
static int xbox_zbb_initialized = 0;

static inline uint32_t ROL(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static void initXboxZbb(void)
{
    if (xbox_zbb_initialized) return;

    for (int i = 0; i < 256; i++) {
        XBOX0[i] = sm4L((uint32_t)sm4Sbox((uint8_t)i));
        XBOX1[i] = ROL(XBOX0[i],  8);
        XBOX2[i] = ROL(XBOX0[i], 16);
        XBOX3[i] = ROL(XBOX0[i], 24);
    }
    xbox_zbb_initialized = 1;
}

/* ==========================================================================
 * Encryption — 4 merged XBox tables (zero runtime shifts)
 *
 * The compiler, with Zbb, generates:
 *   - rev8 for loading/storing big-endian words
 *   - rori/ror for table initialization ROL() calls
 * The inner loop (table lookups + XOR) is already optimal in the
 * baseline version; Zbb mainly benefits the periphery.
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

    initXboxZbb();

    for (int i = 0; i < 32; i++) {
        uint32_t t    = X1 ^ X2 ^ X3 ^ rk[i];
        uint32_t newX0 = X0 ^ XBOX3[(t >> 24) & 0xFF]
                           ^ XBOX2[(t >> 16) & 0xFF]
                           ^ XBOX1[(t >>  8) & 0xFF]
                           ^ XBOX0[ t        & 0xFF];

        X0 = X1; X1 = X2; X2 = X3; X3 = newX0;
    }

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Decryption
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

    initXboxZbb();

    for (int i = 31; i >= 0; i--) {
        uint32_t t    = X1 ^ X2 ^ X3 ^ rk[i];
        uint32_t newX0 = X0 ^ XBOX3[(t >> 24) & 0xFF]
                           ^ XBOX2[(t >> 16) & 0xFF]
                           ^ XBOX1[(t >>  8) & 0xFF]
                           ^ XBOX0[ t        & 0xFF];

        X0 = X1; X1 = X2; X2 = X3; X3 = newX0;
    }

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Correctness self-test
 * ========================================================================== */
#ifndef SM4_AS_LIBRARY
static int verifyCorrectness(void)
{
    uint32_t rk[32];
    uint8_t cipher[16];
    uint8_t plain[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    sm4EncryptBlock(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox_zbb: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox_zbb: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_xbox_zbb: test vector correct\n");
    return 1;
}
#endif /* !SM4_AS_LIBRARY */

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_xbox_zbb_encrypt(const uint32_t rk[32],
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
 * Standalone main
 * ========================================================================== */
#ifndef SM4_AS_LIBRARY
int main(void)
{
    uint32_t rk[32];
    sm4KeyExpand(SM4_TEST_KEY, rk);

    if (!verifyCorrectness()) return 1;

    size_t data_size = 1024 * 1024;
    int    repeat    = 10;

    uint64_t elapsed = bench_xbox_zbb_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 XBox Merged + Zbb — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
