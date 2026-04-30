import csv
from pathlib import Path
from cache_simulator_pro import CacheSimulator, iter_trace_file

def run_replacement_analysis():
    trace_path = Path("traces/trace_n128.log")
    if not trace_path.exists():
        print(f"Error: {trace_path} not found.")
        return

    # 固定参数
    BLOCK_SIZE = 64
    ASSOC = 4
    
    # 变量
    capacities_kb = [16, 32, 64,128]
    policies = ['lru', 'fifo']

    results = []

    print("Starting Replacement Policy Analysis (N=128, 4-Way)...")
    
    print("Reading trace into memory...")
    all_accesses = list(iter_trace_file(trace_path))

    for cap_kb in capacities_kb:
        print(f"\nTesting Capacity: {cap_kb}KB")
        for poly in policies:
            sim = CacheSimulator(
                cache_size=cap_kb * 1024,
                block_size=BLOCK_SIZE,
                associativity=ASSOC,
                policy=poly
            )
            
            sim.run(all_accesses)
            stats = sim.stats
            
            results.append({
                'Capacity_KB': cap_kb,
                'Policy': poly.upper(),
                'Hit_Rate': stats.hit_rate * 100,
                'Misses': stats.misses
            })
            print(f"  Policy: {poly.upper():<4} | Hit Rate: {stats.hit_rate*100:.2f}%")

    # 保存结果
    output_file = Path('replacement_study.csv')
    with output_file.open('w', newline='') as f:
        fieldnames = ['Capacity_KB', 'Policy', 'Hit_Rate', 'Misses']
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    
    print(f"\nAnalysis complete. Data saved to {output_file}")

if __name__ == "__main__":
    run_replacement_analysis()
