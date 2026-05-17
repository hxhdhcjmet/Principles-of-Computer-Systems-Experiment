# ==============================================
# SM4-CBC Encryption Performance Analysis & Visualization
# Cross-platform & cross-method comprehensive comparison
# ==============================================

# ----------------------
# 1. Import Required Libraries
# ----------------------
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
import seaborn as sns
import os
import re

# ----------------------
# 2. Configuration
# ----------------------
DATA_FILES = [
    'wsl_arch.csv',
    'sm4_qemu_results.csv'
]
OUTPUT_DIR = os.path.dirname(os.path.abspath(__file__))  # current script directory
STATISTICS_OUTPUT_PATH = os.path.join(OUTPUT_DIR, 'performance_statistics.csv')

CORE_METRICS = ['time_s', 'throughput_MBps']
GROUP_DIMENSIONS = ['target', 'variant', 'mode', 'threads', 'opt', 'compiler']

# ----------------------
# 3. Global Plot Style (English, publication-quality)
# ----------------------
sns.set_style("whitegrid")
plt.rcParams.update({
    'font.size': 11,
    'font.family': 'DejaVu Sans',
    'axes.titlesize': 13,
    'axes.labelsize': 12,
    'xtick.labelsize': 10,
    'ytick.labelsize': 10,
    'legend.fontsize': 9,
    'figure.dpi': 300,
    'figure.figsize': (12, 7),
    'axes.grid': True,
    'grid.alpha': 0.25,
    'grid.linestyle': '--',
    'savefig.bbox': 'tight',
    'savefig.pad_inches': 0.1,
})

# Unified color palette for categories
COLORS = {
    'wsl-arch': '#1f77b4',
    'qemu-riscv': '#ff7f0e',
    'single_original': '#d62728',
    'single_table': '#2ca02c',
    'multibuffer_original': '#9467bd',
    'multibuffer_table': '#8c564b',
    'decrypt_parallel_original': '#e377c2',
    'decrypt_parallel_table': '#17becf',
    'O0': '#1b9e77',
    'O1': '#d95f02',
    'O2': '#7570b3',
    'O3': '#e7298a',
}

PLATFORM_NAMES = {
    'wsl-arch': 'WSL-Arch (x86_64)',
    'qemu-riscv': 'QEMU RISC-V (rv64)',
}

VARIANT_NAMES = {
    'single_original': 'Original (Single)',
    'single_table': 'Table (Single)',
    'multibuffer_original': 'Original (Multi-buf)',
    'multibuffer_table': 'Table (Multi-buf)',
    'decrypt_parallel_original': 'Decrypt Original (Parallel)',
    'decrypt_parallel_table': 'Decrypt Table (Parallel)',
}

MODE_NAMES = {
    'cbc_encrypt_single': 'CBC Encrypt (Single Stream)',
    'cbc_encrypt_multi_buffer': 'CBC Encrypt (Multi-Buffer)',
    'cbc_decrypt_parallel': 'CBC Decrypt (Parallel)',
}


# ----------------------
# 4. Helper Functions
# ----------------------
def sanitize_filename(name: str) -> str:
    """Sanitize filename for cross-platform compatibility."""
    return re.sub(r'[^\w\-_.]', '_', name)


# ----------------------
# 5. Data Loading & Preprocessing
# ----------------------
def load_and_preprocess_data(file_paths: list) -> pd.DataFrame:
    """
    Load and merge CSV files. Fixed: only drop rows with explicitly invalid verify
    (e.g. 'fail'), NOT rows with NaN verify (multi-buffer encrypt data has no verify).
    """
    df_list = []
    for file_path in file_paths:
        if not os.path.exists(file_path):
            print(f"Warning: File not found - {file_path}")
            continue
        df = pd.read_csv(file_path)
        print(f"Loaded {file_path}: {len(df)} rows, {len(df.columns)} columns")

        # Drop rows with missing core metrics
        before = len(df)
        df = df.dropna(subset=CORE_METRICS)
        dropped = before - len(df)
        if dropped > 0:
            print(f"  Dropped {dropped} rows with missing core metrics")

        # FIXED: only filter rows where verify is explicitly 'fail'
        # NaN verify (multi-buffer encrypt) should be KEPT
        if 'verify' in df.columns:
            invalid_mask = (~df['verify'].isna()) & (df['verify'] != 'ok')
            invalid_count = invalid_mask.sum()
            if invalid_count > 0:
                print(f"  Dropping {invalid_count} rows with verify != 'ok' (explicit failure)")
                df = df[~invalid_mask]

        df_list.append(df)

    merged_df = pd.concat(df_list, ignore_index=True)

    # Summary
    print(f"\n{'='*60}")
    print(f"Merged Dataset Summary")
    print(f"{'='*60}")
    print(f"Total valid test rows: {len(merged_df)}")
    print(f"Platforms: {merged_df['target'].unique().tolist()}")
    print(f"Variants: {merged_df['variant'].unique().tolist()}")
    print(f"Modes: {merged_df['mode'].unique().tolist()}")
    print(f"Thread counts: {sorted(merged_df['threads'].unique().tolist())}")
    print(f"Optimization levels: {sorted(merged_df['opt'].unique().tolist())}")

    return merged_df


