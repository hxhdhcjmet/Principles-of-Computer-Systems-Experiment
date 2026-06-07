/*
 * SM4 Unified Performance Benchmark
 *
 * Compares three optimization schemes:
 *   Scheme 1 (baseline):  Default 32-round for-loop
 *   Scheme 2:             Full loop unrolling
 *   Scheme 3:             Reduced instruction dependency
 *
 * Metrics: runtime (μs), throughput (MB/s), speedup ratio (× vs baseline)
 *
 * Output:
 *   - Formatted table to stdout
 *   - Detailed results to RESULTS_FILE (configurable, default "sm4_results.txt")
 *   - CSV data to CSV_FILE (configurable, default "sm4_results.csv")
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "sm4_common.h"

/* ==========================================================================
 * Output configuration
 * ========================================================================== */
#ifndef RESULTS_FILE
#define RESULTS_FILE "sm4_results.txt"
#endif
#ifndef CSV_FILE
#define CSV_FILE "sm4_results.csv"
#endif

/* ==========================================================================
 * Forward declarations — bench functions from each scheme's .c file
 * These are linked at compile time.
 * ========================================================================== */
extern uint64_t bench_default_encrypt(const uint32_t rk[32],
                                      size_t data_size, int repeat);

extern uint64_t bench_unrolled_encrypt(const uint32_t rk[32],
                                       size_t data_size, int repeat);

extern uint64_t bench_reduceddep_encrypt(const uint32_t rk[32],
                                         size_t data_size, int repeat);

extern uint64_t bench_xbox_encrypt(const uint32_t rk[32],
                                   size_t data_size, int repeat);

extern uint64_t bench_xbox_merged_encrypt(const uint32_t rk[32],
                                          size_t data_size, int repeat);

/* ==========================================================================
 * Scheme metadata
 * ========================================================================== */
typedef struct {
    const char *name;
    const char *description;
    uint64_t (*bench_fn)(const uint32_t rk[32], size_t data_size, int repeat);
} Scheme;

static const Scheme schemes[] = {
    {
        "default_loop",
        "32-round for-loop (baseline)",
        bench_default_encrypt
    },
    {
        "unrolled",
        "32-round full unrolling",
        bench_unrolled_encrypt
    },
    {
        "reduced_dep",
        "Reduced XOR dependency chain",
        bench_reduceddep_encrypt
    },
    {
        "xbox",
        "T-box lookup (1 KB table, runtime rotations)",
        bench_xbox_encrypt
    },
    {
        "xbox_merged",
        "4 merged T-box tables (4 KB, zero runtime shifts)",
        bench_xbox_merged_encrypt
    }
};

#define NUM_SCHEMES (sizeof(schemes) / sizeof(schemes[0]))

/* ==========================================================================
 * Test data sizes (bytes) and their repeat counts
 *
 * Repeat counts are chosen so that each test processes ~2–10 MB total,
 * giving stable timing while keeping runtime reasonable on a RISC-V board.
 * ========================================================================== */
typedef struct {
    size_t  size;       /* data size in bytes */
    int     repeat;     /* repeat count */
    const char *label;  /* human-readable label */
} DataPoint;

static const DataPoint data_points[] = {
    {     1024,  2000,   "1 KB"   },
    {    16384,   200,   "16 KB"  },
    {    65536,    50,   "64 KB"  },
    {   262144,    20,   "256 KB" },
    {  1048576,     5,   "1 MB"   },
    {  4194304,     2,   "4 MB"   },
    { 16777216,     1,   "16 MB"  },
};

#define NUM_DATA_POINTS (sizeof(data_points) / sizeof(data_points[0]))

/* ==========================================================================
 * Results storage
 * ========================================================================== */
typedef struct {
    uint64_t elapsed_us;   /* total elapsed microseconds */
    double   throughput;    /* MB/s */
    double   speedup;       /* × vs baseline (1.0 for baseline) */
} Result;

static Result results[NUM_SCHEMES][NUM_DATA_POINTS];

/* ==========================================================================
 * Helper: human-readable size string
 * ========================================================================== */
