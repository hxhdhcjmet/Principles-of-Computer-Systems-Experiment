#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sm4_portable.h"

/*
 * SM4-CBC single-stream implementation accelerated with the RISC-V Zksed
 * scalar crypto extension when available.
 *
 * The core round and key-schedule functions are expressed directly in terms
 * of sm4ed / sm4ks. When compiled on non-Zksed hosts, exact C fallbacks are
 * used so the file remains buildable for development and review.
 */

#ifndef DATA_LEN
#define DATA_LEN (16 * 1024)
#endif

#ifndef LOOP
#define LOOP 2000
#endif

#if defined(__riscv) && defined(__riscv_zksed)
#define ZKSED_BACKEND "zksed-hw"
#else
#define ZKSED_BACKEND "zksed-emulated"
#endif

static inline uint32_t rotl32(uint32_t x, unsigned n)
{
    return (x << n) | (x >> (32 - n));
}

static inline uint32_t load_be32(const uint8_t *src)
{
    return ((uint32_t)src[0] << 24) |
           ((uint32_t)src[1] << 16) |
           ((uint32_t)src[2] << 8) |
           ((uint32_t)src[3]);
}

static inline void store_be32(uint32_t value, uint8_t *dst)
{
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

static uint32_t sm4ed_fallback(uint32_t rs1, uint32_t rs2, unsigned bs)
{
    unsigned shamt = (bs & 3u) * 8u;
    uint8_t sb_in = (uint8_t)((rs2 >> shamt) & 0xFFu);
    uint32_t x = sm4_sbox_byte(sb_in);
    uint32_t y = x ^ (x << 8) ^ (x << 2) ^ (x << 18) ^
                 ((x & 0x0000003Fu) << 26) ^
                 ((x & 0x000000C0u) << 10);
    uint32_t z = rotl32(y, shamt);
    return rs1 ^ z;
}

static uint32_t sm4ks_fallback(uint32_t rs1, uint32_t rs2, unsigned bs)
{
    unsigned shamt = (bs & 3u) * 8u;
    uint8_t sb_in = (uint8_t)((rs2 >> shamt) & 0xFFu);
    uint32_t x = sm4_sbox_byte(sb_in);
    uint32_t y = x ^ ((x & 0x00000007u) << 29) ^
                 ((x & 0x000000FEu) << 7) ^
                 ((x & 0x00000001u) << 23) ^
                 ((x & 0x000000F8u) << 13);
    uint32_t z = rotl32(y, shamt);
    return rs1 ^ z;
}

#if defined(__riscv) && defined(__riscv_zksed)
#define DEF_SM4ED(BS)                                                         \
    static inline uint32_t sm4ed_##BS(uint32_t rs1, uint32_t rs2)            \
    {                                                                         \
        uint32_t rd;                                                          \
        __asm__ volatile("sm4ed %0, %1, %2, " #BS                            \
                         : "=r"(rd)                                           \
                         : "r"(rs1), "r"(rs2));                               \
        return rd;                                                            \
    }
#define DEF_SM4KS(BS)                                                         \
    static inline uint32_t sm4ks_##BS(uint32_t rs1, uint32_t rs2)            \
    {                                                                         \
        uint32_t rd;                                                          \
        __asm__ volatile("sm4ks %0, %1, %2, " #BS                            \
                         : "=r"(rd)                                           \
                         : "r"(rs1), "r"(rs2));                               \
        return rd;                                                            \
    }
#else
#define DEF_SM4ED(BS)                                                         \
    static inline uint32_t sm4ed_##BS(uint32_t rs1, uint32_t rs2)            \
    {                                                                         \
        return sm4ed_fallback(rs1, rs2, BS);                                  \
    }
#define DEF_SM4KS(BS)                                                         \
    static inline uint32_t sm4ks_##BS(uint32_t rs1, uint32_t rs2)            \
    {                                                                         \
        return sm4ks_fallback(rs1, rs2, BS);                                  \
    }
#endif

DEF_SM4ED(0)
DEF_SM4ED(1)
DEF_SM4ED(2)
DEF_SM4ED(3)
DEF_SM4KS(0)
DEF_SM4KS(1)
DEF_SM4KS(2)
DEF_SM4KS(3)

static void sm4_set_key_zksed(const uint8_t key[16], uint32_t rk[32])
{
    uint32_t k[4];

    for (int i = 0; i < 4; i++) {
        k[i] = load_be32(key + i * 4) ^ SM4_FK[i];
    }

    for (int i = 0; i < 32; i++) {
        uint32_t t = k[1] ^ k[2] ^ k[3] ^ SM4_CK[i];
        uint32_t next = k[0];
        next = sm4ks_0(next, t);
        next = sm4ks_1(next, t);
        next = sm4ks_2(next, t);
        next = sm4ks_3(next, t);
        rk[i] = next;
        k[0] = k[1];
        k[1] = k[2];
        k[2] = k[3];
        k[3] = next;
    }
}

static void sm4_encrypt_block_zksed(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32])
{
    uint32_t x0 = load_be32(in);
    uint32_t x1 = load_be32(in + 4);
    uint32_t x2 = load_be32(in + 8);
    uint32_t x3 = load_be32(in + 12);

    for (int i = 0; i < 32; i++) {
        uint32_t t = x1 ^ x2 ^ x3 ^ rk[i];
        uint32_t next = x0;
        next = sm4ed_0(next, t);
        next = sm4ed_1(next, t);
        next = sm4ed_2(next, t);
        next = sm4ed_3(next, t);
        x0 = x1;
        x1 = x2;
        x2 = x3;
        x3 = next;
    }

    store_be32(x3, out);
    store_be32(x2, out + 4);
    store_be32(x1, out + 8);
    store_be32(x0, out + 12);
}

