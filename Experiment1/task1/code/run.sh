# 在qemu中运行使用的脚本
#!/bin/sh
# run.sh - 增强版 RISC-V 矩阵乘法自动化测试脚本

RESULT_FILE="/tmp/results.csv"
LOG_FILE="/tmp/last_run.txt"
EXE_PATH="/tmp/matmul"

# 1. 初始化文件与环境信息
echo "=== 矩阵乘法实验自动化测试工具 ==="
echo "开始时间: $(date)"
echo "逻辑核心数: $(nproc)"
echo "-----------------------------------------------"

# 写入 CSV 表头
echo "Scale,Threads,Full_Version_Description,Time_Seconds" > $RESULT_FILE

# 定义测试参数
SCALES="128 256 512"
THREADS="1 2 4 8"

# 确保二进制文件可执行
chmod +x $EXE_PATH

# 2. 嵌套循环开始测试
for s in $SCALES; do
    echo ">>> 正在处理规模: ${s}x${s}x${s} ..."
    for t in $THREADS; do
        printf "    线程数 %2d: " "$t"
        
        # 运行测试并捕获输出
        $EXE_PATH $s $s $s $t > $LOG_FILE
        
        # 3. 逐行解析输出
        # 我们寻找包含 "版本" 的行，并处理格式
        grep "版本" $LOG_FILE | while read -r line; do
            # 提取描述部分（冒号左边）：例如 [版本 1] 循环重排 (i-k-j)
            DESC=$(echo "$line" | cut -d':' -f1 | sed 's/^[ \t]*//')
            
            # 提取时间部分（冒号右边，去掉“秒”）：例如 0.1234
            TIME=$(echo "$line" | cut -d':' -f2 | sed 's/秒//g' | sed 's/^[ \t]*//')
            
            # 写入 CSV
            echo "${s}x${s},$t,\"$DESC\",$TIME" >> $RESULT_FILE
        done
        echo "完成"
    done
done

# 4. 最终汇总打印
echo "-----------------------------------------------"
echo "测试全部结束！"
echo "数据汇总如下 (单位: 秒/Seconds):"
echo ""
# 使用 column 命令让输出对齐（如果 BusyBox 支持的话，不支持则直接 cat）
cat $RESULT_FILE
echo ""
echo "提示: 你可以通过挂载 rootfs 提取 $RESULT_FILE 进行绘图分析。"

# 强制刷盘，防止 QEMU 直接退出导致数据没写进去
sync