#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <csr.h>

typedef int data_t;

#define MAX_VEC_LEN 10000
static data_t static_data[MAX_VEC_LEN];  // 静态内存分配

typedef struct {
    long len;
    data_t *data;
} vec_rec, *vec_ptr;

vec_ptr new_vec(long len) {
    static vec_rec static_vec;  // 静态结构体分配
    
    if (len <= 0 || len > MAX_VEC_LEN) {
        return NULL;
    }

    static_vec.len = len;
    static_vec.data = static_data;
    return &static_vec;
}

int get_vec_element(vec_ptr v, long index, data_t *dest) {
    if (index < 0 || index >= v->len)
        return 0;
    *dest = v->data[index];
    return 1;
}

long vec_length(vec_ptr v) {
    return v->len;
}

void combine1(vec_ptr v, data_t *dest) {
    long i;
    *dest = 0;
    for (i = 0; i < vec_length(v); i++) {
        data_t val;
        get_vec_element(v, i, &val);
        *dest = *dest + val;
    }
}


int main() {
     _ioe_init();  // 初始化 I/O 设备

    // 添加计时变量
    uint64_t start_cycle, end_cycle;

    // 记录初始时钟
    start_cycle = csr_read(CSR_MCYCLE);

    long len = 10000;
    vec_ptr v = new_vec(len);
    
    if (!v) {
        printf("Failed to create vector\n");
        return 1;
    }

    // 初始化数据
    for (long i = 0; i < len; i++) {
        v->data[i] = i + 1;
    }

    data_t result1;
    combine1(v, &result1);  // 主要计算部分

    // 记录结束时钟
    end_cycle = csr_read(CSR_MCYCLE);

    // 计算耗时
    uint64_t cycles = end_cycle - start_cycle;

    // 输出结果
    printf("combine1 result: %d\n", result1);
    printf("Cycle count: %lu\n", cycles);

    return 0;
}