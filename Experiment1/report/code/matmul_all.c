#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <immintrin.h>  // AVX2指令集（WSL专用）

// 数据类型定义
typedef double matrix_t;
#define FLOP_PER_MULT 2.0  // 一次乘法+加法=2次浮点运算

// ===================== 全局测试配置 =====================
// 测试矩阵尺寸 (M,K,N)
const int test_sizes[][3] = {
    {1024, 1024, 1024},
    {2048, 2048, 2048},
    {4096, 4096, 4096}
};
#define SIZE_COUNT (sizeof(test_sizes)/sizeof(test_sizes[0]))

// 测试线程数（适配6核CPU）
const int test_threads[] = {1, 2, 4, 6};
#define THREAD_COUNT (sizeof(test_threads)/sizeof(test_threads[0]))

// 测试分块大小（适配你的L1/L2缓存）
const int test_blocks[] = {32, 64, 128, 256};
#define BLOCK_COUNT (sizeof(test_blocks)/sizeof(test_blocks[0]))

// 多线程参数结构体
typedef struct {
    int start, end;
    int M, K, N;
    matrix_t *A, *B, *C;
    int block_size;
} ThreadData;

// ===================== 工具函数 =====================
// 矩阵初始化
void mat_init(int rows, int cols, matrix_t *mat) {
    for (int i = 0; i < rows*cols; i++) mat[i] = 1.0;
}

// 矩阵清零
void mat_zero(int rows, int cols, matrix_t *mat) {
    memset(mat, 0, rows*cols*sizeof(matrix_t));
}

// 矩阵转置（版本2/8专用）
matrix_t* mat_transpose(int K, int N, const matrix_t *B) {
    matrix_t *B_T = (matrix_t*)malloc(K*N*sizeof(matrix_t));
    for (int i = 0; i < K; i++)
        for (int j = 0; j < N; j++)
            B_T[j*K + i] = B[i*N + j];
    return B_T;
}

// 计时函数
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 计算GFLOPS
double calc_gflops(int M, int K, int N, double time) {
    return (FLOP_PER_MULT * M * K * N) / time / 1e9;
}

// ===================== 版本0：基础Naive算法（i-j-k） =====================
void matmul_v0(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C) {
    for (int i = 0; i < M; i++)
        for (int j = 0; j < N; j++)
            for (int k = 0; k < K; k++)
                C[i*N+j] += A[i*K+k] * B[k*N+j];
}

// ===================== 版本1：IKJ循环重排（访存优化） =====================
void matmul_v1(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C) {
    for (int i = 0; i < M; i++)
        for (int k = 0; k < K; k++) {
            matrix_t a = A[i*K+k];
            for (int j = 0; j < N; j++)
                C[i*N+j] += a * B[k*N+j];
        }
}

// ===================== 版本2：矩阵转置 + IKJ =====================
void matmul_v2(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C) {
    matrix_t *B_T = mat_transpose(K, N, B);
    for (int i = 0; i < M; i++)
        for (int k = 0; k < K; k++) {
            matrix_t a = A[i*K+k];
            for (int j = 0; j < N; j++)
                C[i*N+j] += a * B_T[j*K+k];
        }
    free(B_T);
}

// ===================== 版本3：单线程分块优化 =====================
void matmul_v3(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C, int block) {
    for (int i0 = 0; i0 < M; i0+=block)
        for (int k0 = 0; k0 < K; k0+=block)
            for (int j0 = 0; j0 < N; j0+=block)
                for (int i = i0; i < i0+block && i<M; i++)
                    for (int k = k0; k < k0+block && k<K; k++) {
                        matrix_t a = A[i*K+k];
                        for (int j = j0; j < j0+block && j<N; j++)
                            C[i*N+j] += a * B[k*N+j];
                    }
}

// ===================== 版本4：多线程IKJ（并行优化） =====================
void* thread_v4(void *arg) {
    ThreadData *d = (ThreadData*)arg;
    for (int i = d->start; i < d->end; i++)
        for (int k = 0; k < d->K; k++) {
            matrix_t a = d->A[i*d->K + k];
            for (int j = 0; j < d->N; j++)
                d->C[i*d->N + j] += a * d->B[k*d->N + j];
        }
    free(d);
    return NULL;
}
void matmul_v4(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C, int threads) {
    pthread_t *pt = malloc(threads * sizeof(pthread_t));
    int rows = M / threads;
    for (int i = 0; i < threads; i++) {
        ThreadData *d = malloc(sizeof(ThreadData));
        d->start = i*rows;
        d->end = (i==threads-1) ? M : (i+1)*rows;
        d->M=M, d->K=K, d->N=N, d->A=A, d->B=B, d->C=C;
        pthread_create(&pt[i], NULL, thread_v4, d);
    }
    for (int i = 0; i < threads; i++) pthread_join(pt[i], NULL);
    free(pt);
}