# ----------------------
# 6. Statistical Summary
# ----------------------
def generate_statistics(df: pd.DataFrame) -> pd.DataFrame:
    """Generate comprehensive statistical summary grouped by core dimensions."""
    agg_dict = {metric: ['mean', 'std', 'min', 'max', 'count'] for metric in CORE_METRICS}
    stats_df = df.groupby(GROUP_DIMENSIONS, dropna=False).agg(agg_dict).reset_index()
    stats_df.columns = [f'{col[0]}_{col[1]}' if col[1] else col[0] for col in stats_df.columns]
    stats_df.to_csv(STATISTICS_OUTPUT_PATH, index=False)
    print(f"\nStatistical summary saved to: {STATISTICS_OUTPUT_PATH}")
    return stats_df


# ----------------------
# 7. Visualization Functions
# ----------------------

def _save_fig(filename: str):
    """Helper to save figure and close."""
    path = os.path.join(OUTPUT_DIR, filename)
    plt.savefig(path, dpi=300, bbox_inches='tight')
    print(f"  Saved: {path}")
    plt.close()


def plot_platform_comparison(df: pd.DataFrame):
    """
    Fig 1: Cross-platform throughput comparison (bar chart grouped by variant & opt).
    Separate subplot per variant, bars grouped by optimization level.
    """
    for metric in ['throughput_MBps', 'time_s']:
        opt_levels = sorted(df['opt'].unique())
        x = np.arange(len(opt_levels))
        width = 0.35
        fig, ax = plt.subplots(figsize=(14, 8))

        for pi, platform in enumerate(sorted(df['target'].unique())):
            pdata = df[df['target'] == platform]
            summary = pdata.groupby('opt')[metric].agg(['mean', 'std']).reindex(opt_levels)
            ax.bar(
                x + pi * width,
                summary['mean'],
                width,
                yerr=summary['std'].fillna(0),
                capsize=4,
                label=PLATFORM_NAMES.get(platform, platform),
                color='#1f77b4' if pi == 0 else '#ff7f0e',
                alpha=0.85,
                edgecolor='black',
                linewidth=0.5,
            )

        ylabel = 'Throughput (MB/s)' if metric == 'throughput_MBps' else 'Execution Time (s)'
        ax.set_xlabel('Compiler Optimization Level', fontweight='bold')
        ax.set_ylabel(ylabel, fontweight='bold')
        ax.set_title(f'Cross-Platform {ylabel} Comparison by Optimization Level', fontweight='bold')
        ax.set_xticks(x + width / 2)
        ax.set_xticklabels(opt_levels)
        ax.legend(title='Platform', frameon=True, fancybox=True, shadow=True)
        ax.grid(axis='y', alpha=0.3, linestyle='--')

        _save_fig(f'bar_platform_{metric}_by_opt.png')