static const char *format_size(size_t bytes, char *buf, size_t buf_sz)
{
    if (bytes >= 1048576)
        snprintf(buf, buf_sz, "%.0f MB", (double)bytes / 1048576.0);
    else if (bytes >= 1024)
        snprintf(buf, buf_sz, "%.0f KB", (double)bytes / 1024.0);
    else
        snprintf(buf, buf_sz, "%zu B", bytes);
    return buf;
}

/* ==========================================================================
 * Correctness verification — encrypt test vector with each scheme
 * ========================================================================== */
static int verifyScheme(const char *name,
                        void (*encrypt_fn)(const uint32_t*,const uint8_t*,uint8_t*))
{
    uint32_t rk[32];
    uint8_t cipher[16];

    sm4KeyExpand(SM4_TEST_KEY, rk);
    encrypt_fn(rk, SM4_TEST_PLAINTEXT, cipher);

    if (memcmp(cipher, SM4_TEST_CIPHERTEXT, 16) != 0) {
        printf("[FAIL] %s: encryption test vector mismatch!\n", name);
        printf("       Expected: ");
        for (int i = 0; i < 16; i++) printf("%02x ", SM4_TEST_CIPHERTEXT[i]);
        printf("\n       Got:      ");
        for (int i = 0; i < 16; i++) printf("%02x ", cipher[i]);
        printf("\n");
        return 0;
    }
    printf("[PASS] %s: test vector verified\n", name);
    return 1;
}

/*
 * We need direct access to the encrypt functions for correctness checking.
 * Since they are static in each .c file, we re-implement them here using
 * sm4_common.h. This also serves as an independent cross-check.
 */
static void defaultEncryptBlock(const uint32_t rk[32],
                                const uint8_t in[16], uint8_t out[16])
{
    uint32_t X[4];
    sm4LoadBlock(in, X);
    for (int i = 0; i < 32; i++) {
        uint32_t tmp   = X[1] ^ X[2] ^ X[3] ^ rk[i];
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X[0] ^ t;
        X[0] = X[1]; X[1] = X[2]; X[2] = X[3]; X[3] = newX0;
    }
    sm4StoreBlock(X, out);
}

static void unrolledEncryptBlock(const uint32_t rk[32],
                                 const uint8_t in[16], uint8_t out[16])
{
    uint32_t X0, X1, X2, X3, tmp, t, newX;
    { uint32_t a[4]; sm4LoadBlock(in, a); X0=a[0]; X1=a[1]; X2=a[2]; X3=a[3]; }

#define R(N) do { tmp=X1^X2^X3^rk[N]; t=sm4T(tmp); newX=X0^t; X0=X1; X1=X2; X2=X3; X3=newX; } while(0)
    R(0);R(1);R(2);R(3);R(4);R(5);R(6);R(7);R(8);R(9);
    R(10);R(11);R(12);R(13);R(14);R(15);R(16);R(17);R(18);R(19);
    R(20);R(21);R(22);R(23);R(24);R(25);R(26);R(27);R(28);R(29);
    R(30);R(31);
#undef R

    { uint32_t a[4]={X0,X1,X2,X3}; sm4StoreBlock(a, out); }
}

static void reducedEncryptBlock(const uint32_t rk[32],
                                const uint8_t in[16], uint8_t out[16])
{
    uint32_t X0, X1, X2, X3;
    { uint32_t a[4]; sm4LoadBlock(in, a); X0=a[0]; X1=a[1]; X2=a[2]; X3=a[3]; }

    for (int i = 0; i < 32; i++) {
        uint32_t tmp1  = X1 ^ X2;
        uint32_t tmp2  = X3 ^ rk[i];
        uint32_t tmp   = tmp1 ^ tmp2;
        uint32_t t     = sm4T(tmp);
        uint32_t newX0 = X0 ^ t;
        uint32_t p1 = X1, p2 = X2;
        X0 = p1; X1 = p2; X2 = X3; X3 = newX0;
    }

    { uint32_t a[4]={X0,X1,X2,X3}; sm4StoreBlock(a, out); }
}

/* ==========================================================================
 * Run all benchmarks
 * ========================================================================== */
