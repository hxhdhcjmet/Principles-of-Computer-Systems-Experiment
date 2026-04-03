#!/bin/bash
# 实验报告文件夹一键初始化脚本
# 在希望创建report的文件夹中执行,创建对应report文件夹,自动创建对应各种文件夹和文件
# 1. 定义根文件夹名称
REPORT_DIR="report"

# 2. 定义所有子文件夹（修正你笔误的 foemat → format）
SUB_DIRS=(
    "$REPORT_DIR/content"
    "$REPORT_DIR/code"
    "$REPORT_DIR/config"
    "$REPORT_DIR/bib"
    "$REPORT_DIR/figure"
    
)

# 3. 递归创建所有文件夹（-p 参数：不存在则创建，已存在不报错）
echo "正在创建实验报告文件夹结构..."
for dir in "${SUB_DIRS[@]}"; do
    mkdir -p "$dir"
done

# 4. 在对应文件夹创建空的 tex 文件
touch "$REPORT_DIR/content/abstract.tex"       # 摘要(其他内容均在此文件夹中)
touch "$REPORT_DIR/code/code.tex"           # 代码
touch "$REPORT_DIR/config/format.tex"       # 格式
touch "$REPORT_DIR/bib/bib.tex"             # 参考文献

# 5. 可选：创建 report 主 tex 文件（直接编译用）
touch "$REPORT_DIR/report.tex"

echo "✅ 实验报告结构初始化完成！文件夹列表："
tree "$REPORT_DIR"  # 如果没装tree，删掉这行也能用