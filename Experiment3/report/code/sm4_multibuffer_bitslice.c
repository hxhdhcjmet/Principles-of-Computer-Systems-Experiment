#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sm4_portable.h"

#if defined(BS_BACKEND_AVX2)
#include <immintrin.h>
#endif

#if defined(BS_BACKEND_RVV)
#include <riscv_vector.h>
#endif

/*
 * Portable bitsliced SM4-CBC multibuffer implementation.
 *
 * This version is a real bitslice engine:
 * 1. 32 independent blocks are orthogonalized into bit-planes.
 * 2. The SM4 S-box is evaluated as a Boolean function over those planes.
 * 3. The linear layer is implemented as plane permutations and XORs.
 *
 * No secret-dependent table lookup or platform-specific intrinsics are used.
 */

#ifndef BS_LANES
#define BS_LANES 32
#endif

#ifndef BS_VERSION
#define BS_VERSION "multi-buffer-bitslice-portable-ref"
#endif

#ifndef DATA_LEN
#define DATA_LEN (4 * 1024)
#endif

#ifndef LOOP
#define LOOP 10
#endif

#if defined(BS_BACKEND_AVX2)
typedef __m256i bs_word;

static inline bs_word bs_zero(void)
{
    return _mm256_setzero_si256();
}

static inline bs_word bs_ones(void)
{
    bs_word z = _mm256_setzero_si256();
    return _mm256_cmpeq_epi64(z, z);
}

static inline bs_word bs_xor_word(bs_word a, bs_word b)
{
    return _mm256_xor_si256(a, b);
}

static inline bs_word bs_and_word(bs_word a, bs_word b)
{
    return _mm256_and_si256(a, b);
}

static inline bs_word bs_lane_mask(int lane)
{
    uint64_t parts[4] = {0, 0, 0, 0};
    parts[lane >> 6] = UINT64_C(1) << (lane & 63);
    return _mm256_set_epi64x((long long)parts[3], (long long)parts[2],
                             (long long)parts[1], (long long)parts[0]);
}

static inline int bs_lane_test(bs_word word, int lane)
{
    uint64_t parts[4];
    _mm256_storeu_si256((__m256i *)parts, word);
    return (int)((parts[lane >> 6] >> (lane & 63)) & 1u);
}

#elif defined(BS_BACKEND_RVV)
typedef vuint64m8_t bs_word;

static inline size_t bs_rvv_vl(void)
{
    return __riscv_vsetvl_e64m8(4);
}

static inline bs_word bs_zero(void)
{
    return __riscv_vmv_v_x_u64m8(0, bs_rvv_vl());
}

static inline bs_word bs_ones(void)
{
    return __riscv_vmv_v_x_u64m8(UINT64_MAX, bs_rvv_vl());
}

static inline bs_word bs_xor_word(bs_word a, bs_word b)
{
    return __riscv_vxor_vv_u64m8(a, b, bs_rvv_vl());
}

static inline bs_word bs_and_word(bs_word a, bs_word b)
{
    return __riscv_vand_vv_u64m8(a, b, bs_rvv_vl());
}

static inline bs_word bs_lane_mask(int lane)
{
    uint64_t parts[4] = {0, 0, 0, 0};
    parts[lane >> 6] = UINT64_C(1) << (lane & 63);
    return __riscv_vle64_v_u64m8(parts, bs_rvv_vl());
}

static inline int bs_lane_test(bs_word word, int lane)
{
    uint64_t parts[4];
    __riscv_vse64_v_u64m8(parts, word, bs_rvv_vl());
    return (int)((parts[lane >> 6] >> (lane & 63)) & 1u);
}

#else
#ifndef BS_WORD_TYPE
#define BS_WORD_TYPE uint32_t
#endif

typedef BS_WORD_TYPE bs_word;

static inline bs_word bs_zero(void)
{
    return (bs_word)0;
}

static inline bs_word bs_ones(void)
{
    return ~(bs_word)0;
}

static inline bs_word bs_xor_word(bs_word a, bs_word b)
{
    return a ^ b;
}

static inline bs_word bs_and_word(bs_word a, bs_word b)
{
    return a & b;
}

