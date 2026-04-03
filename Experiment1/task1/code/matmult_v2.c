#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

// 矩阵维度定义：A(M x K) * B(K x N) = C(M x N)
//该版本矩阵一维展开
int M = 1024; // A的行数，C的行数
int K = 1024; // A的列数，B的行数
int N = 1024; // B的列数，C的列数

int num_threads = 4; // 默认线程数
int block_size = 64; // 分块大小

// 全局指针，稍后在 main 中动态分配内存（防止栈溢出）
double *A, *B, *C;

// 计时辅助函数
double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 辅助函数：每次测试前清空 C 矩阵
void clear_C() {
    memset(C, 0, M * N * sizeof(double));
}

// ==================== 版本 0：朴素版本 (Naive i-j-k) ====================
void matmul_naive() {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < K; k++) {
                sum += A[i * K + k] * B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// ==================== 版本 1：循环重排 (Loop Permutation i-k-j) ====================
void matmul_ikj() {
    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            double r = A[i * K + k]; // 提取出不变的项
            for (int j = 0; j < N; j++) {
                C[i * N + j] += r * B[k * N + j]; // 此时B和C的访问都是连续的
            }
        }
    }
}

// ==================== 版本 2：矩阵转置 (Transpose B) ====================
void matmul_transpose() {
    // 动态分配 B 的转置矩阵空间：大小为 N x K
    double *B_T = (double *)malloc(N * K * sizeof(double));
    
    // 1. 求转置
    for (int k = 0; k < K; k++) {
        for (int j = 0; j < N; j++) {
            B_T[j * K + k] = B[k * N + j];
        }
    }
    
    // 2. 利用转置矩阵相乘
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            double sum = 0.0;
            for (int k = 0; k < K; k++) {
                // 此时 A 和 B_T 都是连续访问
                sum += A[i * K + k] * B_T[j * K + k];
            }
            C[i * N + j] = sum;
        }
    }
    free(B_T);
}

// ==================== 版本 3：分块矩阵 (Cache Blocking) ====================
// 使用 min 宏处理维度不能被 block_size 整除的边界情况
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void matmul_blocked() {
    for (int i = 0; i < M; i += block_size) {
        for (int k = 0; k < K; k += block_size) {
            for (int j = 0; j < N; j += block_size) {
                // 内部小块的 i-k-j 乘法
                for (int ii = i; ii < MIN(i + block_size, M); ii++) {
                    for (int kk = k; kk < MIN(k + block_size, K); kk++) {
                        double r = A[ii * K + kk];
                        for (int jj = j; jj < MIN(j + block_size, N); jj++) {
                            C[ii * N + jj] += r * B[kk * N + jj];
                        }
                    }
                }
            }
        }
    }
}

// ==================== 版本 4：多线程 + IKJ优化 (Multi-threaded) ====================
typedef struct {
    int start_row;
    int end_row;
} thread_data_t;

