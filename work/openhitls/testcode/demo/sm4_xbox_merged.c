/*
 * Scheme 5: SM4 XBox Merged (4 pre-shifted T-box tables)
 *
 * Extends the single T-box approach by precomputing four rotated copies
 * of the T-box, eliminating ALL runtime shift/rotate instructions from
 * the round function.
 *
 * Tables (1 KB each, 4 KB total):
 *   XBOX0[i] = L(SBOX[i])           — LSB position
 *   XBOX1[i] = XBOX0[i] <<< 8       — pre-shifted by 1 byte
 *   XBOX2[i] = XBOX0[i] <<< 16      — pre-shifted by 2 bytes
 *   XBOX3[i] = XBOX0[i] <<< 24      — pre-shifted by 3 bytes (MSB position)
 *
 * Round function (4 lookups + 4 XORs, zero shifts):
 *   X0 ^= XBOX3[b0] ^ XBOX2[b1] ^ XBOX1[b2] ^ XBOX0[b3]
 *
 * where b0..b3 are bytes of (X1⊕X2⊕X3⊕rk) from MSB to LSB.
 *
 * This is the approach used by the official openHiTLS SM4 implementation
 * (crypt_sm4.c) and represents the most efficient software-only SM4
 * on platforms without hardware AES/SM4 acceleration.
 *
 * Optimization rationale:
 *   - Zero runtime rotations — all shifting is precomputed
 *   - 4 independent table lookups per round → ILP-friendly
 *   - 4 KB total table size — fits in L1 data cache on most RISC-V cores
 *
 * Trade-off: 4× table memory vs. sm4_xbox.c (4 KB vs 1 KB)
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Four pre-shifted T-box tables (4 × 256 × 4 bytes = 4 KB)
 *
 * Initialized once on first use.  XBOX0 = base T-box; XBOX1/2/3 are
 * rotated-left copies (XBOX_k[i] = ROL(XBOX0[i], 8*k)).
 * ========================================================================== */
static uint32_t XBOX0[256];
static uint32_t XBOX1[256];
static uint32_t XBOX2[256];
static uint32_t XBOX3[256];
static int xbox_merged_initialized = 0;

static inline uint32_t ROL(uint32_t x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static void initXboxMerged(void)
{
    if (xbox_merged_initialized) return;

    for (int i = 0; i < 256; i++) {
        XBOX0[i] = sm4L((uint32_t)sm4Sbox((uint8_t)i));
        XBOX1[i] = ROL(XBOX0[i],  8);
        XBOX2[i] = ROL(XBOX0[i], 16);
        XBOX3[i] = ROL(XBOX0[i], 24);
    }
    xbox_merged_initialized = 1;
}

/* ==========================================================================
 * Encryption — 4 merged XBox tables (zero runtime shifts)
 *
 * Equivalent to T(X1⊕X2⊕X3⊕rk), decomposed as:
 *   X0 ^= XBOX3[(t>>24)&0xFF] ^ XBOX2[(t>>16)&0xFF]
 *       ^ XBOX1[(t>> 8)&0xFF] ^ XBOX0[ t     &0xFF]
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

    initXboxMerged();

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
 * Decryption — same merged XBox with reversed round-key order
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

    initXboxMerged();

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
static int verifyCorrectness(void)
{
    uint32_t rk[32];
    uint8_t cipher[16];
    uint8_t plain[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    sm4EncryptBlock(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox_merged: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_xbox_merged: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_xbox_merged: test vector correct\n");
    return 1;
}

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_xbox_merged_encrypt(const uint32_t rk[32],
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

    uint64_t elapsed = bench_xbox_merged_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 XBox Merged (4 tables) — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
