#!/bin/sh
set -eu

# Guest-side script. Run this inside QEMU RISC-V Linux.
# It assumes all benchmark binaries have already been copied to /tmp.

OUT="${1:-sm4_qemu_results.csv}"
TARGET="${TARGET:-qemu-riscv}"
COMPILER="${COMPILER:-riscv64-unknown-linux-gnu-gcc}"
OPT_LEVELS="${OPT_LEVELS:-O0 O1 O2 O3}"
THREAD_COUNTS="${THREAD_COUNTS:-1 2 4 8}"

cd "$(dirname "$0")"
chmod +x ./* 2>/dev/null || true

if [ ! -f "$OUT" ]; then
    echo "timestamp,target,compiler,opt,variant,mode,threads,data_len,loop,time_s,data_bytes,throughput_MBps,throughput_MiBps,verify,checksum,binary,cflags" > "$OUT"
fi

field_first_word() {
    key="$1"
    printf '%s\n' "$TEXT" | awk -F': ' -v k="$key" '$1 == k {print $2; exit}' | awk '{print $1}'
}

field_any_first_word() {
    key1="$1"
    key2="$2"
    printf '%s\n' "$TEXT" | awk -F': ' -v a="$key1" -v b="$key2" '$1 == a || $1 == b {print $2; exit}' | awk '{print $1}'
}

field_text() {
    key="$1"
    printf '%s\n' "$TEXT" | awk -F': ' -v k="$key" '$1 == k {print $2; exit}'
}

checksum_field() {
    printf '%s\n' "$TEXT" | awk -F': ' '$1 == "cipher_checksum" || $1 == "plain_checksum" {print $2; exit}'
}

mbps_field() {
    printf '%s\n' "$TEXT" | awk '/MB\/s$/ {print $2; exit}'
}

mibps_field() {
    printf '%s\n' "$TEXT" | awk '/MiB\/s$/ {print $2; exit}'
}

run_one() {
    bin="$1"
    opt="$2"
    variant="$3"
    mode="$4"
    threads="$5"

    if [ ! -f "$bin" ]; then
        echo "skip missing binary: $bin" >&2
        return
    fi

    echo "running: $bin" >&2
    if ! TEXT=$(./"$bin" 2>&1); then
        echo "$TEXT" >&2
        echo "failed: $bin" >&2
        return
    fi

    ts=$(date '+%Y-%m-%dT%H:%M:%S' 2>/dev/null || echo "unknown")
    data_len=$(field_any_first_word "data_len" "data_len_per_thread")
    loop_count=$(field_any_first_word "loop" "loop_per_thread")
    time_s=$(field_first_word "time")
    data_bytes=$(field_first_word "data")
    mbps=$(mbps_field)
    mibps=$(mibps_field)
    verify=$(field_text "verify")
    checksum=$(checksum_field)
    cflags="compiled_on_host"

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$ts" "$TARGET" "$COMPILER" "$opt" "$variant" "$mode" "$threads" \
        "$data_len" "$loop_count" "$time_s" "$data_bytes" "$mbps" "$mibps" \
        "$verify" "$checksum" "$bin" "$cflags" >> "$OUT"
}

for opt in $OPT_LEVELS; do
    run_one "single_original_${opt}" "$opt" "single_original" "cbc_encrypt_single" "1"
    run_one "single_table_${opt}" "$opt" "single_table" "cbc_encrypt_single" "1"
    run_one "single_riscv_zksed_${opt}" "$opt" "single_riscv_zksed" "cbc_encrypt_single" "1"
    run_one "multibuffer_bitslice_${opt}" "$opt" "multibuffer_bitslice" "cbc_encrypt_multi_buffer" "256"

    for threads in $THREAD_COUNTS; do
        run_one "multibuffer_original_${opt}_t${threads}" "$opt" "multibuffer_original" "cbc_encrypt_multi_buffer" "$threads"
        run_one "multibuffer_table_${opt}_t${threads}" "$opt" "multibuffer_table" "cbc_encrypt_multi_buffer" "$threads"
        run_one "decrypt_parallel_original_${opt}_t${threads}" "$opt" "decrypt_parallel_original" "cbc_decrypt_parallel" "$threads"
        run_one "decrypt_parallel_table_${opt}_t${threads}" "$opt" "decrypt_parallel_table" "cbc_decrypt_parallel" "$threads"
    done
done

echo "benchmark finished: $OUT"
