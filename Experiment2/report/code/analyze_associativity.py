import csv
from pathlib import Path
from cache_simulator_pro import CacheSimulator, iter_trace_file

def run_assoc_analysis():
    trace_path = Path("traces/trace_n128.log")
    if not trace_path.exists():
        print(f"Error: {trace_path} not found.")
        return

    # 固定参数
    CAP_KB = 64# 固定容量为 32KB 以观察冲突改善
    BLOCK_SIZE = 64  # 实验要求：64B
    POLICY = 'lru'   # 实验要求：LRU
    
    # 变量：相联度
    assoc_list = [1, 2, 4, 8, 'full']

    results = []

    print(f"Starting Associativity Analysis (Cap={CAP_KB}KB, Block={BLOCK_SIZE}B)...")
    
    print("Reading trace into memory...")
    all_accesses = list(iter_trace_file(trace_path))

    for assoc in assoc_list:
        # 初始化模拟器
        sim = CacheSimulator(
            cache_size=CAP_KB * 1024,
            block_size=BLOCK_SIZE,
            associativity=assoc,
            policy=POLICY
        )
        
        sim.run(all_accesses)
        stats = sim.stats
        
        # 记录展示名
        display_name = "Full" if assoc == 'full' else f"{assoc}-Way"
        
        results.append({
            'Associativity': display_name,
            'Hit_Rate': stats.hit_rate * 100,
            'Misses': stats.misses,
            'Accesses': stats.accesses
        })
        print(f"  Assoc: {display_name:>6} | Hit Rate: {stats.hit_rate*100:.2f}%")

    # 保存结果
    output_file = Path('associativity_study.csv')
    with output_file.open('w', newline='') as f:
        fieldnames = ['Associativity', 'Hit_Rate', 'Misses', 'Accesses']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    
    print(f"\nAnalysis complete. Data saved to {output_file}")

if __name__ == "__main__":
    run_assoc_analysis()
