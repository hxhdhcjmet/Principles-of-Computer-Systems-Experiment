import matplotlib.pyplot as plt

# 各方案名称
schemes = [
    "default_loop",
    "unrolled",
    "reduced_dep",
    "xbox",
    "xbox_merged",
    "zbb_default",
    "zbb_xbox",
]

# 1 KB 与 16 MB 下的吞吐率（最新数据）
throughput_1kb = [
    21.949,
    24.834,
    22.028,
    28.366,
    36.402,
    24.943,
    35.998,
]

throughput_16mb = [
    21.183,
    23.551,
    21.168,
    26.465,
    32.243,
    23.707,
    31.794,
]

# 计算下降比例
decrease = [
    (small - large) / small * 100
    for small, large in zip(throughput_1kb, throughput_16mb)
]

# 为每个柱子分配不同颜色
colors = [
    "#7f7f7f",  # gray
    "#1f77b4",  # blue
    "#ff7f0e",  # orange
    "#2ca02c",  # green
    "#d62728",  # red
    "#9467bd",  # purple
    "#8c564b",  # brown
]

plt.figure(figsize=(10.5, 5.8))
bars = plt.bar(schemes, decrease, color=colors)

plt.xlabel("Optimization Scheme", fontsize=12)
plt.ylabel("Throughput Decrease from 1 KB to 16 MB (%)", fontsize=12)
plt.title("Throughput Decrease Across Input Data Sizes", fontsize=14)

plt.xticks(rotation=20, ha="right")
plt.grid(axis="y", linestyle="--", alpha=0.35)

# 在柱顶显示数值
for bar, value in zip(bars, decrease):
    plt.text(
        bar.get_x() + bar.get_width() / 2,
        bar.get_height() + 0.12,
        f"{value:.2f}%",
        ha="center",
        va="bottom",
        fontsize=10,
        fontweight="bold",
    )

plt.tight_layout()
plt.savefig("figure16_throughput_decrease.png", dpi=300, bbox_inches="tight")
plt.show()