def plot_method_comparison_per_platform(df: pd.DataFrame):
    """
    Fig 2: Cross-method comparison per platform, bar chart grouped by threads.
    One figure per platform, variants as bar groups.
    """
    for platform in sorted(df['target'].unique()):
        pdata = df[df['target'] == platform].copy()

        for opt in sorted(df['opt'].unique()):
            opt_data = pdata[pdata['opt'] == opt]
            if len(opt_data) == 0:
                continue

            threads = sorted(opt_data['threads'].unique())
            variants = [v for v in sorted(opt_data['variant'].unique())]

            x = np.arange(len(variants))
            width = 0.2
            fig, ax = plt.subplots(figsize=(14, 8))

            for ti, t in enumerate(threads):
                tdata = opt_data[opt_data['threads'] == t]
                values = []
                errors = []
                for v in variants:
                    vdata = tdata[tdata['variant'] == v]
                    if len(vdata) > 0:
                        values.append(vdata['throughput_MBps'].mean())
                        errors.append(vdata['throughput_MBps'].std() if len(vdata) > 1 else 0)
                    else:
                        values.append(0)
                        errors.append(0)
                ax.bar(
                    x + ti * width,
                    values,
                    width,
                    yerr=errors,
                    capsize=3,
                    label=f'{t} Thread{"s" if t > 1 else ""}',
                    alpha=0.85,
                    edgecolor='black',
                    linewidth=0.4,
                )

            ax.set_xlabel('Method Variant', fontweight='bold')
            ax.set_ylabel('Throughput (MB/s)', fontweight='bold')
            ax.set_title(
                f'Method Comparison – {PLATFORM_NAMES.get(platform, platform)}  ({opt})',
                fontweight='bold',
            )
            ax.set_xticks(x + width * (len(threads) - 1) / 2)
            ax.set_xticklabels([VARIANT_NAMES.get(v, v) for v in variants],
                               rotation=25, ha='right')
            ax.legend(title='Threads', frameon=True, fancybox=True, shadow=True)
            ax.grid(axis='y', alpha=0.3, linestyle='--')

            _save_fig(f'bar_method_{platform}_{opt}_by_threads.png')


def plot_encrypt_vs_decrypt(df: pd.DataFrame):
    """
    Fig 3: Encryption vs Decryption throughput line plot across threads.
    Compare encrypt (multi-buffer / single) vs decrypt parallel.
    """
    for opt in sorted(df['opt'].unique()):
        fig, ax = plt.subplots(figsize=(12, 7))
        for platform, marker, ls in zip(
            sorted(df['target'].unique()),
            ['o', 's'],
            ['-', '--'],
        ):
            pdata = df[(df['target'] == platform) & (df['opt'] == opt)]

            for variant, label, lw, alpha_v in [
                ('multibuffer_table', 'Encrypt (Table Multi-Buf)', 2.5, 1.0),
                ('decrypt_parallel_table', 'Decrypt (Table Parallel)', 2.5, 1.0),
                ('multibuffer_original', 'Encrypt (Original Multi-Buf)', 1.5, 0.6),
                ('decrypt_parallel_original', 'Decrypt (Original Parallel)', 1.5, 0.6),
            ]:
                vdata = pdata[pdata['variant'] == variant].groupby('threads')['throughput_MBps']
                if vdata.count().sum() == 0:
                    continue
                summary = vdata.agg(['mean']).reset_index().sort_values('threads')
                ax.plot(
                    summary['threads'], summary['mean'],
                    marker=marker, linestyle=ls, linewidth=lw,
                    markersize=8, alpha=alpha_v,
                    label=f'{PLATFORM_NAMES.get(platform, platform)} – {label}',
                )

        ax.set_xlabel('Number of Threads', fontweight='bold')
        ax.set_ylabel('Throughput (MB/s)', fontweight='bold')
        ax.set_title(f'Encryption vs. Decryption Throughput Comparison ({opt})',
                     fontweight='bold')
        ax.set_xticks(sorted(df['threads'].unique()))
        ax.legend(bbox_to_anchor=(1.02, 1), loc='upper left',
                  frameon=True, fancybox=True, shadow=True, fontsize=8)
        ax.grid(alpha=0.3, linestyle='--')

        _save_fig(f'line_encrypt_vs_decrypt_{opt}.png')


