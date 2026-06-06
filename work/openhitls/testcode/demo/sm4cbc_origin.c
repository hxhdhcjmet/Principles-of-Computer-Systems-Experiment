/*
 * This file is part of the openHiTLS project.
 *
 * openHiTLS is licensed under the Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

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
#include <pthread.h>

#define THREAD_NUM 8

typedef struct timespec TimeSpec;

static inline void start_timer(TimeSpec* ts) {
    clock_gettime(CLOCK_MONOTONIC, ts);
}

static inline uint64_t get_elapsed_us(const TimeSpec* start, const TimeSpec* end) {
    return (end->tv_sec - start->tv_sec) * 1000000 + (end->tv_nsec - start->tv_nsec) / 1000;
}

void* StdMalloc(uint32_t len) {
    return malloc((size_t)len);
}

void PrintLastError(void) {
    const char* file = NULL;
    uint32_t line = 0;
    BSL_ERR_GetLastErrorFileLine(&file, &line);
    printf("failed at file %s at line %d\n", file, line);
}

typedef struct {
    uint8_t* cipherText;
    uint32_t cipherLen;
    uint8_t* plainText;
    uint8_t* key;
    uint8_t* iv;
    uint32_t index;
    uint32_t totalThreads;
} ThreadDecryptData;

// 用于精确计时（纯解密计算耗时）
static TimeSpec g_decrypt_start;
static TimeSpec g_decrypt_end;
static volatile int g_threads_completed = 0;
static pthread_barrier_t g_barrier;

void* thread_decrypt(void* arg) {
    ThreadDecryptData* data = (ThreadDecryptData*)arg;
    uint32_t thread_index = data->index;
    uint32_t total_threads = data->totalThreads;

    // 等待所有线程就绪
    pthread_barrier_wait(&g_barrier);

    // 第一个线程负责记录开始时间
    if (thread_index == 0) {
        clock_gettime(CLOCK_MONOTONIC, &g_decrypt_start);
    }

    // 执行解密任务（与原逻辑完全相同）
    CRYPT_EAL_CipherCtx* ctx = CRYPT_EAL_CipherNewCtx(CRYPT_CIPHER_SM4_CBC);
    if (ctx == NULL) pthread_exit(NULL);

    CRYPT_EAL_CipherSetPadding(ctx, CRYPT_PADDING_NONE);

    for (uint32_t i = thread_index; i < data->cipherLen / 16; i += total_threads) {
        uint8_t localIV[16];
        if (i == 0)
            memcpy(localIV, data->iv, 16);
        else
            memcpy(localIV, data->cipherText + (i - 1) * 16, 16);

        if (CRYPT_EAL_CipherInit(ctx, data->key, 16, localIV, 16, false) != CRYPT_SUCCESS) {
            continue;
        }

        uint32_t outLen = 16;
        CRYPT_EAL_CipherUpdate(ctx,
            data->cipherText + i * 16,
            16,
            data->plainText + i * 16,
            &outLen);
    }

    CRYPT_EAL_CipherFreeCtx(ctx);

    // 最后一个线程记录结束时间
    __sync_add_and_fetch(&g_threads_completed, 1);
    if (g_threads_completed == total_threads) {
        clock_gettime(CLOCK_MONOTONIC, &g_decrypt_end);
    }

    pthread_exit(NULL);
}

int main(void)
{
    uint8_t data[1024 * 1024];
    memset(data, 0x1a, sizeof(data));
    uint8_t iv[16] = { 0 };
    uint8_t key[16] = { 0 };
    uint32_t dataLen = sizeof(data);
    uint8_t cipherText[1024 * 1024 * 2];
    uint8_t plainText[1024 * 1024 * 2];
    uint32_t outTotalLen = 0;
    uint32_t outLen = sizeof(cipherText);
    uint32_t cipherTextLen;
    int32_t ret;

    BSL_ERR_Init();
    BSL_SAL_CallBack_Ctrl(BSL_SAL_MEM_MALLOC, StdMalloc);
    BSL_SAL_CallBack_Ctrl(BSL_SAL_MEM_FREE, free);

    CRYPT_EAL_CipherCtx* ctx = CRYPT_EAL_CipherNewCtx(CRYPT_CIPHER_SM4_CBC);
    if (ctx == NULL) {
        PrintLastError();
        BSL_ERR_DeInit();
        return 1;
    }

    ret = CRYPT_EAL_CipherInit(ctx, key, sizeof(key), iv, sizeof(iv), true);
    if (ret != CRYPT_SUCCESS) {
        printf("error code is %x\n", ret);
        PrintLastError();
        goto EXIT;
    }

    ret = CRYPT_EAL_CipherSetPadding(ctx, CRYPT_PADDING_PKCS7);
    if (ret != CRYPT_SUCCESS) {
        printf("error code is %x\n", ret);
        PrintLastError();
        goto EXIT;
    }

    TimeSpec encrypt_start, encrypt_end;
    uint64_t encrypt_total_us = 0;

    start_timer(&encrypt_start);
    ret = CRYPT_EAL_CipherUpdate(ctx, data, dataLen, cipherText, &outLen);
    start_timer(&encrypt_end);
    encrypt_total_us += get_elapsed_us(&encrypt_start, &encrypt_end);

    if (ret != CRYPT_SUCCESS) {
        printf("error code is %x\n", ret);
        PrintLastError();
        goto EXIT;
    }

    outTotalLen += outLen;
    outLen = sizeof(cipherText) - outTotalLen;

    start_timer(&encrypt_start);
    ret = CRYPT_EAL_CipherFinal(ctx, cipherText + outTotalLen, &outLen);
    start_timer(&encrypt_end);
    encrypt_total_us += get_elapsed_us(&encrypt_start, &encrypt_end);

    if (ret != CRYPT_SUCCESS) {
        printf("error code is %x\n", ret);
        PrintLastError();
        goto EXIT;
    }

    outTotalLen += outLen;
    cipherTextLen = outTotalLen;
    printf("Encryption took %lu microseconds\n", encrypt_total_us);

    // ========== 解密阶段：精确测量纯计算时间 ==========
    // 初始化屏障
    pthread_barrier_init(&g_barrier, NULL, THREAD_NUM);
    g_threads_completed = 0;

    pthread_t threads[THREAD_NUM];
    ThreadDecryptData threadData[THREAD_NUM];

    for (uint32_t i = 0; i < THREAD_NUM; i++) {
        threadData[i] = (ThreadDecryptData){
            .cipherText = cipherText,
            .cipherLen = cipherTextLen,
            .plainText = plainText,
            .key = key,
            .iv = iv,
            .index = i,
            .totalThreads = THREAD_NUM
        };
        pthread_create(&threads[i], NULL, thread_decrypt, &threadData[i]);
    }

    for (uint32_t i = 0; i < THREAD_NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_barrier_destroy(&g_barrier);

    uint64_t decrypt_total_us = get_elapsed_us(&g_decrypt_start, &g_decrypt_end);
    printf("Decryption (pure compute) took %lu microseconds\n", decrypt_total_us);
    // =================================================

    // 填充验证与结果比对（与原代码完全相同）
    uint32_t blockNum = cipherTextLen / 16;
    if (blockNum == 0) goto EXIT;

    uint8_t pad = plainText[blockNum * 16 - 1];
    int valid_padding = 0;
    if (pad > 0 && pad <= 16) {
        valid_padding = 1;
        for (int i = 0; i < pad; i++) {
            if (plainText[blockNum * 16 - 1 - i] != pad) {
                valid_padding = 0;
                break;
            }
        }
    }

    if (!valid_padding) {
        printf("Invalid PKCS7 padding!\n");
        blockNum = 0;
    } else {
        blockNum = blockNum * 16 - pad;
    }

    if (memcmp(plainText, data, dataLen) != 0 && blockNum == dataLen) {
        printf("plaintext comparison failed\n");
    } else {
        printf("pass \n");
    }

EXIT:
    CRYPT_EAL_CipherFreeCtx(ctx);
    BSL_ERR_DeInit();
    return ret;
}