// ===================== 版本5：多线程+分块（核心优化） =====================
void* thread_v5(void *arg) {
    ThreadData *d = (ThreadData*)arg;
    int block = d->block_size;
    for (int i0 = d->start; i0 < d->end; i0+=block)
        for (int k0 = 0; k0 < d->K; k0+=block)
            for (int j0 = 0; j0 < d->N; j0+=block)
                for (int i = i0; i < i0+block && i<d->end; i++)
                    for (int k = k0; k < k0+block && k<d->K; k++) {
                        matrix_t a = d->A[i*d->K + k];
                        for (int j = j0; j < j0+block && j<d->N; j++)
                            d->C[i*d->N + j] += a * d->B[k*d->N + j];
                    }
    free(d);
    return NULL;
}
void matmul_v5(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C, int threads, int block) {
    pthread_t *pt = malloc(threads * sizeof(pthread_t));
    int rows = M / threads;
    for (int i = 0; i < threads; i++) {
        ThreadData *d = malloc(sizeof(ThreadData));
        d->start = i*rows;
        d->end = (i==threads-1) ? M : (i+1)*rows;
        d->M=M, d->K=K, d->N=N, d->A=A, d->B=B, d->C=C;
        d->block_size = block;
        pthread_create(&pt[i], NULL, thread_v5, d);
    }
    for (int i = 0; i < threads; i++) pthread_join(pt[i], NULL);
    free(pt);
}

// ===================== 版本8：全优化（多线程+转置+分块） =====================
void matmul_v8(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C, int threads, int block) {
    matrix_t *B_T = mat_transpose(K, N, B);
    pthread_t *pt = malloc(threads * sizeof(pthread_t));
    int rows = M / threads;
    for (int i = 0; i < threads; i++) {
        ThreadData *d = malloc(sizeof(ThreadData));
        d->start = i*rows;
        d->end = (i==threads-1) ? M : (i+1)*rows;
        d->M=M, d->K=K, d->N=N, d->A=A, d->B=B_T, d->C=C;
        d->block_size = block;
        pthread_create(&pt[i], NULL, thread_v5, d);
    }
    for (int i = 0; i < threads; i++) pthread_join(pt[i], NULL);
    free(pt);
    free(B_T);
}

// ===================== 版本6/9：循环展开+向量化 =====================
void matmul_v6(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C) {
    for (int i = 0; i < M; i++)
        for (int k = 0; k < K; k++) {
            matrix_t a = A[i*K+k];
            int j;
            // 循环展开4次，提升流水线效率
            for (j = 0; j <= N-4; j+=4) {
                C[i*N+j] += a * B[k*N+j];
                C[i*N+j+1] += a * B[k*N+j+1];
                C[i*N+j+2] += a * B[k*N+j+2];
                C[i*N+j+3] += a * B[k*N+j+3];
            }
            for (; j < N; j++) C[i*N+j] += a * B[k*N+j];
        }
}

// ===================== 版本10：手动AVX2向量化（WSL专用） =====================
void matmul_v10(int M, int K, int N, matrix_t *A, matrix_t *B, matrix_t *C) {
    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            __m256d a = _mm256_broadcast_sd(&A[i*K + k]);
            int j;
            for (j = 0; j <= N-4; j+=4) {
                __m256d b = _mm256_loadu_pd(&B[k*N + j]);
                __m256d c = _mm256_loadu_pd(&C[i*N + j]);
                c = _mm256_fmadd_pd(a, b, c);
                _mm256_storeu_pd(&C[i*N + j], c);
            }
            for (; j < N; j++) C[i*N+j] += A[i*K+k] * B[k*N+j];
        }
    }
    _mm256_zeroupper();
}

