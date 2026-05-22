#!/usr/bin/env python3
import argparse
import csv
import os
import re
import shlex
import subprocess
from datetime import datetime
from pathlib import Path


ROOT = Path(__file__).resolve().parent


VARIANTS = [
    {
        "name": "single_original",
        "source": "sm4_single_stream_original.c",
        "mode": "cbc_encrypt_single",
        "pthread": False,
        "threaded": False,
    },
    {
        "name": "single_table",
        "source": "sm4_single_stream_table.c",
        "mode": "cbc_encrypt_single",
        "pthread": False,
        "threaded": False,
        "targets": ["wsl-arch", "qemu-riscv"],
    },
    {
        "name": "single_riscv_zksed",
        "source": "sm4_single_stream_riscv_zksed.c",
        "mode": "cbc_encrypt_single",
        "pthread": False,
        "threaded": False,
        "targets": ["qemu-riscv"],
    },
    {
        "name": "multibuffer_bitslice",
        "source_by_target": {
            "wsl-arch": "sm4_multibuffer_bitslice_x86.c",
            "qemu-riscv": "sm4_multibuffer_bitslice_rv64.c",
        },
        "mode": "cbc_encrypt_multi_buffer",
        "pthread": False,
        "threaded": False,
        "targets": ["wsl-arch", "qemu-riscv"],
        "data_len_scale": 32,
        "loop_scale": 10,
        "reported_threads": 256,
        "extra_cflags_by_target": {
            "wsl-arch": ["-mavx2"],
            "qemu-riscv": ["-march=rv64gcv", "-mabi=lp64d"],
        },
    },
    {
        "name": "multibuffer_simd",
        "source": "sm4_multibuffer_simd.c",
        "mode": "cbc_encrypt_multi_buffer",
        "pthread": False,
        "threaded": False,
        "targets": ["wsl-arch"],
        "extra_cflags": ["-mavx2"],
        "reported_threads": 8,
    },
    {
        "name": "multibuffer_original",
        "source": "sm4_multibuffer_pthread.c",
        "mode": "cbc_encrypt_multi_buffer",
        "pthread": True,
        "threaded": True,
        "targets": ["wsl-arch", "qemu-riscv"],
    },
    {
        "name": "multibuffer_table",
        "source": "sm4_multibuffer_table_pthread.c",
        "mode": "cbc_encrypt_multi_buffer",
        "pthread": True,
        "threaded": True,
        "targets": ["wsl-arch", "qemu-riscv"],
    },
    {
        "name": "decrypt_parallel_original",
        "source": "sm4_cbc_decrypt_parallel.c",
        "mode": "cbc_decrypt_parallel",
        "pthread": True,
        "threaded": True,
        "targets": ["wsl-arch", "qemu-riscv"],
    },
    {
        "name": "decrypt_parallel_table",
        "source": "sm4_cbc_decrypt_table_parallel.c",
        "mode": "cbc_decrypt_parallel",
        "pthread": True,
        "threaded": True,
        "targets": ["wsl-arch", "qemu-riscv"],
    },
]


CSV_FIELDS = [
    "timestamp",
    "target",
    "compiler",
    "opt",
    "variant",
    "mode",
    "threads",
    "data_len",
    "loop",
    "time_s",
    "data_bytes",
    "throughput_MBps",
    "throughput_MiBps",
    "verify",
    "checksum",
    "binary",
    "cflags",
]


def split_csv_list(value):
    return [item.strip() for item in value.split(",") if item.strip()]


def parse_output(text):
    result = {}
    patterns = {
        "version": r"^version:\s*(.+)$",
        "threads": r"^threads:\s*(\d+)$",
        "data_len": r"^(?:data_len|data_len_per_thread):\s*(\d+)\s+bytes$",
        "loop": r"^(?:loop|loop_per_thread):\s*(\d+)$",
        "time_s": r"^time:\s*([0-9.]+)\s+s$",
        "data_bytes": r"^data:\s*([0-9.]+)\s+bytes$",
        "throughput_MBps": r"^throughput:\s*([0-9.]+)\s+MB/s$",
        "throughput_MiBps": r"^throughput:\s*([0-9.]+)\s+MiB/s$",
        "verify": r"^verify:\s*(.+)$",
    }

    for key, pattern in patterns.items():
        match = re.search(pattern, text, re.MULTILINE)
        if match:
            result[key] = match.group(1).strip()

    checksum = re.search(r"^(?:cipher_checksum|plain_checksum):\s*([0-9a-fA-F]+)$", text, re.MULTILINE)
    if checksum:
        result["checksum"] = checksum.group(1).strip()

    return result


