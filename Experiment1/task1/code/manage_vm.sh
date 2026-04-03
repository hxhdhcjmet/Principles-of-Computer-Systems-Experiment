#!/bin/bash
# manage_vm.sh - 宿主机自动化部署与启动脚本

# 配置路径（请根据你的实际目录修改）
SRC="matmul.c"
EXE="matmul"
TEST_SH="run.sh"
IMG="rootfs.img"
MNT_DIR="/mnt"

echo "==== 1. 交叉编译中 (RISC-V 静态编译) ===="
# 移除所有 x86 特有的 AVX/FMA 指令，确保 RISC-V 能运行
riscv64-unknown-linux-gnu-gcc $SRC -o $EXE -static -O3 -lpthread
if [ $? -ne 0 ]; then
    echo "编译失败，请检查代码！"
    exit 1
fi

echo "==== 2. 挂载镜像并部署文件 ===="
# 增加 -o sync 选项可以同步写入，虽然慢一点但稳
sudo mount -o loop $IMG $MNT_DIR
sudo cp $EXE $MNT_DIR/tmp/
sudo cp $TEST_SH $MNT_DIR/tmp/
sudo chmod +x $MNT_DIR/tmp/$TEST_SH

# 关键：先同步，再卸载，再同步
sync
sudo umount -d $MNT_DIR  # -d 确保循环设备被释放
sync
echo "部署成功并已强制刷新到磁盘"

echo "==== 3. 启动 QEMU RISC-V 虚拟机 ===="
# 注意：这里加上了 -smp 8 以确保多线程测试有效
# 将 append 中的 ro 改为 rw，方便直接在里面写 results.csv
qemu-system-riscv64 -M virt -smp 8 -m 8192 \
    -bios opensbi/build/platform/generic/firmware/fw_jump.elf \
    -kernel linux-6.14.1/arch/riscv/boot/Image \
    -drive file=$IMG,format=raw,id=hd0 \
    -device virtio-blk-device,drive=hd0 \
    -append "rootwait root=/dev/vda rw" \
    -nographic