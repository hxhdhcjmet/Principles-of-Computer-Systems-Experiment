#ifndef SM4_PORTABLE_H
#define SM4_PORTABLE_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SCHEDULE 32

#if defined(__GNUC__) || defined(__clang__)
#define SM4_UNUSED __attribute__((unused))
#else
#define SM4_UNUSED
#endif

typedef struct {
    uint32_t rk[SM4_KEY_SCHEDULE];
} SM4_KEY;

static const uint32_t SM4_FK[4] = {
    0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
};

static const uint32_t SM4_CK[32] = {
    0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
    0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
    0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
    0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
    0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
    0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
    0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
    0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
};

static const uint8_t SM4_S[256] = {
    0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2,
    0x28, 0xFB, 0x2C, 0x05, 0x2B, 0x67, 0x9A, 0x76, 0x2A, 0xBE, 0x04, 0xC3,
    0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99, 0x9C, 0x42, 0x50, 0xF4,
    0x91, 0xEF, 0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62,
    0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80, 0xDF, 0x94, 0xFA,
    0x75, 0x8F, 0x3F, 0xA6, 0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA,
    0x83, 0x59, 0x3C, 0x19, 0xE6, 0x85, 0x4F, 0xA8, 0x68, 0x6B, 0x81, 0xB2,
    0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D, 0x35,
    0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B,
    0x01, 0x21, 0x78, 0x87, 0xD4, 0x00, 0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52,
    0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E, 0xEA, 0xBF, 0x8A, 0xD2,
    0x40, 0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1,
    0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55, 0xAD, 0x93, 0x32, 0x30,
    0xF5, 0x8C, 0xB1, 0xE3, 0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60,
    0xC0, 0x29, 0x23, 0xAB, 0x0D, 0x53, 0x4E, 0x6F, 0xD5, 0xDB, 0x37, 0x45,
    0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C, 0x5B, 0x51,
    0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41,
    0x1F, 0x10, 0x5A, 0xD8, 0x0A, 0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD,
    0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0, 0x89, 0x69, 0x97, 0x4A,
    0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84,
    0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D, 0x20, 0x79, 0xEE, 0x5F, 0x3E,
    0xD7, 0xCB, 0x39, 0x48
};

#ifdef SM4_USE_T_TABLE
static uint32_t SM4_T0[256];
static uint32_t SM4_T1[256];
static uint32_t SM4_T2[256];
static uint32_t SM4_T3[256];
static int SM4_TABLE_READY = 0;
#endif

static inline uint32_t sm4_rotl(uint32_t a, uint8_t n)
{
    return (a << n) | (a >> (32 - n));
}

static inline uint32_t sm4_load_u32_be(const uint8_t *b, uint32_t n)
{
    return ((uint32_t)b[4 * n] << 24) |
           ((uint32_t)b[4 * n + 1] << 16) |
           ((uint32_t)b[4 * n + 2] << 8) |
           ((uint32_t)b[4 * n + 3]);
}

static inline void sm4_store_u32_be(uint32_t v, uint8_t *b)
{
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)v;
}

static inline uint32_t sm4_sbox_word(uint32_t x)
{
    return ((uint32_t)SM4_S[(uint8_t)(x >> 24)] << 24) |
           ((uint32_t)SM4_S[(uint8_t)(x >> 16)] << 16) |
           ((uint32_t)SM4_S[(uint8_t)(x >> 8)] << 8) |
           ((uint32_t)SM4_S[(uint8_t)x]);
}

static inline uint8_t sm4_sbox_byte(uint8_t x)
{
    return SM4_S[x];
}

static inline uint32_t sm4_linear(uint32_t x)
{
    return x ^ sm4_rotl(x, 2) ^ sm4_rotl(x, 10) ^ sm4_rotl(x, 18) ^ sm4_rotl(x, 24);
}

static inline uint32_t sm4_key_linear(uint32_t x)
{
    return x ^ sm4_rotl(x, 13) ^ sm4_rotl(x, 23);
}

#ifdef SM4_USE_T_TABLE
static void sm4_prepare_tables(void)
{
    if (SM4_TABLE_READY) {
        return;
    }

    for (int i = 0; i < 256; i++) {
        uint32_t s = (uint32_t)SM4_S[i];
        SM4_T0[i] = sm4_linear(s << 24);
        SM4_T1[i] = sm4_linear(s << 16);
        SM4_T2[i] = sm4_linear(s << 8);
        SM4_T3[i] = sm4_linear(s);
    }
    SM4_TABLE_READY = 1;
}
#else
static void sm4_prepare_tables(void)
{
}
#endif

static inline uint32_t sm4_round_t(uint32_t x)
{
#ifdef SM4_USE_T_TABLE
    return SM4_T0[(uint8_t)(x >> 24)] ^
           SM4_T1[(uint8_t)(x >> 16)] ^
           SM4_T2[(uint8_t)(x >> 8)] ^
           SM4_T3[(uint8_t)x];
#else
    return sm4_linear(sm4_sbox_word(x));
#endif
}

static inline uint32_t sm4_key_sub(uint32_t x)
{
    return sm4_key_linear(sm4_sbox_word(x));
}