// ===================== 统一自动测试函数（全版本遍历） =====================
void test_all_versions() {
    printf("==================== 矩阵乘法全版本优化测试结果 ====================\n");
    printf("CPU：6核 | L1d=48KiB | L2=1.25MiB | L3=12MiB | 数据类型：double\n");
    printf("测试维度：1024/2048/4096 | 线程：1/2/4/6 | 分块：32/64/128/256\n");
    printf("=====================================================================\n");
    printf("%-8s\t%-6s\t%-6s\t%-8s\t%-8s\t%-10s\t%s\n",
           "版本","M","线程","分块","耗时(s)","GFLOPS","说明");
    printf("=====================================================================\n");

    // 遍历所有矩阵尺寸
    for (int s = 0; s < SIZE_COUNT; s++) {
        int M = test_sizes[s][0];
        int K = test_sizes[s][1];
        int N = test_sizes[s][2];

        // 分配内存
        matrix_t *A = malloc(M*K*sizeof(matrix_t));
        matrix_t *B = malloc(K*N*sizeof(matrix_t));
        matrix_t *C = malloc(M*N*sizeof(matrix_t));
        mat_init(M, K, A);
        mat_init(K, N, B);

        double t_start, t_cost, gflops;

        // -------- 版本0：基础版 --------
        mat_zero(M, N, C);
        t_start = get_time();
        matmul_v0(M, K, N, A, B, C);
        t_cost = get_time() - t_start;
        gflops = calc_gflops(M,K,N,t_cost);
        printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\t基础Naive\n",
               "v0",M,1,"-",t_cost,gflops);

        // -------- 版本1：IKJ --------
        mat_zero(M, N, C);
        t_start = get_time();
        matmul_v1(M, K, N, A, B, C);
        t_cost = get_time() - t_start;
        gflops = calc_gflops(M,K,N,t_cost);
        printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\tIKJ重排\n",
               "v1",M,1,"-",t_cost,gflops);

        // -------- 版本2：转置+IKJ --------
        mat_zero(M, N, C);
        t_start = get_time();
        matmul_v2(M, K, N, A, B, C);
        t_cost = get_time() - t_start;
        gflops = calc_gflops(M,K,N,t_cost);
        printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\t矩阵转置\n",
               "v2",M,1,"-",t_cost,gflops);

        // -------- 版本3：单线程分块（遍历所有分块） --------
        for (int b = 0; b < BLOCK_COUNT; b++) {
            int block = test_blocks[b];
            mat_zero(M, N, C);
            t_start = get_time();
            matmul_v3(M, K, N, A, B, C, block);
            t_cost = get_time() - t_start;
            gflops = calc_gflops(M,K,N,t_cost);
            printf("%-8s\t%-6d\t%-6d\t%-8d\t%-8.2f\t%-10.2f\t单线程分块\n",
                   "v3",M,1,block,t_cost,gflops);
        }

        // -------- 版本4：多线程IKJ（遍历所有线程） --------
        for (int t = 0; t < THREAD_COUNT; t++) {
            int threads = test_threads[t];
            mat_zero(M, N, C);
            t_start = get_time();
            matmul_v4(M, K, N, A, B, C, threads);
            t_cost = get_time() - t_start;
            gflops = calc_gflops(M,K,N,t_cost);
            printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\t多线程IKJ\n",
                   "v4",M,threads,"-",t_cost,gflops);
        }

        // -------- 版本5：多线程+分块（遍历线程+分块） --------
        for (int t = 0; t < THREAD_COUNT; t++) {
            for (int b = 0; b < BLOCK_COUNT; b++) {
                int threads = test_threads[t];
                int block = test_blocks[b];
                mat_zero(M, N, C);
                t_start = get_time();
                matmul_v5(M, K, N, A, B, C, threads, block);
                t_cost = get_time() - t_start;
                gflops = calc_gflops(M,K,N,t_cost);
                printf("%-8s\t%-6d\t%-6d\t%-8d\t%-8.2f\t%-10.2f\t线程+分块\n",
                       "v5",M,threads,block,t_cost,gflops);
            }
        }

        // -------- 版本8：全优化（线程+分块+转置） --------
        for (int t = 0; t < THREAD_COUNT; t++) {
            for (int b = 0; b < BLOCK_COUNT; b++) {
                int threads = test_threads[t];
                int block = test_blocks[b];
                mat_zero(M, N, C);
                t_start = get_time();
                matmul_v8(M, K, N, A, B, C, threads, block);
                t_cost = get_time() - t_start;
                gflops = calc_gflops(M,K,N,t_cost);
                printf("%-8s\t%-6d\t%-6d\t%-8d\t%-8.2f\t%-10.2f\t全优化\n",
                       "v8",M,threads,block,t_cost,gflops);
            }
        }

        // -------- 版本6：循环展开 --------
        mat_zero(M, N, C);
        t_start = get_time();
        matmul_v6(M, K, N, A, B, C);
        t_cost = get_time() - t_start;
        gflops = calc_gflops(M,K,N,t_cost);
        printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\t循环展开\n",
               "v6",M,1,"-",t_cost,gflops);

        // -------- 版本10：AVX2向量化（WSL专用） --------
        mat_zero(M, N, C);
        t_start = get_time();
        matmul_v10(M, K, N, A, B, C);
        t_cost = get_time() - t_start;
        gflops = calc_gflops(M,K,N,t_cost);
        printf("%-8s\t%-6d\t%-6d\t%-8s\t%-8.2f\t%-10.2f\tAVX2手动\n",
               "v10",M,1,"-",t_cost,gflops);

        // 释放内存
        free(A); free(B); free(C);
    }
}

// ===================== 主函数 =====================
int main() {
    test_all_versions();
    return 0;
}