def compile_binary(args, variant, opt, threads):
    source = variant.get("source_by_target", {}).get(args.target, variant.get("source"))
    src = ROOT / source
    suffix = f"{variant['name']}_{opt}"
    if variant["threaded"]:
        suffix += f"_t{threads}"
    binary = args.build_dir / suffix

    data_len = args.data_len
    loop = args.loop
    if "data_len_scale" in variant:
        data_len = max(16, (data_len // variant["data_len_scale"]) // 16 * 16)
    if "loop_scale" in variant:
        loop = max(1, loop // variant["loop_scale"])

    cmd = [
        args.compiler,
        "-std=c11",
        f"-{opt}",
        "-Wall",
        "-Wextra",
        f"-DDATA_LEN={data_len}",
        f"-DLOOP={loop}",
    ]

    if variant["threaded"]:
        cmd.append(f"-DTHREADS={threads}")

    if args.static:
        cmd.append("-static")

    if variant["pthread"]:
        cmd.append("-pthread")

    cmd.extend(variant.get("extra_cflags", []))
    cmd.extend(variant.get("extra_cflags_by_target", {}).get(args.target, []))
    cmd.extend(args.extra_cflags)
    cmd.extend([str(src), "-o", str(binary)])

    subprocess.run(cmd, cwd=ROOT, check=True)
    return binary, cmd


def append_csv(path, row):
    path.parent.mkdir(parents=True, exist_ok=True)
    exists = path.exists()
    with path.open("a", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=CSV_FIELDS)
        if not exists:
            writer.writeheader()
        writer.writerow(row)


def build_row(args, variant, opt, threads, binary, compile_cmd, parsed):
    reported_threads = threads if variant["threaded"] else variant.get("reported_threads", 1)
    return {
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "target": args.target,
        "compiler": args.compiler,
        "opt": opt,
        "variant": variant["name"],
        "mode": variant["mode"],
        "threads": parsed.get("threads", reported_threads),
        "data_len": parsed.get("data_len", args.data_len),
        "loop": parsed.get("loop", args.loop),
        "time_s": parsed.get("time_s", ""),
        "data_bytes": parsed.get("data_bytes", ""),
        "throughput_MBps": parsed.get("throughput_MBps", ""),
        "throughput_MiBps": parsed.get("throughput_MiBps", ""),
        "verify": parsed.get("verify", ""),
        "checksum": parsed.get("checksum", ""),
        "binary": binary.name,
        "cflags": " ".join(shlex.quote(part) for part in compile_cmd),
    }


def shell_quote(value):
    return "'" + str(value).replace("'", "'\"'\"'") + "'"


def write_qemu_runner(path, jobs, target, output_name):
    lines = [
        "#!/bin/sh",
        "set -eu",
        f"OUT=${{1:-{shell_quote(output_name)}}}",
        "if [ ! -f \"$OUT\" ]; then",
        "  echo 'timestamp,target,compiler,opt,variant,mode,threads,data_len,loop,time_s,data_bytes,throughput_MBps,throughput_MiBps,verify,checksum,binary,cflags' > \"$OUT\"",
        "fi",
        "extract() {",
        "  key=\"$1\"",
        "  printf '%s\\n' \"$TEXT\" | awk -F': ' -v k=\"$key\" '$1 == k {print $2; exit}' | awk '{print $1}'",
        "}",
        "extract_any_checksum() {",
        "  printf '%s\\n' \"$TEXT\" | awk -F': ' '$1 == \"cipher_checksum\" || $1 == \"plain_checksum\" {print $2; exit}'",
        "}",
    ]

    for job in jobs:
        binary_ref = "./" + job["binary"].name
        lines.extend([
            f"echo 'running {job['binary'].name}' >&2",
            f"TEXT=$({shell_quote(binary_ref)})",
            "TIME_S=$(extract time)",
            "DATA_LEN=$(printf '%s\\n' \"$TEXT\" | awk -F': ' '$1 == \"data_len\" || $1 == \"data_len_per_thread\" {print $2; exit}' | awk '{print $1}')",
            "LOOP_N=$(printf '%s\\n' \"$TEXT\" | awk -F': ' '$1 == \"loop\" || $1 == \"loop_per_thread\" {print $2; exit}' | awk '{print $1}')",
            "DATA_BYTES=$(extract data)",
            "MBPS=$(printf '%s\\n' \"$TEXT\" | awk '/MB\\/s$/ {print $2; exit}')",
            "MIBPS=$(printf '%s\\n' \"$TEXT\" | awk '/MiB\\/s$/ {print $2; exit}')",
            "VERIFY=$(extract verify)",
            "CHECKSUM=$(extract_any_checksum)",
            "TS=$(date '+%Y-%m-%dT%H:%M:%S')",
            "printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\\n' \\",
            f"  \"$TS\" {shell_quote(target)} {shell_quote(job['compiler'])} {shell_quote(job['opt'])} {shell_quote(job['variant'])} {shell_quote(job['mode'])} {shell_quote(job['threads'])} \\",
            f"  \"$DATA_LEN\" \"$LOOP_N\" \"$TIME_S\" \"$DATA_BYTES\" \"$MBPS\" \"$MIBPS\" \"$VERIFY\" \"$CHECKSUM\" {shell_quote(job['binary'].name)} {shell_quote(job['cflags'])} >> \"$OUT\"",
        ])

    path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    os.chmod(path, 0o755)


def selected_variants(names):
    if names == ["all"]:
        return VARIANTS

    selected = []
    known = {variant["name"]: variant for variant in VARIANTS}
    for name in names:
        if name not in known:
            raise SystemExit(f"unknown variant: {name}")
        selected.append(known[name])
    return selected


def main():
    parser = argparse.ArgumentParser(description="Build and run SM4-CBC benchmark variants.")
    parser.add_argument("--target", default="wsl-arch", help="Label written to CSV, e.g. wsl-arch or qemu-riscv.")
    parser.add_argument("--compiler", default="gcc", help="C compiler, e.g. gcc or riscv64-unknown-linux-gnu-gcc.")
    parser.add_argument("--opt", default="O0,O1,O2,O3", help="Comma-separated optimization levels.")
    parser.add_argument("--threads", default="1,2,4,8", help="Comma-separated thread counts for threaded variants.")
    parser.add_argument("--variants", default="all", help="Comma-separated variants or all.")
    parser.add_argument("--data-len", type=int, default=1024 * 1024, help="Bytes processed per CBC stream.")
    parser.add_argument("--loop", type=int, default=100, help="Loop count per benchmark.")
    parser.add_argument("--output", type=Path, default=ROOT / "results" / "sm4_benchmark.csv")
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build_sm4_bench")
    parser.add_argument("--extra-cflags", nargs="*", default=[])
    parser.add_argument("--static", action="store_true", help="Add -static when building.")
    parser.add_argument("--no-run", action="store_true", help="Only build binaries.")
    parser.add_argument("--qemu-runner", type=Path, default=None, help="Generate a POSIX shell runner for copied binaries.")
    args = parser.parse_args()

    opts = split_csv_list(args.opt)
    thread_counts = [int(item) for item in split_csv_list(args.threads)]
    variants = selected_variants(split_csv_list(args.variants))
    args.build_dir.mkdir(parents=True, exist_ok=True)

    qemu_jobs = []
    for opt in opts:
        for variant in variants:
            if args.target not in variant.get("targets", ["wsl-arch", "qemu-riscv"]):
                continue
            threads_to_build = thread_counts if variant["threaded"] else [1]
            for threads in threads_to_build:
                binary, compile_cmd = compile_binary(args, variant, opt, threads)
                cflags_text = " ".join(shlex.quote(part) for part in compile_cmd)
                qemu_jobs.append({
                    "binary": binary,
                    "compiler": args.compiler,
                    "opt": opt,
                    "variant": variant["name"],
                    "mode": variant["mode"],
                    "threads": threads if variant["threaded"] else variant.get("reported_threads", 1),
                    "cflags": cflags_text,
                })

                if args.no_run:
                    print(f"built {binary}")
                    continue

                completed = subprocess.run([str(binary)], cwd=args.build_dir, check=True,
                                           text=True, capture_output=True)
                print(completed.stdout, end="")
                parsed = parse_output(completed.stdout)
                row = build_row(args, variant, opt, threads, binary, compile_cmd, parsed)
                append_csv(args.output, row)

    if args.qemu_runner is not None:
        runner_path = args.qemu_runner
        if not runner_path.is_absolute():
            runner_path = args.build_dir / runner_path
        write_qemu_runner(runner_path, qemu_jobs, args.target, "sm4_qemu_results.csv")
        print(f"wrote qemu runner: {runner_path}")


if __name__ == "__main__":
    main()