static inline bs_word bs_lane_mask(int lane)
{
    return ((bs_word)1) << lane;
}

static inline int bs_lane_test(bs_word word, int lane)
{
    return (int)((word >> lane) & 1u);
}
#endif

typedef struct {
    bs_word bit[32];
} bs32;

typedef struct {
    uint8_t masks[256];
    uint16_t count;
} sbox_anf_terms;

static sbox_anf_terms SBOX_TERMS[8];
static int SBOX_TERMS_READY = 0;

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

static void sm4_set_key_scalar(const uint8_t key[16], uint32_t rk[32])
{
    uint32_t k[4];

    for (int i = 0; i < 4; i++) {
        k[i] = load_be32(key + i * 4) ^ SM4_FK[i];
    }

    for (int i = 0; i < 32; i++) {
        uint32_t t = k[1] ^ k[2] ^ k[3] ^ SM4_CK[i];
        uint32_t s = ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 24)) << 24) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 16)) << 16) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 8)) << 8) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)t));
        rk[i] = k[0] ^ s ^ rotl32(s, 13) ^ rotl32(s, 23);
        k[0] = k[1];
        k[1] = k[2];
        k[2] = k[3];
        k[3] = rk[i];
    }
}

static void sm4_encrypt_block_scalar(const uint8_t in[16], uint8_t out[16], const uint32_t rk[32])
{
    uint32_t x[4];

    x[0] = load_be32(in);
    x[1] = load_be32(in + 4);
    x[2] = load_be32(in + 8);
    x[3] = load_be32(in + 12);

    for (int i = 0; i < 32; i++) {
        uint32_t t = x[1] ^ x[2] ^ x[3] ^ rk[i];
        uint32_t s = ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 24)) << 24) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 16)) << 16) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)(t >> 8)) << 8) |
                     ((uint32_t)sm4_sbox_byte((uint8_t)t));
        uint32_t y = s ^ rotl32(s, 2) ^ rotl32(s, 10) ^ rotl32(s, 18) ^ rotl32(s, 24);
        uint32_t next = x[0] ^ y;
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = next;
    }

    store_be32(x[3], out);
    store_be32(x[2], out + 4);
    store_be32(x[1], out + 8);
    store_be32(x[0], out + 12);
}

static void init_sbox_terms(void)
{
    if (SBOX_TERMS_READY) {
        return;
    }

    for (int out_bit = 0; out_bit < 8; out_bit++) {
        uint8_t coeff[256];
        sbox_anf_terms *terms = &SBOX_TERMS[out_bit];

        for (int x = 0; x < 256; x++) {
            coeff[x] = (sm4_sbox_byte((uint8_t)x) >> out_bit) & 1u;
        }

        for (int bit = 0; bit < 8; bit++) {
            for (int mask = 0; mask < 256; mask++) {
                if (mask & (1 << bit)) {
                    coeff[mask] ^= coeff[mask ^ (1 << bit)];
                }
            }
        }

        terms->count = 0;
        for (int mask = 0; mask < 256; mask++) {
            if (coeff[mask]) {
                terms->masks[terms->count++] = (uint8_t)mask;
            }
        }
    }

    SBOX_TERMS_READY = 1;
}

static inline bs_word bs_all_ones(void)
{
    return bs_ones();
}

static inline int bs_low_bit_index(unsigned mask)
{
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_ctz(mask);
#else
    int index = 0;
    while (((mask >> index) & 1u) == 0) {
        index++;
    }
    return index;
#endif
}

static bs32 bs32_xor(bs32 a, bs32 b)
{
    bs32 out;
    for (int i = 0; i < 32; i++) {
        out.bit[i] = bs_xor_word(a.bit[i], b.bit[i]);
    }
    return out;
}

static bs32 bs32_xor_const(bs32 a, uint32_t constant)
{
    for (int i = 0; i < 32; i++) {
        if ((constant >> i) & 1u) {
            a.bit[i] = bs_xor_word(a.bit[i], bs_all_ones());
        }
    }
    return a;
}

static bs32 bs32_rol(bs32 a, unsigned n)
{
    bs32 out;
    for (int i = 0; i < 32; i++) {
        out.bit[(i + n) & 31] = a.bit[i];
    }
    return out;
}

