#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <csr.h>

typedef int data_t;

#define MAX_VEC_LEN 10000
static data_t static_data[MAX_VEC_LEN];

typedef struct {
    long len;
    data_t *data;
} vec_rec, *vec_ptr;

vec_ptr new_vec(long len) {
    static vec_rec static_vec;

    if (len <= 0 || len > MAX_VEC_LEN) {
        return NULL;
    }

    static_vec.len = len;
    static_vec.data = static_data;
    return &static_vec;
}

/* 优化四：提高并行性，使用 8 路累加器 */
void combine_optimized4(vec_ptr v, data_t *dest) {
    long length = v->len;
    data_t *data = v->data;

    long i = 0;
    long limit = length - (length % 8);

    data_t sum0 = 0;
    data_t sum1 = 0;
    data_t sum2 = 0;
    data_t sum3 = 0;
    data_t sum4 = 0;
    data_t sum5 = 0;
    data_t sum6 = 0;
    data_t sum7 = 0;

    for (; i < limit; i += 8) {
        sum0 += data[i];
        sum1 += data[i + 1];
        sum2 += data[i + 2];
        sum3 += data[i + 3];
        sum4 += data[i + 4];
        sum5 += data[i + 5];
        sum6 += data[i + 6];
        sum7 += data[i + 7];
    }

    data_t sum = sum0 + sum1 + sum2 + sum3 +
                 sum4 + sum5 + sum6 + sum7;

    for (; i < length; i++) {
        sum += data[i];
    }

    *dest = sum;
}

int main() {
    _ioe_init();

    long len = 10000;
    vec_ptr v = new_vec(len);

    if (!v) {
        printf("Failed to create vector\n");
        return 1;
    }

    for (long i = 0; i < len; i++) {
        v->data[i] = i + 1;
    }

    data_t result;
    uint64_t start_cycle, end_cycle, cycles;

    start_cycle = csr_read(CSR_MCYCLE);
    combine_optimized4(v, &result);
    end_cycle = csr_read(CSR_MCYCLE);

    cycles = end_cycle - start_cycle;

    printf("optimized4 result: %d\n", result);
    printf("optimized4 cycle count: %lu\n", cycles);

    return 0;
}