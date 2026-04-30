import csv
from pathlib import Path
from cache_simulator_pro import CacheSimulator, iter_trace_file

def run_algo_comparison():
    # 定义两个 Trace 的路径
    trace_naive = Path("traces/trace_n128.log")         # 普通 IJK 算法
    trace_blocked = Path("traces/trace_blocked_n128.log") # 分块算法
    
    if not trace_naive.exists() or not trace_blocked.exists():
        print("Error: Ensure both trace_n128.log and trace_blocked_n128.log exist.")
        return

    # 固定参数：针对矩阵乘法，通常用 4路/8路组相联
    BLOCK_SIZE = 64
    ASSOC = 4
    POLICY = 'lru'
    
    # 遍历 Cache 容量
    capacities_kb = [4, 8, 16, 32, 64, 128]
    
    results = []

    # 预加载两个 Trace
    print("Loading Naive Trace...")
    naive_accesses = list(iter_trace_file(trace_naive))
    print("Loading Blocked Trace...")
    blocked_accesses = list(iter_trace_file(trace_blocked))

    for cap_kb in capacities_kb:
        print(f"Simulating Capacity: {cap_kb}KB...")
        
        # 1. 模拟普通算法
        sim_n = CacheSimulator(cap_kb*1024, BLOCK_SIZE, ASSOC, POLICY)
        sim_n.run(naive_accesses)
        
        # 2. 模拟分块算法
        sim_b = CacheSimulator(cap_kb*1024, BLOCK_SIZE, ASSOC, POLICY)
        sim_b.run(blocked_accesses)
        
        results.append({
            'Capacity_KB': cap_kb,
            'Naive_Hit_Rate': sim_n.stats.hit_rate * 100,
            'Blocked_Hit_Rate': sim_b.stats.hit_rate * 100
        })

    # 保存 CSV
    output_file = Path('algorithm_comparison.csv')
    with output_file.open('w', newline='') as f:
        writer = csv.DictWriter(f, fieldnames=['Capacity_KB', 'Naive_Hit_Rate', 'Blocked_Hit_Rate'])
        writer.writeheader()
        writer.writerows(results)
    
    print(f"Data saved to {output_file}")

if __name__ == "__main__":
    run_algo_comparison()
