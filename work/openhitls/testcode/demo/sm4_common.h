/*
 * SM4 Common Definitions — shared across all SM4 optimization schemes.
 *
 * Reference: GB/T 32907-2016 (SM4 block cipher)
 *
 * This header provides:
 *   - S-box, FK, CK constants
 *   - T  function (uses L  for encryption rounds)
 *   - T' function (uses L' for key expansion)
 *   - Standard test vectors for correctness verification
 *
 * Usage: #include "sm4_common.h" in each scheme implementation.
 */

#ifndef SM4_COMMON_H
#define SM4_COMMON_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * S-box
 * ========================================================================== */
static const uint8_t SM4_SBOX[16][16] = {
    {0xd6,0x90,0xe9,0xfe,0xcc,0xe1,0x3d,0xb7,0x16,0xb6,0x14,0xc2,0x28,0xfb,0x2c,0x05},
    {0x2b,0x67,0x9a,0x76,0x2a,0xbe,0x04,0xc3,0xaa,0x44,0x13,0x26,0x49,0x86,0x06,0x99},
    {0x9c,0x42,0x50,0xf4,0x91,0xef,0x98,0x7a,0x33,0x54,0x0b,0x43,0xed,0xcf,0xac,0x62},
    {0xe4,0xb3,0x1c,0xa9,0xc9,0x08,0xe8,0x95,0x80,0xdf,0x94,0xfa,0x75,0x8f,0x3f,0xa6},
    {0x47,0x07,0xa7,0xfc,0xf3,0x73,0x17,0xba,0x83,0x59,0x3c,0x19,0xe6,0x85,0x4f,0xa8},
    {0x68,0x6b,0x81,0xb2,0x71,0x64,0xda,0x8b,0xf8,0xeb,0x0f,0x4b,0x70,0x56,0x9d,0x35},
    {0x1e,0x24,0x0e,0x5e,0x63,0x58,0xd1,0xa2,0x25,0x22,0x7c,0x3b,0x01,0x21,0x78,0x87},
    {0xd4,0x00,0x46,0x57,0x9f,0xd3,0x27,0x52,0x4c,0x36,0x02,0xe7,0xa0,0xc4,0xc8,0x9e},
    {0xea,0xbf,0x8a,0xd2,0x40,0xc7,0x38,0xb5,0xa3,0xf7,0xf2,0xce,0xf9,0x61,0x15,0xa1},
    {0xe0,0xae,0x5d,0xa4,0x9b,0x34,0x1a,0x55,0xad,0x93,0x32,0x30,0xf5,0x8c,0xb1,0xe3},
    {0x1d,0xf6,0xe2,0x2e,0x82,0x66,0xca,0x60,0xc0,0x29,0x23,0xab,0x0d,0x53,0x4e,0x6f},
    {0xd5,0xdb,0x37,0x45,0xde,0xfd,0x8e,0x2f,0x03,0xff,0x6a,0x72,0x6d,0x6c,0x5b,0x51},
    {0x8d,0x1b,0xaf,0x92,0xbb,0xdd,0xbc,0x7f,0x11,0xd9,0x5c,0x41,0x1f,0x10,0x5a,0xd8},
    {0x0a,0xc1,0x31,0x88,0xa5,0xcd,0x7b,0xbd,0x2d,0x74,0xd0,0x12,0xb8,0xe5,0xb4,0xb0},
    {0x89,0x69,0x97,0x4a,0x0c,0x96,0x77,0x7e,0x65,0xb9,0xf1,0x09,0xc5,0x6e,0xc6,0x84},
    {0x18,0xf0,0x7d,0xec,0x3a,0xdc,0x4d,0x20,0x79,0xee,0x5f,0x3e,0xd7,0xcb,0x39,0x48}
};

/* ==========================================================================
 * FK — system parameters (constant XOR for initial key expansion)
 * ========================================================================== */
static const uint32_t SM4_FK[4] = {
    0xA3B1BAC6, 0x56AA3350, 0x677D9197, 0xB27022DC
};

/* ==========================================================================
 * CK — fixed parameters for each round of key expansion
 * ========================================================================== */
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

/* ==========================================================================
 * Standard test vector (GB/T 32907-2016, Appendix A)
 * Used by all implementations for correctness self-check.
 * ========================================================================== */
static const uint8_t SM4_TEST_KEY[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};

static const uint8_t SM4_TEST_PLAINTEXT[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
};

static const uint8_t SM4_TEST_CIPHERTEXT[16] = {
    0x68, 0x1e, 0xdf, 0x34, 0xd2, 0x06, 0x96, 0x5e,
    0x86, 0xb3, 0xe9, 0x4f, 0x53, 0x6e, 0x42, 0x46
};

/* ==========================================================================
 * S-box lookup — single byte substitution
 * ========================================================================== */
static inline uint8_t sm4Sbox(uint8_t in)
{
    return SM4_SBOX[(in >> 4) & 0x0F][in & 0x0F];
}

/* ==========================================================================
 * Non-linear transformation S (byte-wise S-box on 32-bit word)
 *   S(A) = (Sbox(a0), Sbox(a1), Sbox(a2), Sbox(a3))
 *         where A = (a0, a1, a2, a3) as big-endian bytes
 * ========================================================================== */
static inline uint32_t sm4S(uint32_t a)
{
    return ((uint32_t)sm4Sbox((a >> 24) & 0xFF) << 24)
         | ((uint32_t)sm4Sbox((a >> 16) & 0xFF) << 16)
         | ((uint32_t)sm4Sbox((a >>  8) & 0xFF) <<  8)
         | ((uint32_t)sm4Sbox( a        & 0xFF));
}

