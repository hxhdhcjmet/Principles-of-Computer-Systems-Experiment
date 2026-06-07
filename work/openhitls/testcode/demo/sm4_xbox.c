/*
 * Scheme 4: SM4 XBox Table Lookup
 *
 * Precomputes a T-box: xbox[byte] = L(S(byte)) for all 256 byte values.
 * Each round replaces the full sm4T() computation (S-box + L transform)
 * with 4 table lookups + 3 rotations + 4 XORs.
 *
 * Optimization rationale:
 *   - Eliminates per-round S-box byte substitutions (4 × sm4Sbox)
 *   - Eliminates per-round L transform (4 shifts + 4 XORs)
 *   - 256 × 4 bytes = 1 KB table — fits comfortably in L1 data cache
 *
 * This is the baseline "XBox lookup" approach. See sm4_xbox_merged.c for
 * the 4-pre-shifted-table variant that further eliminates runtime rotations.
 *
 * Reference: GB/T 32907-2016; openHiTLS crypt_sm4.c T-box approach
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * T-box: xbox[byte] = L(SBOX[byte])
 *
 * SBOX[byte] is the S-box result treated as a uint32_t value in the LSB
 * position (bits 7:0).  L is then applied to produce a 32-bit T-box entry.
 *
 * This table is initialized once on first use.
 * ========================================================================== */
static uint32_t xbox[256];
static int xbox_initialized = 0;

static void initXbox(void)
{
    if (xbox_initialized) return;
    for (int i = 0; i < 256; i++) {
        xbox[i] = sm4L((uint32_t)sm4Sbox((uint8_t)i));
    }
    xbox_initialized = 1;
}

/* ==========================================================================
 * ROL — 32-bit rotate left
 * ========================================================================== */
static inline uint32_t ROL(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

/* ==========================================================================
 * Encryption — XBox table lookup with runtime rotations
 *
 * SM4 round function via T-box decomposition:
 *   T(X1⊕X2⊕X3⊕rk) =
 *     ROL(xbox[b0], 24) ⊕ ROL(xbox[b1], 16)
 *   ⊕ ROL(xbox[b2],  8) ⊕     xbox[b3]
 *
 * where b0..b3 are the 4 bytes of (X1⊕X2⊕X3⊕rk) from MSB to LSB.
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

    initXbox();

    for (int i = 0; i < 32; i++) {
        uint32_t t    = X1 ^ X2 ^ X3 ^ rk[i];
        uint8_t  b0   = (uint8_t)((t >> 24) & 0xFF);
        uint8_t  b1   = (uint8_t)((t >> 16) & 0xFF);
        uint8_t  b2   = (uint8_t)((t >>  8) & 0xFF);
        uint8_t  b3   = (uint8_t)( t        & 0xFF);

        uint32_t newX0 = X0 ^ ROL(xbox[b0], 24)
                            ^ ROL(xbox[b1], 16)
                            ^ ROL(xbox[b2],  8)
                            ^     xbox[b3];

        X0 = X1; X1 = X2; X2 = X3; X3 = newX0;
    }

    {
        uint32_t arr[4] = {X0, X1, X2, X3};
        sm4StoreBlock(arr, out);
    }
}

/* ==========================================================================
 * Decryption — same structure, reversed round-key order
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

    initXbox();

    for (int i = 31; i >= 0; i--) {
        uint32_t t    = X1 ^ X2 ^ X3 ^ rk[i];
        uint8_t  b0   = (uint8_t)((t >> 24) & 0xFF);
        uint8_t  b1   = (uint8_t)((t >> 16) & 0xFF);
        uint8_t  b2   = (uint8_t)((t >>  8) & 0xFF);
        uint8_t  b3   = (uint8_t)( t        & 0xFF);

        uint32_t newX0 = X0 ^ ROL(xbox[b0], 24)
                            ^ ROL(xbox[b1], 16)
                            ^ ROL(xbox[b2],  8)
                            ^     xbox[b3];

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
static int verifyCorrectness(void)
{
    uint32_t rk[32];
    uint8_t cipher[16];
    uint8_t plain[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    sm4EncryptBlock(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_xbox: test vector correct\n");
    return 1;
}

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_xbox_encrypt(const uint32_t rk[32],
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

    uint64_t elapsed = bench_xbox_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 XBox Table Lookup — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
