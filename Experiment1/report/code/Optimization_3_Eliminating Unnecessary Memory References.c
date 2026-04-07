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

/* 优化三：局部累加 + 4 路展开 + 指针遍历 */
void combine_optimized3(vec_ptr v, data_t *dest) {
    long length = v->len;
    data_t *ptr = v->data;

    data_t sum0 = 0;
    data_t sum1 = 0;
    data_t sum2 = 0;
    data_t sum3 = 0;

    long cnt = length / 4;
    while (cnt--) {
        sum0 += ptr[0];
        sum1 += ptr[1];
        sum2 += ptr[2];
        sum3 += ptr[3];
        ptr += 4;
    }

    data_t sum = sum0 + sum1 + sum2 + sum3;

    long remain = length % 4;
    while (remain--) {
        sum += *ptr;
        ptr++;
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
    combine_optimized3(v, &result);
    end_cycle = csr_read(CSR_MCYCLE);

    cycles = end_cycle - start_cycle;

    printf("optimized3 result: %d\n", result);
    printf("optimized3 cycle count: %lu\n", cycles);

    return 0;
}