import os
import re

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns


DATA_FILES = [
    "wsl_arch.csv",
    "sm4_qemu_results.csv",
]

OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))
STATISTICS_OUTPUT_PATH = os.path.join(OUTPUT_DIR, "performance_statistics.csv")
QEMU_STATISTICS_OUTPUT_PATH = os.path.join(OUTPUT_DIR, "qemu_riscv_statistics.csv")
QEMU_SUMMARY_TEX_PATH = os.path.join(OUTPUT_DIR, "..", "content", "result_qemu_summary.tex")

CORE_NUMERIC_COLUMNS = [
    "threads",
    "data_len",
    "loop",
    "time_s",
    "data_bytes",
    "throughput_MBps",
    "throughput_MiBps",
]

VARIANT_ORDER = [
    "single_original",
    "single_table",
    "single_riscv_zksed",
    "multibuffer_bitslice",
    "multibuffer_original",
    "multibuffer_table",
    "decrypt_parallel_original",
    "decrypt_parallel_table",
    "multibuffer_simd",
]

VARIANT_NAMES = {
    "single_original": "Original (Single)",
    "single_table": "T-table (Single)",
    "single_riscv_zksed": "Zksed (Single)",
    "multibuffer_bitslice": "Bitslice (32-way)",
    "multibuffer_original": "Original (Multi-buffer)",
    "multibuffer_table": "T-table (Multi-buffer)",
    "decrypt_parallel_original": "Decrypt Original (Parallel)",
    "decrypt_parallel_table": "Decrypt T-table (Parallel)",
    "multibuffer_simd": "SIMD (x86 only)",
}

PLATFORM_NAMES = {
    "qemu-riscv": "QEMU RISC-V (rv64)",
    "wsl-arch": "WSL-Arch (x86_64)",
}

VARIANT_COLORS = {
    "single_original": "#b83b5e",
    "single_table": "#2a9d8f",
    "single_riscv_zksed": "#e9c46a",
    "multibuffer_bitslice": "#4c6e91",
    "multibuffer_original": "#7b6d8d",
    "multibuffer_table": "#a05a2c",
    "decrypt_parallel_original": "#c06c84",
    "decrypt_parallel_table": "#355c7d",
    "multibuffer_simd": "#1f9d8a",
}

OPT_COLORS = {
    "O0": "#6c757d",
    "O1": "#457b9d",
    "O2": "#2a9d8f",
    "O3": "#e76f51",
}

THREAD_VARIANTS = {
    "multibuffer_original",
    "multibuffer_table",
    "decrypt_parallel_original",
    "decrypt_parallel_table",
}

SINGLE_STREAM_VARIANTS = {
    "single_original",
    "single_table",
    "single_riscv_zksed",
}


sns.set_style("whitegrid")
plt.rcParams.update({
    "font.size": 11,
    "font.family": "DejaVu Sans",
    "axes.titlesize": 13,
    "axes.labelsize": 12,
    "xtick.labelsize": 10,
    "ytick.labelsize": 10,
    "legend.fontsize": 9,
    "figure.dpi": 300,
    "figure.figsize": (12, 7),
    "axes.grid": True,
    "grid.alpha": 0.25,
    "grid.linestyle": "--",
    "savefig.bbox": "tight",
    "savefig.pad_inches": 0.12,
})


def sanitize_filename(name: str) -> str:
    return re.sub(r"[^\w\-_.]", "_", name)


def save_fig(filename: str):
    path = os.path.join(OUTPUT_DIR, filename)
    plt.savefig(path, dpi=300, bbox_inches="tight")
    print(f"  Saved: {path}")
    plt.close()


def display_variant_name(variant: str) -> str:
    return VARIANT_NAMES.get(variant, variant)


def ordered_variants(values) -> list:
    values = list(values)
    known = [variant for variant in VARIANT_ORDER if variant in values]
    unknown = sorted([variant for variant in values if variant not in VARIANT_ORDER])
    return known + unknown