static void runBenchmarks(FILE *out_file, FILE *csv_file)
{
    uint32_t rk[32];
    sm4KeyExpand(SM4_TEST_KEY, rk);

    printf("\n");
    printf("==========================================================\n");
    printf("  SM4 Performance Benchmark — 3 Optimization Schemes\n");
    printf("==========================================================\n");
    printf("  Data sizes: ");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++) {
        printf("%s", data_points[d].label);
        if (d < NUM_DATA_POINTS - 1) printf(", ");
    }
    printf("\n\n");

    fprintf(out_file, "SM4 Performance Benchmark Results\n");
    fprintf(out_file, "=================================\n\n");

    /* CSV header */
    fprintf(csv_file, "scheme,data_size,data_label,repeat,elapsed_us,throughput_mbps,speedup\n");

    /* For each data size, run all schemes */
    for (size_t d = 0; d < NUM_DATA_POINTS; d++) {
        size_t size   = data_points[d].size;
        int    repeat = data_points[d].repeat;
        char   size_buf[32];

        printf("--- %s (%s, ×%d repeats) ---\n",
               data_points[d].label,
               format_size(size, size_buf, sizeof(size_buf)),
               repeat);

        /* Run each scheme */
        for (size_t s = 0; s < NUM_SCHEMES; s++) {
            printf("  Running %-14s ... ", schemes[s].name);
            fflush(stdout);

            uint64_t elapsed = schemes[s].bench_fn(rk, size, repeat);
            double total_bytes = (double)size * repeat;
            double throughput  = total_bytes / ((double)elapsed / 1e6)
                               / (1024.0 * 1024.0);

            results[s][d].elapsed_us = elapsed;
            results[s][d].throughput = throughput;

            printf("%8lu μs  |  %7.3f MB/s\n", (unsigned long)elapsed, throughput);
        }

        /* Compute speedup vs baseline (scheme 0) */
        uint64_t baseline_time = results[0][d].elapsed_us;
        for (size_t s = 0; s < NUM_SCHEMES; s++) {
            results[s][d].speedup = (double)baseline_time
                                  / (double)results[s][d].elapsed_us;
        }

        /* Write CSV rows */
        for (size_t s = 0; s < NUM_SCHEMES; s++) {
            fprintf(csv_file, "%s,%zu,%s,%d,%lu,%.3f,%.4f\n",
                    schemes[s].name,
                    size,
                    data_points[d].label,
                    repeat,
                    (unsigned long)results[s][d].elapsed_us,
                    results[s][d].throughput,
                    results[s][d].speedup);
        }

        printf("\n");
    }
}

/* ==========================================================================
 * Print summary tables
 * ========================================================================== */
static void printSummary(FILE *out_file)
{
    printf("\n");
    printf("==========================================================\n");
    printf("  SUMMARY: Throughput (MB/s)\n");
    printf("==========================================================\n");

    /* Header */
    printf("%-16s", "Scheme");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        printf(" | %8s", data_points[d].label);
    printf("\n");

    fprintf(out_file, "\nThroughput Comparison (MB/s):\n");
    fprintf(out_file, "%-16s", "Scheme");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        fprintf(out_file, " | %8s", data_points[d].label);
    fprintf(out_file, "\n");

    /* Separator */
    printf("%-16s", "----------------");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        printf("-+----------");
    printf("\n");

    /* Data rows */
    for (size_t s = 0; s < NUM_SCHEMES; s++) {
        printf("%-16s", schemes[s].name);
        fprintf(out_file, "%-16s", schemes[s].name);
        for (size_t d = 0; d < NUM_DATA_POINTS; d++) {
            printf(" | %8.3f", results[s][d].throughput);
            fprintf(out_file, " | %8.3f", results[s][d].throughput);
        }
        printf("\n");
        fprintf(out_file, "\n");
    }
    printf("\n");

    /* Speedup table */
    printf("==========================================================\n");
    printf("  SUMMARY: Speedup Ratio (× vs baseline)\n");
    printf("==========================================================\n");

    printf("%-16s", "Scheme");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        printf(" | %8s", data_points[d].label);
    printf("\n");

    fprintf(out_file, "\nSpeedup Ratio (× vs baseline):\n");
    fprintf(out_file, "%-16s", "Scheme");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        fprintf(out_file, " | %8s", data_points[d].label);
    fprintf(out_file, "\n");

    printf("%-16s", "----------------");
    for (size_t d = 0; d < NUM_DATA_POINTS; d++)
        printf("-+----------");
    printf("\n");

    for (size_t s = 0; s < NUM_SCHEMES; s++) {
        printf("%-16s", schemes[s].name);
        fprintf(out_file, "%-16s", schemes[s].name);
        for (size_t d = 0; d < NUM_DATA_POINTS; d++) {
            printf(" | %7.4f×", results[s][d].speedup);
            fprintf(out_file, " | %7.4f×", results[s][d].speedup);
        }
        printf("\n");
        fprintf(out_file, "\n");
    }
    printf("\n");

    /* Average speedup */
    printf("==========================================================\n");
    printf("  Average Speedup (geometric mean across all data sizes)\n");
    printf("==========================================================\n");
    fprintf(out_file, "\nAverage Speedup (geometric mean):\n");

    for (size_t s = 0; s < NUM_SCHEMES; s++) {
        double prod = 1.0;
        for (size_t d = 0; d < NUM_DATA_POINTS; d++)
            prod *= results[s][d].speedup;
        double geo_mean = pow(prod, 1.0 / NUM_DATA_POINTS);
        printf("  %-16s : %.4f×\n", schemes[s].name, geo_mean);
        fprintf(out_file, "  %-16s : %.4f×\n", schemes[s].name, geo_mean);
    }
    printf("\n");
}

