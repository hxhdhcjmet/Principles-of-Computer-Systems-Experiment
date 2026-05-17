#!/usr/bin/env bash
set -euo pipefail

# Host-side script. Run this script in WSL/Docker, not inside QEMU.
# It cross-compiles all SM4 benchmark variants, copies binaries plus the
# guest-side runner into rootfs.img:/tmp, then starts QEMU.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# --- Configurable paths ---
SRC_DIR="${SRC_DIR:-$SCRIPT_DIR}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_DIR/build_qemu_sm4}"
IMG_FILE="${IMG_FILE:-rootfs.img}"
IMG_DST_DIR="${IMG_DST_DIR:-/tmp}"

BIOS="${BIOS:-opensbi/build/platform/generic/firmware/fw_jump.bin}"
KERNEL="${KERNEL:-linux/arch/riscv/boot/Image}"

CROSS_CC="${CROSS_CC:-riscv64-unknown-linux-gnu-gcc}"
QEMU="${QEMU:-qemu-system-riscv64}"

# --- Benchmark matrix ---
OPT_LEVELS="${OPT_LEVELS:-O0 O1 O2 O3}"
THREAD_COUNTS="${THREAD_COUNTS:-1 2 4 8}"
DATA_LEN="${DATA_LEN:-1048576}"
LOOP="${LOOP:-20}"
QEMU_MEM="${QEMU_MEM:-256M}"

GUEST_RUNNER_SRC="${GUEST_RUNNER_SRC:-$SCRIPT_DIR/qemu_run_sm4_bench.sh}"
GUEST_RUNNER_NAME="${GUEST_RUNNER_NAME:-qemu_run_sm4_bench.sh}"

echo "==== Step 0: Checking paths ===="
if [ ! -d "$SRC_DIR" ]; then
    echo "error: source directory does not exist: $SRC_DIR"
    exit 1
fi

if [ ! -f "$IMG_FILE" ]; then
    echo "error: rootfs image does not exist: $IMG_FILE"
    exit 1
fi

if [ ! -f "$BIOS" ]; then
    echo "error: OpenSBI firmware does not exist: $BIOS"
    exit 1
fi

if [ ! -f "$KERNEL" ]; then
    echo "error: Linux kernel image does not exist: $KERNEL"
    exit 1
fi

if [ ! -f "$GUEST_RUNNER_SRC" ]; then
    echo "error: guest runner does not exist: $GUEST_RUNNER_SRC"
    exit 1
fi

mkdir -p "$BUILD_DIR"

echo "==== Step 1: Cross-compiling SM4 benchmark variants ===="
echo "source dir : $SRC_DIR"
echo "build dir  : $BUILD_DIR"
echo "data len   : $DATA_LEN bytes"
echo "loop       : $LOOP"
echo "opts       : $OPT_LEVELS"
echo "threads    : $THREAD_COUNTS"

EXE_FILES=()

compile_one() {
    local variant="$1"
    local source="$2"
    local mode="$3"
    local opt="$4"
    local threads="$5"
    local threaded="$6"
    local need_pthread="$7"

    local src_file="$SRC_DIR/$source"
    local exe_name="${variant}_${opt}"

    if [ "$threaded" = "yes" ]; then
        exe_name="${exe_name}_t${threads}"
    fi

    local exe_file="$BUILD_DIR/$exe_name"

    if [ ! -f "$src_file" ]; then
        echo "error: missing source file: $src_file"
        exit 1
    fi

    local cmd=(
        "$CROSS_CC"
        -std=c11
        "-$opt"
        -Wall
        -Wextra
        -static
        "-DDATA_LEN=$DATA_LEN"
        "-DLOOP=$LOOP"
    )

    if [ "$threaded" = "yes" ]; then
        cmd+=("-DTHREADS=$threads")
    fi

    if [ "$need_pthread" = "yes" ]; then
        cmd+=(-pthread)
    fi

    cmd+=("$src_file" -o "$exe_file")

    echo "compiling [$mode]: $exe_name"
    "${cmd[@]}"
    chmod +x "$exe_file"
    EXE_FILES+=("$exe_file")
}

for opt in $OPT_LEVELS; do
    compile_one "single_original" "sm4_single_stream_original.c" "cbc_encrypt_single" "$opt" "1" "no" "no"
    compile_one "single_table" "sm4_single_stream_table.c" "cbc_encrypt_single" "$opt" "1" "no" "no"

    for threads in $THREAD_COUNTS; do
        compile_one "multibuffer_original" "sm4_multibuffer_pthread.c" "cbc_encrypt_multi_buffer" "$opt" "$threads" "yes" "yes"
        compile_one "multibuffer_table" "sm4_multibuffer_table_pthread.c" "cbc_encrypt_multi_buffer" "$opt" "$threads" "yes" "yes"
        compile_one "decrypt_parallel_original" "sm4_cbc_decrypt_parallel.c" "cbc_decrypt_parallel" "$opt" "$threads" "yes" "yes"
        compile_one "decrypt_parallel_table" "sm4_cbc_decrypt_table_parallel.c" "cbc_decrypt_parallel" "$opt" "$threads" "yes" "yes"
    done
done

echo "==== Step 2: Copying binaries and runner into $IMG_FILE:$IMG_DST_DIR ===="
if ! command -v e2cp >/dev/null 2>&1; then
    echo "e2tools is not installed. Trying to install it with apt..."
    sudo apt-get update
    sudo apt-get install -y e2tools
fi

e2mkdir "$IMG_FILE:$IMG_DST_DIR" 2>/dev/null || true

copy_into_image() {
    local host_file="$1"
    local guest_name="$2"

    echo "copying: $guest_name -> $IMG_FILE:$IMG_DST_DIR/$guest_name"
    e2rm "$IMG_FILE:$IMG_DST_DIR/$guest_name" 2>/dev/null || true
    e2cp "$host_file" "$IMG_FILE:$IMG_DST_DIR/$guest_name"
}

for exe_file in "${EXE_FILES[@]}"; do
    copy_into_image "$exe_file" "$(basename "$exe_file")"
done

copy_into_image "$GUEST_RUNNER_SRC" "$GUEST_RUNNER_NAME"

echo "==== Step 3: Starting QEMU ===="
echo "Inside QEMU, run:"
echo "  cd /tmp"
echo "  sh $GUEST_RUNNER_NAME sm4_qemu_results.csv"
echo
echo "Exit QEMU with Ctrl+A then X."
sleep 1

"$QEMU" -M virt -m "$QEMU_MEM" -nographic \
    -bios "$BIOS" \
    -kernel "$KERNEL" \
    -drive file="$IMG_FILE",format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -append "root=/dev/vda rw console=ttyS0"

echo "==== Step 4: QEMU exited ===="
mkdir -p "$SCRIPT_DIR/results"
if e2ls "$IMG_FILE:$IMG_DST_DIR/sm4_qemu_results.csv" >/dev/null 2>&1; then
    e2cp "$IMG_FILE:$IMG_DST_DIR/sm4_qemu_results.csv" "$SCRIPT_DIR/results/sm4_qemu_results.csv"
    echo "copied result CSV to: $SCRIPT_DIR/results/sm4_qemu_results.csv"
else
    echo "no result CSV found in image. Did you run /tmp/$GUEST_RUNNER_NAME inside QEMU?"
fi