def load_and_preprocess_data(file_names: list) -> pd.DataFrame:
    df_list = []

    for file_name in file_names:
        file_path = os.path.join(OUTPUT_DIR, file_name)
        if not os.path.exists(file_path):
            print(f"Warning: file not found - {file_path}")
            continue

        df = pd.read_csv(file_path)
        df["source_file"] = file_name
        print(f"Loaded {file_name}: {len(df)} rows")
        df_list.append(df)

    if not df_list:
        raise FileNotFoundError("No input CSV files were found in the data directory.")

    df = pd.concat(df_list, ignore_index=True)

    for col in CORE_NUMERIC_COLUMNS:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")

    if "verify" in df.columns:
        invalid_mask = (~df["verify"].isna()) & (df["verify"] != "ok")
        invalid_count = int(invalid_mask.sum())
        if invalid_count > 0:
            print(f"Dropping {invalid_count} rows with explicit verify failure")
            df = df.loc[~invalid_mask].copy()

    required_cols = ["target", "variant", "opt", "time_s", "data_bytes"]
    df = df.dropna(subset=required_cols).copy()

    if "throughput_MBps" not in df.columns or df["throughput_MBps"].isna().all():
        df["throughput_MBps"] = df["data_bytes"] / df["time_s"] / 1_000_000.0

    if "throughput_MiBps" not in df.columns or df["throughput_MiBps"].isna().all():
        df["throughput_MiBps"] = df["data_bytes"] / df["time_s"] / 1024.0 / 1024.0

    df["threads"] = df["threads"].fillna(1).astype(int)
    df["time_per_mib"] = df["time_s"] / (df["data_bytes"] / (1024.0 * 1024.0))
    df["time_per_mb"] = df["time_s"] / (df["data_bytes"] / 1_000_000.0)
    df["variant_label"] = df["variant"].map(VARIANT_NAMES).fillna(df["variant"])
    df["platform_label"] = df["target"].map(PLATFORM_NAMES).fillna(df["target"])

    print(f"\nLoaded valid rows: {len(df)}")
    print(f"Platforms: {sorted(df['target'].dropna().unique().tolist())}")
    print(f"Variants: {ordered_variants(df['variant'].dropna().unique().tolist())}")
    print(f"Opts: {sorted(df['opt'].dropna().unique().tolist())}")

    return df


def save_statistics(df: pd.DataFrame):
    summary = (
        df.groupby(["target", "variant", "opt", "threads"], dropna=False)
        .agg(
            runs=("time_s", "count"),
            mean_time_s=("time_s", "mean"),
            mean_time_per_mib=("time_per_mib", "mean"),
            mean_throughput_MBps=("throughput_MBps", "mean"),
            mean_data_bytes=("data_bytes", "mean"),
        )
        .reset_index()
    )
    summary.to_csv(STATISTICS_OUTPUT_PATH, index=False)
    print(f"Saved: {STATISTICS_OUTPUT_PATH}")

    qemu = summary[summary["target"] == "qemu-riscv"].copy()
    qemu.to_csv(QEMU_STATISTICS_OUTPUT_PATH, index=False)
    print(f"Saved: {QEMU_STATISTICS_OUTPUT_PATH}")


def plot_qemu_single_stream_time(df_qemu: pd.DataFrame):
    subset = df_qemu[
        (df_qemu["variant"].isin(SINGLE_STREAM_VARIANTS)) &
        (df_qemu["threads"] == 1)
    ].copy()
    if subset.empty:
        print("Skip: no qemu single-stream data")
        return

    opts = sorted(subset["opt"].unique())
    variants = ordered_variants(subset["variant"].unique())
    x = np.arange(len(opts))
    width = 0.24 if len(variants) >= 3 else 0.32

    fig, ax = plt.subplots(figsize=(12, 6.5))
    for idx, variant in enumerate(variants):
        vdata = subset[subset["variant"] == variant]
        summary = vdata.groupby("opt")["time_per_mib"].mean().reindex(opts)
        offset = (idx - (len(variants) - 1) / 2.0) * width
        ax.bar(
            x + offset,
            summary.values,
            width=width,
            label=display_variant_name(variant),
            color=VARIANT_COLORS.get(variant, "#888888"),
            edgecolor="black",
            linewidth=0.5,
        )

    ax.set_xticks(x)
    ax.set_xticklabels(opts)
    ax.set_xlabel("Compiler Optimization Level", fontweight="bold")
    ax.set_ylabel("Execution Time per MiB (s)", fontweight="bold")
    ax.set_title("QEMU RISC-V Single-Stream Time Cost by Optimization Level", fontweight="bold")
    ax.legend(frameon=True)
    save_fig("qemu_single_time_per_mib_by_opt.png")