void* thread_func(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    // 在分配给该线程的行范围内，使用最高效的 i-k-j 顺序计算
    for (int i = data->start_row; i < data->end_row; i++) {
        for (int k = 0; k < K; k++) {
            double r = A[i * K + k];
            for (int j = 0; j < N; j++) {
                C[i * N + j] += r * B[k * N + j];
            }
        }
    }
    return NULL;
}
void matmul_multithread() {
    pthread_t threads[num_threads];
    thread_data_t tdata[num_threads];

    int rows_per_thread = M / num_threads;
    for (int t = 0; t < num_threads; t++) {
        tdata[t].start_row = t * rows_per_thread;
        // 最后一个线程处理剩余的所有行（防止 M 不能被 num_threads 整除）
        tdata[t].end_row = (t == num_threads - 1) ? M : (t + 1) * rows_per_thread;
        pthread_create(&threads[t], NULL, thread_func, &tdata[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}

// =======多线程+分块=======
void* thread_func_blocked(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // 线程只负责自己那一部分行 (start_row 到 end_row)
    for (int i = data->start_row; i < data->end_row; i += block_size) {
        for (int k = 0; k < K; k += block_size) {
            for (int j = 0; j < N; j += block_size) {
                // 核心分块计算
                for (int ii = i; ii < MIN(i + block_size, data->end_row); ii++) {
                    for (int kk = k; kk < MIN(k + block_size, K); kk++) {
                        double r = A[ii * K + kk];
                        for (int jj = j; jj < MIN(j + block_size, N); jj++) {
                            C[ii * N + jj] += r * B[kk * N + jj];
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

void matmul_multithread_block() {
    pthread_t threads[num_threads];
    thread_data_t tdata[num_threads];

    int rows_per_thread = M / num_threads;
    for (int t = 0; t < num_threads; t++) {
        tdata[t].start_row = t * rows_per_thread;
        // 最后一个线程处理剩余的所有行（防止 M 不能被 num_threads 整除）
        tdata[t].end_row = (t == num_threads - 1) ? M : (t + 1) * rows_per_thread;
        pthread_create(&threads[t], NULL, thread_func_blocked, &tdata[t]);
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}


//=========IJK+循环展开优化========
void matmul_ikj_unroll() {
    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            double r = A[i * K + k];
            int j;
            // 循环展开4次，一次性计算4个元素
            for (j = 0; j <= N - 4; j += 4) {
                C[i * N + j]     += r * B[k * N + j];
                C[i * N + j + 1] += r * B[k * N + j + 1];
                C[i * N + j + 2] += r * B[k * N + j + 2];
                C[i * N + j + 3] += r * B[k * N + j + 3];
            }
            // 处理剩余不足4个的元素
            for (; j < N; j++) {
                C[i * N + j] += r * B[k * N + j];
            }
        }
    }
}


// ==================== 新增版本7：分块 + 转置 融合优化 ====================
void matmul_block_transpose() {
    // 分配转置矩阵B_T
    double *B_T = (double *)malloc(N * K * sizeof(double));
    // 对B进行转置
    for (int k = 0; k < K; k++) {
        for (int j = 0; j < N; j++) {
            B_T[j * K + k] = B[k * N + j];
        }
    }
    // 分块计算 + 转置矩阵（连续访问+Cache优化）
    for (int i = 0; i < M; i += block_size) {
        for (int k = 0; k < K; k += block_size) {
            for (int j = 0; j < N; j += block_size) {
                for (int ii = i; ii < MIN(i + block_size, M); ii++) {
                    for (int kk = k; kk < MIN(k + block_size, K); kk++) {
                        double r = A[ii * K + kk];
                        for (int jj = j; jj < MIN(j + block_size, N); jj++) {
                            C[ii * N + jj] += r * B_T[jj * K + kk];
                        }
                    }
                }
            }
        }
    }
    free(B_T);
}

// ==================== 新增版本8：多线程 + 分块 + 转置 优化 ====================
// 新增线程函数（不修改原有thread_func）
void* thread_final_optimize(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    // 全局转置矩阵，只初始化一次
    static double *B_T = NULL;
    if (B_T == NULL) {
        B_T = (double *)malloc(N * K * sizeof(double));
        for (int k = 0; k < K; k++) {
            for (int j = 0; j < N; j++) {
                B_T[j * K + k] = B[k * N + j];
            }
        }
    }
    // 线程负责的行范围 + 分块+转置计算
    for (int i = data->start_row; i < data->end_row; i += block_size) {
        for (int k = 0; k < K; k += block_size) {
            for (int j = 0; j < N; j += block_size) {
                for (int ii = i; ii < MIN(i + block_size, data->end_row); ii++) {
                    for (int kk = k; kk < MIN(k + block_size, K); kk++) {
                        double r = A[ii * K + kk];
                        for (int jj = j; jj < MIN(j + block_size, N); jj++) {
                            C[ii * N + jj] += r * B_T[jj * K + kk];
                        }
                    }
                }
            }
        }
    }
    return NULL;
}

// 最终优化接口函数
void matmul_multithread_final() {
    pthread_t threads[num_threads];
    thread_data_t tdata[num_threads];
    int rows_per_thread = M / num_threads;

    for (int t = 0; t < num_threads; t++) {
        tdata[t].start_row = t * rows_per_thread;
        tdata[t].end_row = (t == num_threads - 1) ? M : (t + 1) * rows_per_thread;
        pthread_create(&threads[t], NULL, thread_final_optimize, &tdata[t]);
    }
    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }
}


// ==================== 新增版本9：编译器自动向量化优化 ====================
// 开启GCC向量化编译指令
#pragma GCC target("avx2")
#pragma GCC optimize("O3")
#pragma GCC optimize("unroll-loops")

void matmul_auto_vectorize() {
    // 核心逻辑和IKJ一致，编译器自动优化
    for (int i = 0; i < M; i++) {
        for (int k = 0; k < K; k++) {
            double r = A[i * K + k];
            for (int j = 0; j < N; j++) {
                C[i * N + j] += r * B[k * N + j];
            }
        }
    }
}




// ==================== 主函数：测试与输出 ====================
int main(int argc, char *argv[]) {
    // 允许通过命令行参数修改配置：./matmul_test <M> <K> <N> <Threads>
    if (argc >= 4) {
        M = atoi(argv[1]);
        K = atoi(argv[2]);
        N = atoi(argv[3]);
    }
    if (argc >= 5) {
        num_threads = atoi(argv[4]);
    }

    printf("=== 矩阵乘法性能测试 ===\n");
    printf("矩阵 A: [%d x %d], 矩阵 B: [%d x %d], 矩阵 C: [%d x %d]\n", M, K, K, N, M, N);
    printf("测试线程数: %d\n\n", num_threads);

    // 动态分配内存以支持大矩阵
    A = (double *)malloc(M * K * sizeof(double));
    B = (double *)malloc(K * N * sizeof(double));
    C = (double *)malloc(M * N * sizeof(double));

    // 初始化随机数据
    for (int i = 0; i < M * K; i++) A[i] = (double)rand() / RAND_MAX;
    for (int i = 0; i < K * N; i++) B[i] = (double)rand() / RAND_MAX;

    double start, end;

    // 测试 0: Naive
    clear_C();
    start = get_time();
    matmul_naive();
    end = get_time();
    printf("[版本 0] 朴素算法 (i-j-k):\t %.4f 秒\n", end - start);

    // 测试 1: IKJ 重排
    clear_C();
    start = get_time();
    matmul_ikj();
    end = get_time();
    printf("[版本 1] 循环重排 (i-k-j):\t %.4f 秒\n", end - start);

    // 测试 2: 转置 B
    clear_C();
    start = get_time();
    matmul_transpose();
    end = get_time();
    printf("[版本 2] 矩阵转置 (Transpose):\t %.4f 秒\n", end - start);

    // 测试 3: 分块优化
    clear_C();
    start = get_time();
    matmul_blocked();
    end = get_time();
    printf("[版本 3] 分块矩阵 (Block=%d):\t %.4f 秒\n", block_size, end - start);

    // 测试 4: 多线程 + IKJ
    clear_C();
    start = get_time();
    matmul_multithread();
    end = get_time();
    printf("[版本 4] 多线程 (%d Threads) + IKJ:\t %.4f 秒\n", num_threads, end - start);

    //测试5:多线程 + 分块
    clear_C();
    start = get_time();
    matmul_multithread_block();
    end = get_time();
    printf("[版本 5] 多线程 (%d Threads) + 分块:\t %.4f 秒\n", num_threads, end - start);

    //测试6:IJK + 循环展开
    clear_C();
    start = get_time();
    matmul_ikj_unroll();
    end = get_time();
    printf("[版本 6] IJK + 循环展开:\t %.4f 秒\n", end - start);
    
    // 测试7: 分块+转置融合优化
    clear_C();
    start = get_time();
    matmul_block_transpose();
    end = get_time();
    printf("[版本 7] 分块+转置:\t %.4f 秒\n", end - start);

    // 测试8: 多线程+分块+转置(全局最优)
    clear_C();
    start = get_time();
    matmul_multithread_final();
    end = get_time();
    printf("[版本 8] 全优化(并行+Cache):\t %.4f 秒\n", end - start);


    // 测试9: 编译器自动向量化
    clear_C();
    start = get_time();
    matmul_auto_vectorize();
    end = get_time();
    printf("[版本 9] 自动向量化优化:\t\t %.4f 秒\n", end - start);


    free(A); free(B); free(C);
    return 0;
}