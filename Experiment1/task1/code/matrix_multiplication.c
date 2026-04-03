#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_N 512

typedef struct {
    int tid;
    int num_threads;
    int n;
    double (*A)[MAX_N];
    double (*B)[MAX_N];
    double (*C)[MAX_N];
} thread_arg_t;

void* worker(void* arg) {
    thread_arg_t* t = (thread_arg_t*)arg;
    int start = t->tid * t->n / t->num_threads;
    int end = (t->tid + 1) * t->n / t->num_threads;

    for (int i = start; i < end; i++) {
        for (int j = 0; j < t->n; j++) {
            double sum = 0.0;
            for (int k = 0; k < t->n; k++) {
                sum += t->A[i][k] * t->B[k][j];
            }
            t->C[i][j] = sum;
        }
    }
    return NULL;
}

double get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main(int argc, char* argv[]) {
    int n = 256;
    int num_threads = 4;

    if (argc >= 2) n = atoi(argv[1]);
    if (argc >= 3) num_threads = atoi(argv[2]);

    if (n <= 0 || n > MAX_N) {
        printf("Matrix size N must be between 1 and %d\n", MAX_N);
        return 1;
    }
    if (num_threads <= 0) {
        printf("Number of threads must be positive\n");
        return 1;
    }

    static double A[MAX_N][MAX_N];
    static double B[MAX_N][MAX_N];
    static double C[MAX_N][MAX_N];

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            A[i][j] = (i + j) % 10;
            B[i][j] = (i * j) % 10;
            C[i][j] = 0.0;
        }
    }

    pthread_t threads[num_threads];
    thread_arg_t args[num_threads];

    double start_time = get_time_ms();

    for (int t = 0; t < num_threads; t++) {
        args[t].tid = t;
        args[t].num_threads = num_threads;
        args[t].n = n;
        args[t].A = A;
        args[t].B = B;
        args[t].C = C;

        if (pthread_create(&threads[t], NULL, worker, &args[t]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }

    for (int t = 0; t < num_threads; t++) {
        pthread_join(threads[t], NULL);
    }

    double end_time = get_time_ms();

    printf("Matrix size: %d x %d\n", n, n);
    printf("Threads: %d\n", num_threads);
    printf("Execution time: %.3f ms\n", end_time - start_time);
    printf("C[0][0] = %.2f, C[%d][%d] = %.2f\n", C[0][0], n-1, n-1, C[n-1][n-1]);

    return 0;
}