static int SM4_UNUSED sm4_set_key(const uint8_t *key, SM4_KEY *ks)
{
    uint32_t k[4];
    sm4_prepare_tables();

    k[0] = sm4_load_u32_be(key, 0) ^ SM4_FK[0];
    k[1] = sm4_load_u32_be(key, 1) ^ SM4_FK[1];
    k[2] = sm4_load_u32_be(key, 2) ^ SM4_FK[2];
    k[3] = sm4_load_u32_be(key, 3) ^ SM4_FK[3];

    for (int i = 0; i < SM4_KEY_SCHEDULE; i += 4) {
        k[0] ^= sm4_key_sub(k[1] ^ k[2] ^ k[3] ^ SM4_CK[i]);
        k[1] ^= sm4_key_sub(k[2] ^ k[3] ^ k[0] ^ SM4_CK[i + 1]);
        k[2] ^= sm4_key_sub(k[3] ^ k[0] ^ k[1] ^ SM4_CK[i + 2]);
        k[3] ^= sm4_key_sub(k[0] ^ k[1] ^ k[2] ^ SM4_CK[i + 3]);
        ks->rk[i] = k[0];
        ks->rk[i + 1] = k[1];
        ks->rk[i + 2] = k[2];
        ks->rk[i + 3] = k[3];
    }

    return 1;
}

#define SM4_RNDS(k0, k1, k2, k3)                  \
    do {                                          \
        b0 ^= sm4_round_t(b1 ^ b2 ^ b3 ^ rk[k0]); \
        b1 ^= sm4_round_t(b0 ^ b2 ^ b3 ^ rk[k1]); \
        b2 ^= sm4_round_t(b0 ^ b1 ^ b3 ^ rk[k2]); \
        b3 ^= sm4_round_t(b0 ^ b1 ^ b2 ^ rk[k3]); \
    } while (0)

static void sm4_crypt_block(const uint8_t *in, uint8_t *out,
                            const SM4_KEY *ks, int decrypt)
{
    uint32_t b0 = sm4_load_u32_be(in, 0);
    uint32_t b1 = sm4_load_u32_be(in, 1);
    uint32_t b2 = sm4_load_u32_be(in, 2);
    uint32_t b3 = sm4_load_u32_be(in, 3);
    uint32_t rk[SM4_KEY_SCHEDULE];

    if (decrypt) {
        for (int i = 0; i < SM4_KEY_SCHEDULE; i++) {
            rk[i] = ks->rk[SM4_KEY_SCHEDULE - 1 - i];
        }
    } else {
        memcpy(rk, ks->rk, sizeof(rk));
    }

    SM4_RNDS(0, 1, 2, 3);
    SM4_RNDS(4, 5, 6, 7);
    SM4_RNDS(8, 9, 10, 11);
    SM4_RNDS(12, 13, 14, 15);
    SM4_RNDS(16, 17, 18, 19);
    SM4_RNDS(20, 21, 22, 23);
    SM4_RNDS(24, 25, 26, 27);
    SM4_RNDS(28, 29, 30, 31);

    sm4_store_u32_be(b3, out);
    sm4_store_u32_be(b2, out + 4);
    sm4_store_u32_be(b1, out + 8);
    sm4_store_u32_be(b0, out + 12);
}

#undef SM4_RNDS

static void sm4_encrypt_block(const uint8_t *in, uint8_t *out, const SM4_KEY *ks)
{
    sm4_crypt_block(in, out, ks, 0);
}

static void sm4_decrypt_block(const uint8_t *in, uint8_t *out, const SM4_KEY *ks)
{
    sm4_crypt_block(in, out, ks, 1);
}

static void sm4_xor_block(uint8_t *out, const uint8_t *a, const uint8_t *b)
{
    for (int i = 0; i < SM4_BLOCK_SIZE; i++) {
        out[i] = a[i] ^ b[i];
    }
}

static int SM4_UNUSED sm4_cbc_encrypt(const uint8_t *in, uint8_t *out, size_t len,
                                      const SM4_KEY *ks, uint8_t iv[SM4_BLOCK_SIZE])
{
    uint8_t block[SM4_BLOCK_SIZE];

    if (len % SM4_BLOCK_SIZE != 0) {
        return 0;
    }

    for (size_t i = 0; i < len; i += SM4_BLOCK_SIZE) {
        sm4_xor_block(block, in + i, iv);
        sm4_encrypt_block(block, out + i, ks);
        memcpy(iv, out + i, SM4_BLOCK_SIZE);
    }

    return 1;
}

static int SM4_UNUSED sm4_cbc_decrypt(const uint8_t *in, uint8_t *out, size_t len,
                                      const SM4_KEY *ks, uint8_t iv[SM4_BLOCK_SIZE])
{
    uint8_t block[SM4_BLOCK_SIZE];
    uint8_t prev[SM4_BLOCK_SIZE];

    if (len % SM4_BLOCK_SIZE != 0) {
        return 0;
    }

    for (size_t i = 0; i < len; i += SM4_BLOCK_SIZE) {
        memcpy(prev, in + i, SM4_BLOCK_SIZE);
        sm4_decrypt_block(in + i, block, ks);
        sm4_xor_block(out + i, block, iv);
        memcpy(iv, prev, SM4_BLOCK_SIZE);
    }

    return 1;
}

static void sm4_fill_pattern(uint8_t *buf, size_t len)
{
    uint32_t x = 0x12345678u;
    for (size_t i = 0; i < len; i++) {
        x = x * 1664525u + 1013904223u;
        buf[i] = (uint8_t)(x >> 24);
    }
}

static uint32_t sm4_checksum(const uint8_t *buf, size_t len)
{
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        h ^= buf[i];
        h *= 16777619u;
    }
    return h;
}

#endif
