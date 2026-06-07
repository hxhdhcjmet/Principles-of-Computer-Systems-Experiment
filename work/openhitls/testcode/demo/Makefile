# ============================================================================
# SM4 Benchmark Makefile
#
# Supports two ISA levels:
#   Baseline:  rv64gc (standard GCC target for RISC-V)
#   Zbb:       rv64gc_zba_zbb_zbc_zbs (bit-manipulation extension)
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
# Baseline ISA: rv64gc (RV64IMAFDC + Zicsr + Zifencei)
CFLAGS_CROSS  = -O2 -march=rv64gc -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L
# Zbb ISA: adds Zba (address), Zbb (basic bit-manip), Zbc (carry-less mul), Zbs (single-bit)
CFLAGS_ZBB    = -O2 -march=rv64gc_zba_zbb_zbc_zbs -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L
CFLAGS_NATIVE = -O2 -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=199309L

# When building benchmark (linking multiple .c files), suppress standalone main()
BENCH_CFLAGS  = -DSM4_AS_LIBRARY

# ---- Directories ----
SRCDIR   = .
BUILDDIR = build

# ---- Source files (grouped by ISA target) ----
BASELINE_SRCS = sm4_benchmark.c \
                sm4_default_loop.c \
                sm4_unrolled.c \
                sm4_reduced_dep.c \
                sm4_xbox.c \
                sm4_xbox_merged.c

ZBB_SRCS      = sm4_zbb.c \
                sm4_xbox_zbb.c

# ---- Object files ----
BASELINE_OBJS = $(addprefix $(BUILDDIR)/, $(BASELINE_SRCS:.c=.o))
ZBB_OBJS      = $(addprefix $(BUILDDIR)/, $(ZBB_SRCS:.c=.o))
ALL_OBJS      = $(BASELINE_OBJS) $(ZBB_OBJS)

# ---- Target names ----
BENCH_TARGET       = sm4_benchmark
STANDALONE_TARGETS = sm4_default_loop sm4_unrolled sm4_reduced_dep \
                     sm4_xbox sm4_xbox_merged sm4_zbb sm4_xbox_zbb

# ---- Build targets ----
.PHONY: all native clean standalone

all: $(BUILDDIR) $(BUILDDIR)/$(BENCH_TARGET)

native: CC_CROSS = $(CC_NATIVE)
native: CFLAGS_CROSS = $(CFLAGS_NATIVE)
native: CFLAGS_ZBB   = $(CFLAGS_NATIVE)
native: $(BUILDDIR) $(BUILDDIR)/$(BENCH_TARGET)
	@echo "=== Native build complete. Run: ./$(BUILDDIR)/$(BENCH_TARGET) ==="

standalone: $(BUILDDIR)
	@for t in sm4_default_loop sm4_unrolled sm4_reduced_dep sm4_xbox sm4_xbox_merged; do \
		$(CC_CROSS) $(CFLAGS_CROSS) -o $(BUILDDIR)/$$t $(SRCDIR)/$$t.c -lm; \
		echo "  CC [cross] $$t"; \
	done
	@for t in sm4_zbb sm4_xbox_zbb; do \
		$(CC_CROSS) $(CFLAGS_ZBB) -o $(BUILDDIR)/$$t $(SRCDIR)/$$t.c -lm; \
		echo "  CC [cross+Zbb] $$t"; \
	done
	@echo "=== Standalone targets built in $(BUILDDIR)/ ==="

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# ---- Benchmark: link all object files ----
$(BUILDDIR)/$(BENCH_TARGET): $(ALL_OBJS)
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $^ -lm
	@echo "  LD [cross] $(BENCH_TARGET)"

# ---- Pattern rule: baseline compilation ----
$(BUILDDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/sm4_common.h | $(BUILDDIR)
	$(CC_CROSS) $(CFLAGS_CROSS) $(BENCH_CFLAGS) -c -o $@ $<

# ---- Explicit rules: Zbb compilation (override pattern for these targets) ----
$(BUILDDIR)/sm4_zbb.o: $(SRCDIR)/sm4_zbb.c $(SRCDIR)/sm4_common.h | $(BUILDDIR)
	$(CC_CROSS) $(CFLAGS_ZBB) $(BENCH_CFLAGS) -c -o $@ $<

$(BUILDDIR)/sm4_xbox_zbb.o: $(SRCDIR)/sm4_xbox_zbb.c $(SRCDIR)/sm4_common.h | $(BUILDDIR)
	$(CC_CROSS) $(CFLAGS_ZBB) $(BENCH_CFLAGS) -c -o $@ $<

# ---- Individual standalone compilations (for debugging individual schemes) ----
$(BUILDDIR)/sm4_default_loop: $(SRCDIR)/sm4_default_loop.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_unrolled: $(SRCDIR)/sm4_unrolled.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_reduced_dep: $(SRCDIR)/sm4_reduced_dep.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_xbox: $(SRCDIR)/sm4_xbox.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_xbox_merged: $(SRCDIR)/sm4_xbox_merged.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_CROSS) -o $@ $< -lm

$(BUILDDIR)/sm4_zbb: $(SRCDIR)/sm4_zbb.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_ZBB) -o $@ $< -lm

$(BUILDDIR)/sm4_xbox_zbb: $(SRCDIR)/sm4_xbox_zbb.c $(SRCDIR)/sm4_common.h
	$(CC_CROSS) $(CFLAGS_ZBB) -o $@ $< -lm

clean:
	rm -rf $(BUILDDIR) sm4_results.txt sm4_results.csv
	@echo "=== Clean complete ==="