def plot_speedup_ratio(df: pd.DataFrame):
    """
    Fig 4: Speedup ratio (throughput / single-thread throughput) vs thread count.
    """
    for platform in sorted(df['target'].unique()):
        pdata = df[df['target'] == platform].copy()

        for opt in sorted(df['opt'].unique()):
            opt_data = pdata[pdata['opt'] == opt]
            if len(opt_data) == 0:
                continue

            fig, ax = plt.subplots(figsize=(12, 7))
            all_variants = sorted(opt_data['variant'].unique())

            for variant in all_variants:
                vdata = opt_data[opt_data['variant'] == variant].sort_values('threads')
                if len(vdata[vdata['threads'] == 1]) == 0:
                    continue
                baseline = vdata[vdata['threads'] == 1]['throughput_MBps'].mean()
                if baseline == 0:
                    continue
                speedup = vdata.groupby('threads')['throughput_MBps'].mean() / baseline
                ax.plot(
                    speedup.index, speedup.values,
                    marker='D', linestyle='-', linewidth=2, markersize=8,
                    label=VARIANT_NAMES.get(variant, variant),
                )

            ax.set_xlabel('Number of Threads', fontweight='bold')
            ax.set_ylabel('Speedup Ratio (×)', fontweight='bold')
            ax.set_title(
                f'Speedup Ratio – {PLATFORM_NAMES.get(platform, platform)}  ({opt})',
                fontweight='bold',
            )
            ax.axhline(y=1.0, color='gray', linestyle=':', alpha=0.7, label='Baseline (1×)')
            ax.set_xticks(sorted(opt_data['threads'].unique()))
            ax.legend(bbox_to_anchor=(1.02, 1), loc='upper left',
                      frameon=True, fancybox=True, shadow=True, fontsize=8)
            ax.grid(alpha=0.3, linestyle='--')

            _save_fig(f'line_speedup_{platform}_{opt}.png')


def plot_optimization_impact(df: pd.DataFrame):
    """
    Fig 5: Optimization-level impact heatmap-style bar chart.
    For each variant at threads=1, show how O0→O3 changes throughput.
    """
    threads1 = df[df['threads'] == 1]

    for platform in sorted(df['target'].unique()):
        pdata = threads1[threads1['target'] == platform]

        fig, ax = plt.subplots(figsize=(12, 6))
        variants_sorted = sorted(pdata['variant'].unique())
        opts = sorted(pdata['opt'].unique())

        x = np.arange(len(variants_sorted))
        n_opts = len(opts)
        width = 0.8 / n_opts

        for oi, opt in enumerate(opts):
            values = []
            for v in variants_sorted:
                vrow = pdata[(pdata['variant'] == v) & (pdata['opt'] == opt)]
                values.append(vrow['throughput_MBps'].mean() if len(vrow) > 0 else 0)
            ax.bar(
                x + oi * width - 0.4 + width / 2,
                values, width,
                label=opt, alpha=0.85, edgecolor='black', linewidth=0.4,
            )

        ax.set_xlabel('Method Variant', fontweight='bold')
        ax.set_ylabel('Throughput (MB/s)', fontweight='bold')
        ax.set_title(
            f'Compiler Optimization Impact (Threads=1) – '
            f'{PLATFORM_NAMES.get(platform, platform)}',
            fontweight='bold',
        )
        ax.set_xticks(x)
        ax.set_xticklabels([VARIANT_NAMES.get(v, v) for v in variants_sorted],
                           rotation=25, ha='right')
        ax.legend(title='Opt Level', frameon=True, fancybox=True, shadow=True)
        ax.grid(axis='y', alpha=0.3, linestyle='--')

        _save_fig(f'bar_opt_impact_{platform}_t1.png')


def plot_throughput_vs_threads_overview(df: pd.DataFrame):
    """
    Fig 6: Comprehensive throughput vs threads line plot — all variants, both platforms,
    both optimization levels in a single figure with subplots.
    """
    opts = sorted(df['opt'].unique())
    colors_list = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728',
                   '#9467bd', '#8c564b', '#e377c2', '#17becf']

    fig, axes = plt.subplots(2, 2, figsize=(18, 13))
    axes = axes.flatten()

    for oi, opt in enumerate(opts[:4]):
        ax = axes[oi]
        opt_data = df[df['opt'] == opt]

        for vi, variant in enumerate(sorted(opt_data['variant'].unique())):
            for pi, platform in enumerate(sorted(opt_data['target'].unique())):
                vpdata = opt_data[(opt_data['variant'] == variant) &
                                  (opt_data['target'] == platform)]
                if vpdata.empty:
                    continue
                summary = vpdata.groupby('threads')['throughput_MBps'].mean().sort_index()
                style = '-' if pi == 0 else '--'
                ax.plot(
                    summary.index, summary.values,
                    marker='o' if pi == 0 else 's',
                    linestyle=style,
                    linewidth=1.8,
                    markersize=5,
                    color=colors_list[vi % len(colors_list)],
                    alpha=0.9,
                    label=f'{VARIANT_NAMES.get(variant, variant)} [{PLATFORM_NAMES.get(platform, platform)}]',
                )

        ax.set_xlabel('Number of Threads', fontweight='bold')
        ax.set_ylabel('Throughput (MB/s)', fontweight='bold')
        ax.set_title(f'({opt})', fontweight='bold', fontsize=11)
        ax.set_xticks(sorted(opt_data['threads'].unique()))
        ax.legend(fontsize=6, loc='upper left', framealpha=0.8)
        ax.grid(alpha=0.3, linestyle='--')

    fig.suptitle('SM4-CBC Throughput vs. Threads — Full Comparison',
                 fontweight='bold', fontsize=15, y=1.01)
    plt.tight_layout()
    _save_fig('line_throughput_vs_threads_full.png')