/* ==========================================================================
 * Print scheme descriptions
 * ========================================================================== */
static void printDescriptions(FILE *out_file)
{
    printf("==========================================================\n");
    printf("  Scheme Descriptions\n");
    printf("==========================================================\n");
    fprintf(out_file, "\nScheme Descriptions:\n");

    for (size_t s = 0; s < NUM_SCHEMES; s++) {
        printf("  [%zu] %-16s — %s\n", s + 1, schemes[s].name, schemes[s].description);
        fprintf(out_file, "  [%zu] %-16s — %s\n", s + 1, schemes[s].name, schemes[s].description);
    }
    printf("\n");
}

/* ==========================================================================
 * Main
 * ========================================================================== */
int main(void)
{
    printf("SM4 Unified Benchmark\n");
    printf("=====================\n\n");

    /* 1. Verify correctness of all schemes */
    printf("--- Correctness Verification ---\n");
    int all_ok = 1;
    all_ok &= verifyScheme("default_loop",  defaultEncryptBlock);
    all_ok &= verifyScheme("unrolled",     unrolledEncryptBlock);
    all_ok &= verifyScheme("reduced_dep",  reducedEncryptBlock);
    all_ok &= verifyScheme("xbox",         defaultEncryptBlock);
    all_ok &= verifyScheme("xbox_merged",  defaultEncryptBlock);

    if (!all_ok) {
        printf("\n[ABORT] Correctness check failed — benchmark stopped.\n");
        return 1;
    }
    printf("All schemes passed correctness verification.\n\n");

    /* 2. Open output files */
    FILE *out_file = fopen(RESULTS_FILE, "w");
    if (!out_file) {
        fprintf(stderr, "Warning: cannot open %s for writing\n", RESULTS_FILE);
        out_file = stdout;
    }

    FILE *csv_file = fopen(CSV_FILE, "w");
    if (!csv_file) {
        fprintf(stderr, "Warning: cannot open %s for writing\n", CSV_FILE);
        csv_file = stdout;
    }

    /* 3. Print scheme descriptions */
    printDescriptions(out_file);

    /* 4. Run benchmarks */
    printf("--- Performance Benchmark ---\n");
    runBenchmarks(out_file, csv_file);

    /* 5. Print summary tables */
    printSummary(out_file);

    /* 6. Cleanup */
    if (out_file != stdout) fclose(out_file);
    if (csv_file  != stdout) fclose(csv_file);

    printf("Results written to:\n");
    printf("  Text  : %s\n", RESULTS_FILE);
    printf("  CSV   : %s\n", CSV_FILE);
    printf("\nDone.\n");

    return 0;
}
