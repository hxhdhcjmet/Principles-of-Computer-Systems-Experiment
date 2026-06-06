#!/bin/bash
# ============================================================================
# SM4 Benchmark Automation Script
#
# This script:
#   1. Cross-compiles the SM4 benchmark for RISC-V
#   2. Copies it to the target board via scp
#   3. Runs the benchmark on the board via ssh
#   4. Copies results back to the host
#
# Usage:
#   chmod +x run_benchmark.sh
#   ./run_benchmark.sh                        # use defaults
#   BOARD_IP=192.168.1.100 ./run_benchmark.sh  # specify board IP
#   BOARD_USER=orangepi BOARD_IP=... ./run_benchmark.sh
#   RUN_LOCAL=1 ./run_benchmark.sh             # run locally (for testing)
# ============================================================================

set -euo pipefail

# ============================================================================
# Configuration — override via environment variables
# ============================================================================
BOARD_USER="${BOARD_USER:-orangepi}"         # RISC-V board username
BOARD_IP="${BOARD_IP:-}"                     # RISC-V board IP/hostname (REQUIRED for remote)
BOARD_PORT="${BOARD_PORT:-22}"               # SSH port
BOARD_WORKDIR="${BOARD_WORKDIR:-/home/${BOARD_USER}/sm4_benchmark}"  # Remote work dir
RUN_LOCAL="${RUN_LOCAL:-0}"                  # Set to 1 to run locally instead of on board

# Cross-compiler
CROSS_COMPILE="${CROSS_COMPILE:-riscv64-linux-gnu-}"

# Timestamp for result file naming
TIMESTAMP=$(date '+%Y%m%d_%H%M%S')
RESULT_DIR="results_${TIMESTAMP}"

# ============================================================================
# Helper functions
# ============================================================================
log()  { echo "[$(date '+%H:%M:%S')] $*"; }
die()  { log "ERROR: $*"; exit 1; }

# ============================================================================
# Step 1: Compile
# ============================================================================
log "Step 1/4: Cross-compiling SM4 benchmark..."
make CROSS_COMPILE="${CROSS_COMPILE}" clean all || die "Compilation failed"
log "Compilation successful."

# Verify binary
BINARY="build/sm4_benchmark"
if [ ! -f "${BINARY}" ]; then
    die "Binary not found: ${BINARY}"
fi
log "Binary: ${BINARY} ($(file ${BINARY} | cut -d: -f2))"

# ============================================================================
# Step 2: Deploy to board (or skip if running locally)
# ============================================================================
if [ "${RUN_LOCAL}" = "1" ]; then
    log "Step 2/4: Running LOCALLY (RUN_LOCAL=1)"
    mkdir -p "${RESULT_DIR}"
    cd "${RESULT_DIR}"
    ../build/sm4_benchmark
    cd ..
    log "Benchmark completed locally."
    log "Results: ${RESULT_DIR}/"
    ls -la "${RESULT_DIR}/"
    exit 0
fi

if [ -z "${BOARD_IP}" ]; then
    die "BOARD_IP not set. Usage: BOARD_IP=<ip> $0"
fi

log "Step 2/4: Copying binary to ${BOARD_USER}@${BOARD_IP}:${BOARD_WORKDIR}/"
ssh -p "${BOARD_PORT}" "${BOARD_USER}@${BOARD_IP}" "mkdir -p ${BOARD_WORKDIR}" \
    || die "Cannot create remote directory"
scp -P "${BOARD_PORT}" "${BINARY}" "${BOARD_USER}@${BOARD_IP}:${BOARD_WORKDIR}/" \
    || die "scp failed"

# ============================================================================
# Step 3: Run benchmark on board
# ============================================================================
log "Step 3/4: Running benchmark on board..."
ssh -p "${BOARD_PORT}" "${BOARD_USER}@${BOARD_IP}" \
    "cd ${BOARD_WORKDIR} && chmod +x sm4_benchmark && ./sm4_benchmark" \
    || die "Benchmark execution failed"

# ============================================================================
# Step 4: Copy results back
# ============================================================================
log "Step 4/4: Retrieving results from board..."
mkdir -p "${RESULT_DIR}"

scp -P "${BOARD_PORT}" \
    "${BOARD_USER}@${BOARD_IP}:${BOARD_WORKDIR}/sm4_results.txt" \
    "${RESULT_DIR}/sm4_results_${TIMESTAMP}.txt" \
    || log "Warning: could not retrieve txt results"

scp -P "${BOARD_PORT}" \
    "${BOARD_USER}@${BOARD_IP}:${BOARD_WORKDIR}/sm4_results.csv" \
    "${RESULT_DIR}/sm4_results_${TIMESTAMP}.csv" \
    || log "Warning: could not retrieve csv results"

# ============================================================================
# Done
# ============================================================================
log "============================================"
log "Benchmark complete!"
log "Results saved to: ${RESULT_DIR}/"
ls -la "${RESULT_DIR}/" 2>/dev/null || true
log "============================================"
log ""
log "To view results:"
log "  cat ${RESULT_DIR}/sm4_results_${TIMESTAMP}.txt"
log "  cat ${RESULT_DIR}/sm4_results_${TIMESTAMP}.csv"
log ""
log "For repeated runs with different configurations, use:"
log "  BOARD_IP=<ip> $0"