static bs32 sm4_sbox_bitsliced_word(bs32 in)
{
    bs32 out;

    for (int byte = 0; byte < 4; byte++) {
        bs_word in_bits[8];
        bs_word monomial[256];
        bs_word out_bits[8];

        for (int bit = 0; bit < 8; bit++) {
            in_bits[bit] = in.bit[byte * 8 + bit];
        }

        monomial[0] = bs_all_ones();
        for (int mask = 1; mask < 256; mask++) {
            unsigned low_bit = (unsigned)mask & (0u - (unsigned)mask);
            int bit_index = bs_low_bit_index(low_bit);
            monomial[mask] = bs_and_word(monomial[mask ^ low_bit], in_bits[bit_index]);
        }

        for (int bit = 0; bit < 8; bit++) {
            const sbox_anf_terms *terms = &SBOX_TERMS[bit];
            bs_word value = bs_zero();

            for (uint16_t i = 0; i < terms->count; i++) {
                value = bs_xor_word(value, monomial[terms->masks[i]]);
            }
            out_bits[bit] = value;
        }
        for (int bit = 0; bit < 8; bit++) {
            out.bit[byte * 8 + bit] = out_bits[bit];
        }
    }

    return out;
}

static bs32 sm4_T_bitsliced(bs32 in)
{
    bs32 s = sm4_sbox_bitsliced_word(in);
    bs32 r2 = bs32_rol(s, 2);
    bs32 r10 = bs32_rol(s, 10);
    bs32 r18 = bs32_rol(s, 18);
    bs32 r24 = bs32_rol(s, 24);
    return bs32_xor(bs32_xor(bs32_xor(bs32_xor(s, r2), r10), r18), r24);
}

static void orthogonalize_words(const uint32_t words[BS_LANES][4], bs32 x[4])
{
    for (int w = 0; w < 4; w++) {
        for (int bit = 0; bit < 32; bit++) {
            x[w].bit[bit] = bs_zero();
        }
    }

    for (int lane = 0; lane < BS_LANES; lane++) {
        bs_word lane_mask = bs_lane_mask(lane);
        for (int w = 0; w < 4; w++) {
            uint32_t value = words[lane][w];
            for (int bit = 0; bit < 32; bit++) {
                if ((value >> bit) & 1u) {
                    x[w].bit[bit] = bs_xor_word(x[w].bit[bit], lane_mask);
                }
            }
        }
    }
}

static void deorthogonalize_words(const bs32 x[4], uint32_t words[BS_LANES][4])
{
    for (int lane = 0; lane < BS_LANES; lane++) {
        for (int w = 0; w < 4; w++) {
            uint32_t value = 0;
            for (int bit = 0; bit < 32; bit++) {
                if (bs_lane_test(x[w].bit[bit], lane)) {
                    value |= 1u << bit;
                }
            }
            words[lane][w] = value;
        }
    }
}

static void sm4_encrypt_bitslice_x32(const uint8_t in[BS_LANES][16],
                                     uint8_t out[BS_LANES][16],
                                     const uint32_t rk[32])
{
    uint32_t scalar_words[BS_LANES][4];
    bs32 x[4];

    for (int lane = 0; lane < BS_LANES; lane++) {
        scalar_words[lane][0] = load_be32(in[lane] + 0);
        scalar_words[lane][1] = load_be32(in[lane] + 4);
        scalar_words[lane][2] = load_be32(in[lane] + 8);
        scalar_words[lane][3] = load_be32(in[lane] + 12);
    }

    orthogonalize_words(scalar_words, x);

    for (int round = 0; round < 32; round++) {
        bs32 t = bs32_xor(bs32_xor(x[1], x[2]), x[3]);
        t = bs32_xor_const(t, rk[round]);
        bs32 next = bs32_xor(x[0], sm4_T_bitsliced(t));
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = next;
    }

    {
        bs32 tmp = x[0];
        x[0] = x[3];
        x[3] = tmp;
        tmp = x[1];
        x[1] = x[2];
        x[2] = tmp;
    }

    deorthogonalize_words(x, scalar_words);

    for (int lane = 0; lane < BS_LANES; lane++) {
        store_be32(scalar_words[lane][0], out[lane] + 0);
        store_be32(scalar_words[lane][1], out[lane] + 4);
        store_be32(scalar_words[lane][2], out[lane] + 8);
        store_be32(scalar_words[lane][3], out[lane] + 12);
    }
}

