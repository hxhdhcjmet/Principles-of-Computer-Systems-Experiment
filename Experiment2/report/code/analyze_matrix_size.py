import csv
from pathlib import Path
from cache_simulator_pro import CacheSimulator, iter_trace_file

def run_matrix_size_analysis():
    # 实验变量
    matrix_sizes = [32, 64, 128]
    capacities_kb = [32, 64]
    
    # 固定参数
    BLOCK_SIZE = 64
    ASSOC = 4
    POLICY = 'lru'

    results = []

    print("Starting Matrix Size vs Cache Capacity Analysis...")

    for n in matrix_sizes:
        trace_path = Path(f"traces/trace_n{n}.log")
        if not trace_path.exists():
            print(f"Warning: {trace_path} not found, skipping N={n}")
            continue
            
        print(f"\nProcessing Matrix Size N={n}...")
        # 为了加速，每个 N 只读一次 trace
        all_accesses = list(iter_trace_file(trace_path))
        
        for cap_kb in capacities_kb:
            sim = CacheSimulator(
                cache_size=cap_kb * 1024,
                block_size=BLOCK_SIZE,
                associativity=ASSOC,
                policy=POLICY
            )
            
            sim.run(all_accesses)
            stats = sim.stats
            
            results.append({
                'Matrix_Size': n,
                'Cache_Capacity_KB': cap_kb,
                'Hit_Rate': stats.hit_rate * 100,
                'Misses': stats.misses
            })
            print(f"  Cache: {cap_kb}KB | Hit Rate: {stats.hit_rate*100:.2f}%")

    # 保存结果
    output_file = Path('matrix_size_study.csv')
    with output_file.open('w', newline='') as f:
        fieldnames = ['Matrix_Size', 'Cache_Capacity_KB', 'Hit_Rate', 'Misses']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    
    print(f"\nAnalysis complete. Data saved to {output_file}")

if __name__ == "__main__":
    run_matrix_size_analysis()
