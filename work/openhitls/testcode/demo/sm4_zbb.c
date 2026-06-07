/*
 * Scheme 6: SM4 Default Loop + Zbb ISA Extension
 *
 * Identical algorithm to sm4_default_loop.c (32-round for-loop),
 * but compiled with -march=rv64gc_zba_zbb_zbc_zbs to enable the
 * RISC-V Bit-Manipulation extension.
 *
 * Zbb benefits for SM4:
 *   - rev8:    single-instruction 32-bit byte reversal (bswap32)
 *              replaces 4-shift+mask+OR for big-endian ↔ little-endian
 *   - rori:    single-instruction rotate-right-immediate
 *              replaces (x<<n)|(x>>(32-n)) for L/L' rotations
 *   - andn:    a & ~b in one instruction, useful in bit-clear patterns
 *
 * Comparison target: sm4_default_loop (same algorithm, baseline ISA).
 * This scheme isolates pure ISA-level gains without algorithmic changes.
 *
 * Reference: RISC-V Bit-Manipulation ISA Extension v1.0.0
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include "sm4_common.h"

/* ==========================================================================
 * Encryption — identical logic to sm4_default_loop.c
 *
 * The Zbb optimization comes from the compiler, not manual rewrite:
 * sm4_common.h static inline functions (sm4T, sm4L, sm4S, sm4LoadBlock,
 * sm4StoreBlock, etc.) are compiled with Zbb flags and the compiler
 * automatically selects rev8/rori/andn where applicable.
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
 * Decryption
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
        printf("[FAIL] sm4_zbb: encryption test vector mismatch\n");
        return 0;
    }

    sm4DecryptBlock(rk, cipher, plain);
    if (memcmp(plain, SM4_TEST_PLAINTEXT, 16) != 0) {
        printf("[FAIL] sm4_zbb: decryption mismatch\n");
        return 0;
    }

    printf("[PASS] sm4_zbb: test vector correct\n");
    return 1;
}
#endif /* !SM4_AS_LIBRARY */

/* ==========================================================================
 * Benchmark harness
 * ========================================================================== */
uint64_t bench_zbb_encrypt(const uint32_t rk[32],
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

    uint64_t elapsed = bench_zbb_encrypt(rk, data_size, repeat);
    double throughput = (double)(data_size * repeat)
                      / ((double)elapsed / 1e6)
                      / (1024.0 * 1024.0);

    printf("SM4 Default Loop + Zbb — %zu bytes × %d repeats\n", data_size, repeat);
    printf("  Elapsed  : %lu μs\n", elapsed);
    printf("  Throughput: %.3f MB/s\n", throughput);

    return 0;
}
#endif /* !SM4_AS_LIBRARY */