static double now_seconds(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static int self_test_bitslice(const uint32_t rk[32])
{
    uint8_t in[BS_LANES][16];
    uint8_t out_bs[BS_LANES][16];
    uint8_t out_ref[16];
    static const uint8_t test_plain[16] = {
        0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,
        0xfe,0xdc,0xba,0x98,0x76,0x54,0x32,0x10
    };

    for (int lane = 0; lane < BS_LANES; lane++) {
        memcpy(in[lane], test_plain, 16);
    }

    sm4_encrypt_bitslice_x32(in, out_bs, rk);
    sm4_encrypt_block_scalar(test_plain, out_ref, rk);

    return memcmp(out_bs[0], out_ref, 16) == 0;
}

static void bitslice_multibuffer_cbc_encrypt(const uint8_t *plain,
                                             uint8_t *cipher,
                                             size_t len_per_stream,
                                             const uint32_t rk[32],
                                             const uint8_t base_iv[16])
{
    uint8_t ivs[BS_LANES][16];
    size_t offsets[BS_LANES];
    uint8_t batch_in[BS_LANES][16];
    uint8_t batch_out[BS_LANES][16];
    size_t blocks = len_per_stream / SM4_BLOCK_SIZE;

    for (int lane = 0; lane < BS_LANES; lane++) {
        memcpy(ivs[lane], base_iv, 16);
        ivs[lane][15] ^= (uint8_t)lane;
        offsets[lane] = (size_t)lane * len_per_stream;
    }

    for (size_t blk = 0; blk < blocks; blk++) {
        for (int lane = 0; lane < BS_LANES; lane++) {
            for (int j = 0; j < SM4_BLOCK_SIZE; j++) {
                batch_in[lane][j] = plain[offsets[lane] + j] ^ ivs[lane][j];
            }
        }

        sm4_encrypt_bitslice_x32(batch_in, batch_out, rk);

        for (int lane = 0; lane < BS_LANES; lane++) {
            memcpy(cipher + offsets[lane], batch_out[lane], 16);
            memcpy(ivs[lane], batch_out[lane], 16);
            offsets[lane] += 16;
        }
    }
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
    size_t total_bytes = (size_t)BS_LANES * DATA_LEN;
    uint8_t *plain = malloc(total_bytes);
    uint8_t *cipher = malloc(total_bytes);
    int verify_ok;

    if (plain == NULL || cipher == NULL) {
        fprintf(stderr, "allocation failed\n");
        free(plain);
        free(cipher);
        return 1;
    }

    init_sbox_terms();
    sm4_set_key_scalar(key, rk);
    verify_ok = self_test_bitslice(rk);

    sm4_fill_pattern(plain, total_bytes);

    {
        double start = now_seconds();
        for (int i = 0; i < LOOP; i++) {
            bitslice_multibuffer_cbc_encrypt(plain, cipher, DATA_LEN, rk, base_iv);
        }
        double elapsed = now_seconds() - start;
        double total_data = (double)BS_LANES * (double)LOOP * (double)DATA_LEN;

        printf("version: %s\n", BS_VERSION);
        printf("lanes: %d\n", BS_LANES);
        printf("threads: %d\n", BS_LANES);
        printf("data_len_per_thread: %d bytes\n", DATA_LEN);
        printf("loop_per_thread: %d\n", LOOP);
        printf("time: %.6f s\n", elapsed);
        printf("data: %.0f bytes\n", total_data);
        printf("throughput: %.2f MB/s\n", total_data / elapsed / 1000000.0);
        printf("throughput: %.2f MiB/s\n", total_data / elapsed / 1024.0 / 1024.0);
        printf("verify: %s\n", verify_ok ? "ok" : "failed");
        printf("cipher_checksum: %08x\n", sm4_checksum(cipher, total_bytes));
    }

    free(plain);
    free(cipher);
    return verify_ok ? 0 : 1;
}