def plot_qemu_instruction_optimization(df_qemu: pd.DataFrame):
    subset = df_qemu[
        (df_qemu["variant"].isin(SINGLE_STREAM_VARIANTS)) &
        (df_qemu["threads"] == 1)
    ].copy()
    if subset.empty:
        print("Skip: no qemu instruction-optimization data")
        return

    fig, ax = plt.subplots(figsize=(11.5, 6.5))
    for variant in ordered_variants(subset["variant"].unique()):
        summary = (
            subset[subset["variant"] == variant]
            .groupby("opt")["time_per_mib"]
            .mean()
            .reindex(sorted(subset["opt"].unique()))
        )
        ax.plot(
            summary.index,
            summary.values,
            marker="o",
            linewidth=2.3,
            markersize=7,
            label=display_variant_name(variant),
            color=VARIANT_COLORS.get(variant, "#888888"),
        )

    ax.set_xlabel("Compiler Optimization Level", fontweight="bold")
    ax.set_ylabel("Execution Time per MiB (s)", fontweight="bold")
    ax.set_title("QEMU RISC-V Instruction-Oriented Optimization Comparison", fontweight="bold")
    ax.legend(frameon=True)
    save_fig("qemu_instruction_optimization_line.png")


def plot_qemu_thread_scaling(df_qemu: pd.DataFrame):
    subset = df_qemu[
        (df_qemu["variant"].isin(THREAD_VARIANTS)) &
        (df_qemu["opt"] == "O3")
    ].copy()
    if subset.empty:
        print("Skip: no qemu thread-scaling data")
        return

    fig, ax = plt.subplots(figsize=(12, 7))
    for variant in ordered_variants(subset["variant"].unique()):
        summary = (
            subset[subset["variant"] == variant]
            .groupby("threads")["time_per_mib"]
            .mean()
            .sort_index()
        )
        ax.plot(
            summary.index,
            summary.values,
            marker="o",
            linewidth=2.2,
            markersize=7,
            label=display_variant_name(variant),
            color=VARIANT_COLORS.get(variant, "#888888"),
        )

    ax.set_xlabel("Threads", fontweight="bold")
    ax.set_ylabel("Execution Time per MiB (s)", fontweight="bold")
    ax.set_title("QEMU RISC-V O3 Thread Scaling by Implementation", fontweight="bold")
    ax.set_xticks(sorted(subset["threads"].unique()))
    ax.legend(frameon=True)
    save_fig("qemu_thread_scaling_time_per_mib_o3.png")


def plot_qemu_best_variant_summary(df_qemu: pd.DataFrame):
    if df_qemu.empty:
        print("Skip: no qemu data for best-variant summary")
        return

    grouped = (
        df_qemu.groupby(["variant", "opt", "threads"], dropna=False)
        .agg(
            mean_time_per_mib=("time_per_mib", "mean"),
            mean_throughput_MBps=("throughput_MBps", "mean"),
        )
        .reset_index()
    )

    best_rows = (
        grouped.sort_values(["variant", "mean_time_per_mib"])
        .groupby("variant", as_index=False)
        .first()
    )
    if best_rows.empty:
        print("Skip: no qemu best rows")
        return

    best_rows = best_rows.sort_values("mean_time_per_mib", ascending=True)

    fig, ax = plt.subplots(figsize=(12.5, 7))
    bars = ax.bar(
        np.arange(len(best_rows)),
        best_rows["mean_time_per_mib"],
        color=[VARIANT_COLORS.get(v, "#888888") for v in best_rows["variant"]],
        edgecolor="black",
        linewidth=0.5,
    )

    ax.set_xticks(np.arange(len(best_rows)))
    ax.set_xticklabels(
        [display_variant_name(v) for v in best_rows["variant"]],
        rotation=22,
        ha="right",
    )
    ax.set_ylabel("Best Execution Time per MiB (s)", fontweight="bold")
    ax.set_title("QEMU RISC-V Best Time Cost Reached by Each Variant", fontweight="bold")

    for bar, (_, row) in zip(bars, best_rows.iterrows()):
        ax.text(
            bar.get_x() + bar.get_width() / 2.0,
            bar.get_height(),
            f"{row['opt']}, t={int(row['threads'])}",
            ha="center",
            va="bottom",
            fontsize=8,
            rotation=0,
        )

    save_fig("qemu_best_time_per_mib_by_variant.png")


def plot_qemu_opt_heatmap(df_qemu: pd.DataFrame):
    if df_qemu.empty:
        print("Skip: no qemu data for optimization heatmap")
        return

    pivot = (
        df_qemu.groupby(["variant", "opt"], dropna=False)["time_per_mib"]
        .min()
        .unstack("opt")
    )
    if pivot.empty:
        print("Skip: qemu optimization heatmap pivot is empty")
        return

    pivot = pivot.reindex(ordered_variants(pivot.index), axis=0)
    pivot = pivot.reindex(sorted(pivot.columns), axis=1)

    fig, ax = plt.subplots(figsize=(10.5, 6.5))
    sns.heatmap(
        pivot,
        annot=True,
        fmt=".3f",
        cmap="YlOrRd_r",
        linewidths=0.5,
        cbar_kws={"label": "Min Time per MiB (s)"},
        ax=ax,
    )
    ax.set_yticklabels([display_variant_name(v) for v in pivot.index], rotation=0)
    ax.set_xlabel("Compiler Optimization Level", fontweight="bold")
    ax.set_ylabel("Variant", fontweight="bold")
    ax.set_title("QEMU RISC-V Minimum Time per MiB Heatmap", fontweight="bold")
    save_fig("qemu_opt_heatmap_time_per_mib.png")