static void sm4_cbc_encrypt_zksed(const uint8_t *in, uint8_t *out, size_t len,
                                  const uint32_t rk[32], uint8_t iv[SM4_BLOCK_SIZE])
{
    uint8_t block[SM4_BLOCK_SIZE];

    for (size_t i = 0; i < len; i += SM4_BLOCK_SIZE) {
        for (int j = 0; j < SM4_BLOCK_SIZE; j++) {
            block[j] = in[i + j] ^ iv[j];
        }
        sm4_encrypt_block_zksed(block, out + i, rk);
        memcpy(iv, out + i, SM4_BLOCK_SIZE);
    }
}

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

    uint32_t rk[32];
    uint8_t *plain = malloc(DATA_LEN);
    uint8_t *cipher = malloc(DATA_LEN);
    uint8_t iv[SM4_BLOCK_SIZE];

    if (plain == NULL || cipher == NULL) {
        fprintf(stderr, "allocation failed\n");
        free(plain);
        free(cipher);
        return 1;
    }

    sm4_fill_pattern(plain, DATA_LEN);
    sm4_set_key_zksed(key, rk);

    {
        double start = now_seconds();
        for (int i = 0; i < LOOP; i++) {
            memcpy(iv, base_iv, SM4_BLOCK_SIZE);
            sm4_cbc_encrypt_zksed(plain, cipher, DATA_LEN, rk, iv);
        }
        double elapsed = now_seconds() - start;
        double total_data = (double)LOOP * (double)DATA_LEN;

        printf("version: single-stream-%s\n", ZKSED_BACKEND);
        printf("threads: 1\n");
        printf("data_len: %d bytes\n", DATA_LEN);
        printf("loop: %d\n", LOOP);
        printf("time: %.6f s\n", elapsed);
        printf("data: %.0f bytes\n", total_data);
        printf("throughput: %.2f MB/s\n", total_data / elapsed / 1000000.0);
        printf("throughput: %.2f MiB/s\n", total_data / elapsed / 1024.0 / 1024.0);
        printf("cipher_checksum: %08x\n", sm4_checksum(cipher, DATA_LEN));
    }

    free(plain);
    free(cipher);
    return 0;
}