/* ==========================================================================
 * Linear transformation L (used in encryption rounds)
 *   L(B) = B ⊕ (B <<< 2) ⊕ (B <<< 10) ⊕ (B <<< 18) ⊕ (B <<< 24)
 *
 * Expressed as XOR decomposition (no-overlap property of shifts):
 *   B<<<k  =  (B << k) | (B >> (32-k))
 *   XOR:     (B << k) ^ (B >> (32-k))   (two halves never overlap)
 * ========================================================================== */
static inline uint32_t sm4L(uint32_t b)
{
    return b
        ^ ((b <<  2) | (b >> 30))   /* ROL 2  */
        ^ ((b << 10) | (b >> 22))   /* ROL 10 */
        ^ ((b << 18) | (b >> 14))   /* ROL 18 */
        ^ ((b << 24) | (b >>  8));  /* ROL 24 */
}

/* ==========================================================================
 * Linear transformation L' (used in key expansion)
 *   L'(B) = B ⊕ (B <<< 13) ⊕ (B <<< 23)
 *
 * Expressed as XOR decomposition:
 * ========================================================================== */
static inline uint32_t sm4LPrime(uint32_t b)
{
    return b
        ^ ((b << 13) | (b >> 19))   /* ROL 13 */
        ^ ((b << 23) | (b >>  9));  /* ROL 23 */
}

/* ==========================================================================
 * T  = L ∘ S   (encryption round function)
 * T' = L' ∘ S  (key expansion round function)
 * ========================================================================== */
static inline uint32_t sm4T(uint32_t a)
{
    return sm4L(sm4S(a));
}

static inline uint32_t sm4TPrime(uint32_t a)
{
    return sm4LPrime(sm4S(a));
}

/* ==========================================================================
 * Key expansion — produces 32 round keys from 16-byte master key.
 *
 * Used by ALL schemes identically — key expansion is not the optimization
 * target, only the encryption/decryption round function is varied.
 * ========================================================================== */
static inline void sm4KeyExpand(const uint8_t key[16], uint32_t rk[32])
{
    uint32_t K[4];
    int i;

    /* K_i = MK_i ⊕ FK_i,  i = 0,1,2,3 */
    for (i = 0; i < 4; i++) {
        K[i] = ((uint32_t)key[4*i    ] << 24)
             | ((uint32_t)key[4*i + 1] << 16)
             | ((uint32_t)key[4*i + 2] <<  8)
             | ((uint32_t)key[4*i + 3]);
        K[i] ^= SM4_FK[i];
    }

    /* K_i = K_{i-4} ⊕ T'(K_{i-3} ⊕ K_{i-2} ⊕ K_{i-1} ⊕ CK_i), i = 0..31 */
    for (i = 0; i < 32; i++) {
        uint32_t tmp = K[1] ^ K[2] ^ K[3] ^ SM4_CK[i];
        uint32_t t   = sm4TPrime(tmp);
        rk[i] = K[0] ^ t;
        K[0]  = K[1];
        K[1]  = K[2];
        K[2]  = K[3];
        K[3]  = rk[i];
    }
}

/* ==========================================================================
 * Helper: load a 16-byte block as 4 big-endian 32-bit words into X[0..3]
 * ========================================================================== */
static inline void sm4LoadBlock(const uint8_t in[16], uint32_t X[4])
{
    X[0] = ((uint32_t)in[ 0] << 24) | ((uint32_t)in[ 1] << 16)
         | ((uint32_t)in[ 2] <<  8) |  (uint32_t)in[ 3];
    X[1] = ((uint32_t)in[ 4] << 24) | ((uint32_t)in[ 5] << 16)
         | ((uint32_t)in[ 6] <<  8) |  (uint32_t)in[ 7];
    X[2] = ((uint32_t)in[ 8] << 24) | ((uint32_t)in[ 9] << 16)
         | ((uint32_t)in[10] <<  8) |  (uint32_t)in[11];
    X[3] = ((uint32_t)in[12] << 24) | ((uint32_t)in[13] << 16)
         | ((uint32_t)in[14] <<  8) |  (uint32_t)in[15];
}

/* ==========================================================================
 * Helper: store 4 big-endian 32-bit words X[0..3] in reverse order into
 *         16-byte output (SM4 final reverse transform R).
 * ========================================================================== */
static inline void sm4StoreBlock(const uint32_t X[4], uint8_t out[16])
{
    out[ 0] = (uint8_t)((X[3] >> 24) & 0xFF);
    out[ 1] = (uint8_t)((X[3] >> 16) & 0xFF);
    out[ 2] = (uint8_t)((X[3] >>  8) & 0xFF);
    out[ 3] = (uint8_t)( X[3]        & 0xFF);
    out[ 4] = (uint8_t)((X[2] >> 24) & 0xFF);
    out[ 5] = (uint8_t)((X[2] >> 16) & 0xFF);
    out[ 6] = (uint8_t)((X[2] >>  8) & 0xFF);
    out[ 7] = (uint8_t)( X[2]        & 0xFF);
    out[ 8] = (uint8_t)((X[1] >> 24) & 0xFF);
    out[ 9] = (uint8_t)((X[1] >> 16) & 0xFF);
    out[10] = (uint8_t)((X[1] >>  8) & 0xFF);
    out[11] = (uint8_t)( X[1]        & 0xFF);
    out[12] = (uint8_t)((X[0] >> 24) & 0xFF);
    out[13] = (uint8_t)((X[0] >> 16) & 0xFF);
    out[14] = (uint8_t)((X[0] >>  8) & 0xFF);
    out[15] = (uint8_t)( X[0]        & 0xFF);
}

#ifdef __cplusplus
}
#endif

#endif /* SM4_COMMON_H */