def plot_wsl_simd_supplement(df: pd.DataFrame):
    subset = df[
        (df["target"] == "wsl-arch") &
        (df["variant"].isin(["single_original", "single_table", "multibuffer_simd"])) &
        (df["opt"].isin(["O0", "O1", "O2", "O3"]))
    ].copy()
    if subset.empty or "multibuffer_simd" not in subset["variant"].unique():
        print("Skip: no SIMD supplemental data")
        return

    grouped = (
        subset.groupby(["variant", "opt"], dropna=False)["time_per_mib"]
        .mean()
        .reset_index()
    )
    fig, ax = plt.subplots(figsize=(11.5, 6.5))
    for variant in ordered_variants(grouped["variant"].unique()):
        summary = (
            grouped[grouped["variant"] == variant]
            .set_index("opt")["time_per_mib"]
            .reindex(["O0", "O1", "O2", "O3"])
        )
        ax.plot(
            summary.index,
            summary.values,
            marker="o",
            linewidth=2.2,
            markersize=7,
            label=display_variant_name(variant),
            color=VARIANT_COLORS.get(variant, "#888888"),
        )

    ax.set_xlabel("Compiler Optimization Level", fontweight="bold")
    ax.set_ylabel("Execution Time per MiB (s)", fontweight="bold")
    ax.set_title("WSL Supplemental: x86 SIMD Optimization Trend", fontweight="bold")
    ax.legend(frameon=True)
    save_fig("wsl_simd_supplement_time_per_mib.png")


def generate_qemu_summary_tex(df_qemu: pd.DataFrame):
    grouped = (
        df_qemu.groupby(["variant", "opt", "threads"], dropna=False)
        .agg(
            time_per_mib=("time_per_mib", "mean"),
            throughput_MBps=("throughput_MBps", "mean"),
        )
        .reset_index()
        .sort_values(["variant", "opt", "threads"])
    )

    lines = [
        r"\section{QEMU RISC-V实验结果汇总}",
        r"% Auto-generated by data/analyze.py",
        r"\begin{longtable}{llrrr}",
        r"\caption{QEMU RISC-V平台SM4-CBC性能汇总（按实现、优化级别与线程数）}\\",
        r"\hline",
        r"\textbf{Variant} & \textbf{Opt} & \textbf{Threads} & \textbf{Time/MiB (s)} & \textbf{Throughput (MB/s)} \\",
        r"\hline",
        r"\endfirsthead",
        r"\hline",
        r"\textbf{Variant} & \textbf{Opt} & \textbf{Threads} & \textbf{Time/MiB (s)} & \textbf{Throughput (MB/s)} \\",
        r"\hline",
        r"\endhead",
    ]

    for _, row in grouped.iterrows():
        lines.append(
            f"{display_variant_name(row['variant'])} & {row['opt']} & "
            f"{int(row['threads'])} & {row['time_per_mib']:.4f} & {row['throughput_MBps']:.2f} \\\\"
        )

    lines.extend([r"\hline", r"\end{longtable}", ""])
    os.makedirs(os.path.dirname(QEMU_SUMMARY_TEX_PATH), exist_ok=True)
    with open(QEMU_SUMMARY_TEX_PATH, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"Saved: {QEMU_SUMMARY_TEX_PATH}")


def main():
    print("=" * 68)
    print("  SM4-CBC RISC-V Focused Analysis & Visualization Tool")
    print("=" * 68)

    merged = load_and_preprocess_data(DATA_FILES)
    save_statistics(merged)

    qemu = merged[merged["target"] == "qemu-riscv"].copy()
    print(f"\nQEMU RISC-V rows: {len(qemu)}")

    print(f"\n--- Generating QEMU-focused figures in {OUTPUT_DIR} ---")
    plot_qemu_single_stream_time(qemu)
    plot_qemu_instruction_optimization(qemu)
    plot_qemu_thread_scaling(qemu)
    plot_qemu_best_variant_summary(qemu)
    plot_qemu_opt_heatmap(qemu)
    plot_wsl_simd_supplement(merged)

    print("\n--- Generating QEMU summary table ---")
    generate_qemu_summary_tex(qemu)

    print("\nAnalysis complete.")


if __name__ == "__main__":
    main()