# ----------------------
# 8. LaTeX Table Generation
# ----------------------
def generate_latex_table(df: pd.DataFrame):
    """Generate a comprehensive LaTeX table of all results."""
    table_path = os.path.join(OUTPUT_DIR, '..', 'content', 'result.tex')

    # Aggregate by key dimensions
    pivot = df.groupby(['target', 'opt', 'variant', 'threads'])['throughput_MBps'].mean()

    lines = []
    lines.append(r'\section{实验数据汇总表}')
    lines.append(r'% Auto-generated LaTeX table from experimental data')
    lines.append(r'\begin{longtable}{llllr}')
    lines.append(r'\caption{SM4-CBC 加密性能完整数据汇总（吞吐量 MB/s）}\\')
    lines.append(r'\hline')
    lines.append(r'\textbf{Platform} & \textbf{Opt} & \textbf{Variant} & \textbf{Threads} & \textbf{Throughput (MB/s)} \\')
    lines.append(r'\hline')
    lines.append(r'\endfirsthead')
    lines.append(r'\hline')
    lines.append(r'\textbf{Platform} & \textbf{Opt} & \textbf{Variant} & \textbf{Threads} & \textbf{Throughput (MB/s)} \\')
    lines.append(r'\hline')
    lines.append(r'\endhead')

    for (target, opt, variant, threads), val in pivot.items():
        if pd.isna(val):
            continue
        platform_label = PLATFORM_NAMES.get(target, target)
        variant_label = VARIANT_NAMES.get(variant, variant)
        lines.append(
            f'{platform_label} & {opt} & {variant_label} & {threads} & {val:.2f} \\\\'
        )

    lines.append(r'\hline')
    lines.append(r'\end{longtable}')
    lines.append('')

    # Ensure content directory exists
    os.makedirs(os.path.dirname(table_path), exist_ok=True)
    with open(table_path, 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines))

    print(f"\nLaTeX table saved to: {table_path}")


# ----------------------
# 9. Main Execution
# ----------------------
if __name__ == "__main__":
    print("=" * 65)
    print("  SM4-CBC Performance Analysis & Visualization Tool")
    print("=" * 65)

    # Step 1: Load and preprocess
    merged_data = load_and_preprocess_data(DATA_FILES)

    # Step 2: Statistical summary
    generate_statistics(merged_data)

    # Step 3: Generate all comparison charts
    print(f"\n--- Generating Charts (saved to {OUTPUT_DIR}) ---")

    print("[1/6] Cross-platform comparison by optimization level...")
    plot_platform_comparison(merged_data)

    print("[2/6] Cross-method comparison per platform & optimization level...")
    plot_method_comparison_per_platform(merged_data)

    print("[3/6] Encryption vs Decryption line plots...")
    plot_encrypt_vs_decrypt(merged_data)

    print("[4/6] Speedup ratio analysis...")
    plot_speedup_ratio(merged_data)

    print("[5/6] Optimization impact bar charts (threads=1)...")
    plot_optimization_impact(merged_data)

    print("[6/6] Comprehensive throughput vs threads overview...")
    plot_throughput_vs_threads_overview(merged_data)

    # Step 4: Generate LaTeX table
    print("\n--- Generating LaTeX Table ---")
    generate_latex_table(merged_data)

    print("\n" + "=" * 65)
    print("  Analysis & Visualization Complete!")
    print(f"  All outputs in: {OUTPUT_DIR}")
    print("=" * 65)