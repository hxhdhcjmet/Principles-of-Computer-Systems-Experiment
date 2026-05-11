#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#define SM4_KEY_SCHEDULE 32
#define LOOP 2000000
#define DATA_LEN 16
typedef struct {
    uint32_t rk[SM4_KEY_SCHEDULE];
} SM4_KEY;
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


static inline uint32_t rotl(uint32_t a, uint8_t n)
{
    return (a << n) | (a >> (32 - n));
}

static inline uint32_t load_u32_be(const uint8_t *b, uint32_t n)
{
    return ((uint32_t)b[4 * n] << 24) | ((uint32_t)b[4 * n + 1] << 16) | ((uint32_t)b[4 * n + 2] << 8) | ((uint32_t)b[4 * n + 3]);
}

static inline void store_u32_be(uint32_t v, uint8_t *b)
{
    b[0] = (uint8_t)(v >> 24);
    b[1] = (uint8_t)(v >> 16);
    b[2] = (uint8_t)(v >> 8);
    b[3] = (uint8_t)(v);
}

static inline uint32_t SM4_T_non_lin_sub(uint32_t X)
{
    uint32_t t = 0;

    t |= ((uint32_t)SM4_S[(uint8_t)(X >> 24)]) << 24;
    t |= ((uint32_t)SM4_S[(uint8_t)(X >> 16)]) << 16;
    t |= ((uint32_t)SM4_S[(uint8_t)(X >> 8)]) << 8;
    t |= SM4_S[(uint8_t)X];

    return t;
}

static inline uint32_t SM4_T(uint32_t X)
{
    uint32_t t = SM4_T_non_lin_sub(X);

    /*
     * L linear transform
     */
    return t ^ rotl(t, 2) ^ rotl(t, 10) ^ rotl(t, 18) ^ rotl(t, 24);
}

static inline uint32_t SM4_key_sub(uint32_t X)
{
    uint32_t t = SM4_T_non_lin_sub(X);

    return t ^ rotl(t, 13) ^ rotl(t, 23);
}

int sm4_set_key(const uint8_t *key, SM4_KEY *ks)
{
    /*
     * Family Key
     */
    static const uint32_t FK[4] = {
        0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc
    };

    /*
     * Constant Key
     */
    static const uint32_t CK[32] = {
        0x00070E15, 0x1C232A31, 0x383F464D, 0x545B6269,
        0x70777E85, 0x8C939AA1, 0xA8AFB6BD, 0xC4CBD2D9,
        0xE0E7EEF5, 0xFC030A11, 0x181F262D, 0x343B4249,
        0x50575E65, 0x6C737A81, 0x888F969D, 0xA4ABB2B9,
        0xC0C7CED5, 0xDCE3EAF1, 0xF8FF060D, 0x141B2229,
        0x30373E45, 0x4C535A61, 0x686F767D, 0x848B9299,
        0xA0A7AEB5, 0xBCC3CAD1, 0xD8DFE6ED, 0xF4FB0209,
        0x10171E25, 0x2C333A41, 0x484F565D, 0x646B7279
    };

    uint32_t K[4];
    int i;

    K[0] = load_u32_be(key, 0) ^ FK[0];
    K[1] = load_u32_be(key, 1) ^ FK[1];
    K[2] = load_u32_be(key, 2) ^ FK[2];
    K[3] = load_u32_be(key, 3) ^ FK[3];

    for (i = 0; i < SM4_KEY_SCHEDULE; i = i + 4) {
        K[0] ^= SM4_key_sub(K[1] ^ K[2] ^ K[3] ^ CK[i]);
        K[1] ^= SM4_key_sub(K[2] ^ K[3] ^ K[0] ^ CK[i + 1]);
        K[2] ^= SM4_key_sub(K[3] ^ K[0] ^ K[1] ^ CK[i + 2]);
        K[3] ^= SM4_key_sub(K[0] ^ K[1] ^ K[2] ^ CK[i + 3]);
        ks->rk[i] = K[0];
        ks->rk[i + 1] = K[1];
        ks->rk[i + 2] = K[2];
        ks->rk[i + 3] = K[3];
    }

    return 1;
}

#define SM4_RNDS(k0, k1, k2, k3, F)         \
    do {                                    \
        B0 ^= F(B1 ^ B2 ^ B3 ^ ks->rk[k0]); \
        B1 ^= F(B0 ^ B2 ^ B3 ^ ks->rk[k1]); \
        B2 ^= F(B0 ^ B1 ^ B3 ^ ks->rk[k2]); \
        B3 ^= F(B0 ^ B1 ^ B2 ^ ks->rk[k3]); \
    } while (0)

