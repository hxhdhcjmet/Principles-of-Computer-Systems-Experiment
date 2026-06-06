# ============================================================================
# SM4 Benchmark Makefile
#
# Usage on RISC-V board (OrangePi / LicheePi):
#   make                    # cross-compile for RISC-V
#   make native             # compile with native (host) GCC for testing
#   make clean              # remove build artifacts
#
# Usage on host (for code verification before deploying to board):
#   make native
#   ./build/sm4_benchmark
# ============================================================================

# ---- Toolchain ----
CROSS_COMPILE ?= riscv64-linux-gnu-
CC_CROSS      = $(CROSS_COMPILE)gcc
CC_NATIVE     = gcc

# ---- Compiler flags ----
# -O2: standard optimization; -march=rv64gc for RISC-V GC extension
# (remove -march if your board requires a different ISA string)
CFLAGS_CROSS  = -O2 -march=rv64gc -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L
CFLAGS_NATIVE = -O2 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L

# When building benchmark (linking multiple .c files), suppress standalone main()
BENCH_CFLAGS  = -DSM4_AS_LIBRARY

# ---- Directories ----
SRCDIR   = .
BUILDDIR = build

# ---- Source files ----
COMMON_SRCS = sm4_default_loop.c sm4_unrolled.c sm4_reduced_dep.c
BENCH_SRCS  = sm4_benchmark.c $(COMMON_SRCS)
STANDALONE_TARGETS = sm4_default_loop sm4_unrolled sm4_reduced_dep
BENCH_TARGET       = sm4_benchmark

# ---- Build targets ----
.PHONY: all native clean standalone

all: $(BUILDDIR) $(BUILDDIR)/$(BENCH_TARGET)

native: CC = $(CC_NATIVE)
native: CFLAGS = $(CFLAGS_NATIVE)
native: $(BUILDDIR) $(BUILDDIR)/$(BENCH_TARGET)
	@echo "=== Native build complete. Run: ./$(BUILDDIR)/$(BENCH_TARGET) ==="

standalone: $(BUILDDIR)
	@for t in $(STANDALONE_TARGETS); do \
		$(CC_CROSS) $(CFLAGS_CROSS) -o $(BUILDDIR)/$$t $(SRCDIR)/$$t.c -lm; \
		echo "  CC [cross] $$t"; \
	done
	@echo "=== Standalone targets built in $(BUILDDIR)/ ==="

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# ---- Benchmark (links all scheme .c files) ----
$(BUILDDIR)/$(BENCH_TARGET): $(addprefix $(SRCDIR)/,$(BENCH_SRCS))
	$(CC_CROSS) $(CFLAGS_CROSS) $(BENCH_CFLAGS) -o $@ $^ -lm
	@echo "  CC [cross] $(BENCH_TARGET)"

# ---- Individual standalone compilations (for debugging individual schemes) ----
$(BUILDDIR)/sm4_default_loop: $(SRCDIR)/sm4_default_loop.c
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_unrolled: $(SRCDIR)/sm4_unrolled.c
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_reduced_dep: $(SRCDIR)/sm4_reduced_dep.c
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

clean:
	rm -rf $(BUILDDIR) sm4_results.txt sm4_results.csv
	@echo "=== Clean complete ==="
