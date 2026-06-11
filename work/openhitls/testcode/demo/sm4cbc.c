#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "crypt_eal_cipher.h"
#include "bsl_sal.h"
#include "bsl_err.h"
#include "crypt_algid.h"
#include "crypt_errno.h"

#define TOTAL_SIZE (16 * 1024 * 1024)

static const uint32_t benchSizes[] = {
    1 * 1024,
    16 * 1024,
    64 * 1024,
    256 * 1024,
    1 * 1024 * 1024,
    4 * 1024 * 1024,
    16 * 1024 * 1024
};

static double GetTimeSec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1000000000.0;
}

void *StdMalloc(uint32_t len)
{
    return malloc((size_t)len);
}

void PrintLastError(void)
{
    const char *file = NULL;
    uint32_t line = 0;
    BSL_ERR_GetLastErrorFileLine(&file, &line);
    printf("failed at file %s at line %d\n", file, line);
}

static int32_t RunBench(CRYPT_EAL_CipherCtx *ctx, const uint8_t *key, uint32_t blockSize)
{
    int32_t ret = CRYPT_SUCCESS;
    uint32_t loops = TOTAL_SIZE / blockSize;

    uint8_t *benchIn = malloc(blockSize);
    uint8_t *benchOut = malloc(blockSize + 16);

    if (benchIn == NULL || benchOut == NULL) {
        printf("malloc failed, blockSize = %u\n", blockSize);
        free(benchIn);
        free(benchOut);
        return -1;
    }

    memset(benchIn, 0x11, blockSize);
    memset(benchOut, 0, blockSize + 16);

    double start = GetTimeSec();

    for (uint32_t loop = 0; loop < loops; loop++) {
        uint8_t benchIv[16] = {0};
        uint32_t benchOutLen = blockSize;
        uint32_t finalLen = 0;

        ret = CRYPT_EAL_CipherInit(ctx, key, 16, benchIv, sizeof(benchIv), true);
        if (ret != CRYPT_SUCCESS) {
            printf("CipherInit failed, error code is %x\n", ret);
            goto EXIT;
        }

        ret = CRYPT_EAL_CipherSetPadding(ctx, CRYPT_PADDING_NONE);
        if (ret != CRYPT_SUCCESS) {
            printf("SetPadding failed, error code is %x\n", ret);
            goto EXIT;
        }

        ret = CRYPT_EAL_CipherUpdate(ctx, benchIn, blockSize, benchOut, &benchOutLen);
        if (ret != CRYPT_SUCCESS) {
            printf("CipherUpdate failed, error code is %x\n", ret);
            goto EXIT;
        }

        ret = CRYPT_EAL_CipherFinal(ctx, benchOut + benchOutLen, &finalLen);
        if (ret != CRYPT_SUCCESS) {
            printf("CipherFinal failed, error code is %x\n", ret);
            goto EXIT;
        }
    }

    double end = GetTimeSec();

    double seconds = end - start;
    double totalMB = (double)TOTAL_SIZE / 1024.0 / 1024.0;
    double speed = totalMB / seconds;

    printf("%8u KB | loops: %6u | time: %.6f s | throughput: %.2f MB/s\n",
           blockSize / 1024, loops, seconds, speed);

EXIT:
    free(benchIn);
    free(benchOut);
    return ret;
}
static int32_t RunBenchUpdateOnly(CRYPT_EAL_CipherCtx *ctx, const uint8_t *key, uint32_t blockSize)
{
    int32_t ret = CRYPT_SUCCESS;
    uint32_t loops = TOTAL_SIZE / blockSize;

    uint8_t *benchIn = malloc(blockSize);
    uint8_t *benchOut = malloc(blockSize + 16);

    if (benchIn == NULL || benchOut == NULL) {
        printf("malloc failed, blockSize = %u\n", blockSize);
        free(benchIn);
        free(benchOut);
        return -1;
    }

    memset(benchIn, 0x11, blockSize);
    memset(benchOut, 0, blockSize + 16);

    uint8_t benchIv[16] = {0};

    ret = CRYPT_EAL_CipherInit(ctx, key, 16, benchIv, sizeof(benchIv), true);
    if (ret != CRYPT_SUCCESS) {
        printf("CipherInit failed, error code is %x\n", ret);
        goto EXIT;
    }

    ret = CRYPT_EAL_CipherSetPadding(ctx, CRYPT_PADDING_NONE);
    if (ret != CRYPT_SUCCESS) {
        printf("SetPadding failed, error code is %x\n", ret);
        goto EXIT;
    }

    double start = GetTimeSec();

    for (uint32_t loop = 0; loop < loops; loop++) {
        uint32_t benchOutLen = blockSize;

        ret = CRYPT_EAL_CipherUpdate(ctx, benchIn, blockSize, benchOut, &benchOutLen);
        if (ret != CRYPT_SUCCESS) {
            printf("CipherUpdate failed, error code is %x\n", ret);
            goto EXIT;
        }
    }

    double end = GetTimeSec();

    uint32_t finalLen = 0;
    ret = CRYPT_EAL_CipherFinal(ctx, benchOut, &finalLen);
    if (ret != CRYPT_SUCCESS) {
        printf("CipherFinal failed, error code is %x\n", ret);
        goto EXIT;
    }

    double seconds = end - start;
    double totalMB = (double)TOTAL_SIZE / 1024.0 / 1024.0;
    double speed = totalMB / seconds;

    printf("%8u KB | loops: %6u | time: %.6f s | update-only throughput: %.2f MB/s\n",
           blockSize / 1024, loops, seconds, speed);

EXIT:
    free(benchIn);
    free(benchOut);
    return ret;
}
int main(void)
{
    int32_t ret = CRYPT_SUCCESS;
    uint8_t key[16] = {0};

    CRYPT_EAL_CipherCtx *ctx = CRYPT_EAL_CipherNewCtx(CRYPT_CIPHER_SM4_CBC);
    if (ctx == NULL) {
        PrintLastError();
        BSL_ERR_DeInit();
        return 1;
    }

    printf("SM4-CBC throughput test, total data = 16MB\n");
    printf("block size | loops  | time | throughput\n");
    printf("---------------------------------------------------------------\n");

    for (uint32_t i = 0; i < sizeof(benchSizes) / sizeof(benchSizes[0]); i++) {
        ret = RunBench(ctx, key, benchSizes[i]);
        if (ret != CRYPT_SUCCESS) {
            break;
        }
    }
    printf("SM4-CBC CipherUpdate-only throughput test, total data = 16MB\n");
    printf("block size | loops  | time | throughput\n");
    printf("--------------------------------------------------------------------------\n");

    for (uint32_t i = 0; i < sizeof(benchSizes) / sizeof(benchSizes[0]); i++) {
        ret = RunBenchUpdateOnly(ctx, key, benchSizes[i]);
        if (ret != CRYPT_SUCCESS) {
            break;
        }
    }
    CRYPT_EAL_CipherFreeCtx(ctx);
    BSL_ERR_DeInit();
    return ret;
}