void sm4_encrypt(const uint8_t *in, uint8_t *out, const SM4_KEY *ks)
{
    uint32_t B0 = load_u32_be(in, 0);
    uint32_t B1 = load_u32_be(in, 1);
    uint32_t B2 = load_u32_be(in, 2);
    uint32_t B3 = load_u32_be(in, 3);

    /*
     * Uses byte-wise sbox in the first and last rounds to provide some
     * protection from cache based side channels.
     */
    SM4_RNDS(0, 1, 2, 3, SM4_T);
    SM4_RNDS(4, 5, 6, 7, SM4_T);
    SM4_RNDS(8, 9, 10, 11, SM4_T);
    SM4_RNDS(12, 13, 14, 15, SM4_T);
    SM4_RNDS(16, 17, 18, 19, SM4_T);
    SM4_RNDS(20, 21, 22, 23, SM4_T);
    SM4_RNDS(24, 25, 26, 27, SM4_T);
    SM4_RNDS(28, 29, 30, 31, SM4_T);

    store_u32_be(B3, out);
    store_u32_be(B2, out + 4);
    store_u32_be(B1, out + 8);
    store_u32_be(B0, out + 12);
}
void sm4_decrypt(const uint8_t *in, uint8_t *out, const SM4_KEY *ks)
{
    uint32_t B0 = load_u32_be(in, 0);
    uint32_t B1 = load_u32_be(in, 1);
    uint32_t B2 = load_u32_be(in, 2);
    uint32_t B3 = load_u32_be(in, 3);

    SM4_RNDS(31, 30, 29, 28, SM4_T);
    SM4_RNDS(27, 26, 25, 24, SM4_T);
    SM4_RNDS(23, 22, 21, 20, SM4_T);
    SM4_RNDS(19, 18, 17, 16, SM4_T);
    SM4_RNDS(15, 14, 13, 12, SM4_T);
    SM4_RNDS(11, 10, 9, 8, SM4_T);
    SM4_RNDS(7, 6, 5, 4, SM4_T);
    SM4_RNDS(3, 2, 1, 0, SM4_T);

    store_u32_be(B3, out);
    store_u32_be(B2, out + 4);
    store_u32_be(B1, out + 8);
    store_u32_be(B0, out + 12);
}
#define SM4_BLOCK_SIZE 16

static void xor_block(uint8_t *out, const uint8_t *a, const uint8_t *b)
{
    for (int i = 0; i < SM4_BLOCK_SIZE; i++) {
        out[i] = a[i] ^ b[i];
    }
}

void sm4_cbc_encrypt(const uint8_t *in, uint8_t *out, size_t len,
                          const SM4_KEY *ks, uint8_t iv[SM4_BLOCK_SIZE])
{
    uint8_t block[SM4_BLOCK_SIZE];

    if (len % SM4_BLOCK_SIZE != 0) {
        return;
    }

    for (size_t i = 0; i < len; i += SM4_BLOCK_SIZE) {
        xor_block(block, in + i, iv);
        sm4_encrypt(block, out + i, ks);
        memcpy(iv, out + i, SM4_BLOCK_SIZE);
    }
}
void sm4_cbc_decrypt(const uint8_t *in, uint8_t *out, size_t len,
                     const SM4_KEY *ks, uint8_t iv[SM4_BLOCK_SIZE])
{
    uint8_t block[SM4_BLOCK_SIZE];
    uint8_t prev[SM4_BLOCK_SIZE];

    if (len % SM4_BLOCK_SIZE != 0) {
        return;
    }

    for (size_t i = 0; i < len; i += SM4_BLOCK_SIZE) {
        memcpy(prev, in + i, SM4_BLOCK_SIZE);
        sm4_decrypt(in + i, block, ks);
        xor_block(out + i, block, iv);
        memcpy(iv, prev, SM4_BLOCK_SIZE);
    }
}


int main(void)
{
    uint8_t key[16] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10
    };
    uint8_t iv[16] = {
        0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,
        0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f
    };
    // uint8_t iv[16] = {
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    //     0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
    // };

    uint8_t plain[DATA_LEN] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10,
    };

    uint8_t cipher[DATA_LEN];
    uint8_t decrypted[DATA_LEN];

    uint8_t enc_iv[16];
    uint8_t dec_iv[16];

    SM4_KEY ks;
    sm4_set_key(key, &ks);

    clock_t start, end;

    start = clock();

    for (int i = 0; i < LOOP; i++) {
        memcpy(enc_iv, iv, 16);
        sm4_cbc_encrypt(plain, cipher, DATA_LEN, &ks, enc_iv);
    }

    end = clock();

    double time_used = (double)(end - start) / CLOCKS_PER_SEC;
    double total_bytes = (double)LOOP * DATA_LEN;
    double mbps = total_bytes / time_used / 1000000.0;
    double mibps = total_bytes / time_used / 1024.0 / 1024.0;

    printf("time: %.6f s\n", time_used);
    printf("data: %.0f bytes\n", total_bytes);
    printf("throughput: %.2f MB/s\n", mbps);
    printf("throughput: %.2f MiB/s\n", mibps);

    memcpy(dec_iv, iv, 16);
    sm4_cbc_decrypt(cipher, decrypted, DATA_LEN, &ks, dec_iv);

    printf("plain: ");
    for (int i = 0; i < DATA_LEN; i++) {
        printf("%02x", decrypted[i]);
    }
    printf("\n");
    printf("cipher: ");
    for (int i = 0; i < DATA_LEN; i++) {
        printf("%02x", cipher[i]);
    }
    printf("\n");
    return 0;
}