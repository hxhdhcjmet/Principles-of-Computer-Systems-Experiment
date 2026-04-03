import subprocess
import csv
import os
encoding = 'utf-8'
# 定义测试规模 (M, K, N)
scales = [
    (256, 256, 256),
    (512, 512, 512),
    (1024, 1024, 1024),
    (2048,2048,2048)
]

# 定义测试线程数
threads = [1, 2, 4, 8,10]

def run_test(m, k, n, t):
    print(f"正在测试规模 {m}x{k}x{n}, 线程 {t}...")
    cmd = f"matmult_v2.exe {m} {k} {n} {t}"
    # 使用 check=True 确保程序运行成功
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout

# 执行测试并解析输出
csv_file = 'benchmark_results.csv'
with open(csv_file, 'w', newline='', encoding='utf-8') as f:
    writer = csv.writer(f)
    writer.writerow(['Scale', 'Threads', 'Version', 'Time_Seconds'])

    for s in scales:
        for t in threads:
            output = run_test(s[0], s[1], s[2], t)
            lines = output.split('\n')
            for line in lines:
                # 稳健性检查：只有包含“版本”和“秒”的行才解析
                if "版本" in line and "秒" in line:
                    try:
                        # 先按冒号切分，再处理右侧的时间部分
                        name_part = line.split(']')[0] + ']' # 提取 [版本 X]
                        desc_part = line.split(']')[1].split(':')[0].strip() # 提取描述
                        time_part = line.split(':')[-1].replace('秒', '').strip()
                        
                        full_version_name = f"{name_part} {desc_part}"
                        writer.writerow([f"{s[0]}x{s[2]}", t, full_version_name, time_part])
                    except Exception as e:
                        print(f"解析行出错: {line}, 错误: {e}")

print(f"测试完成！结果已保存至 {csv_file}")