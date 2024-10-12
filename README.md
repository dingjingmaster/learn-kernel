# learn kernel
> 主要记录自己linux内核学习过程中的一些例子，每个文件夹一个课题

## 内核调试
1. 编译rootfs make -C ./000-rootfs
2. 编译内核生成 bzImage 并且复制到当前目录
    - 注意编译时候要打开内核调试信息相关配置，在`Kernel hacking`中
3. 运行：./run-debug.sh
4. 开启另一个终端找到 vmlinux
5. 配置gdb env，打开 `~/.gdbinit` 添加 `add-auto-load-safe-path /path/to/linux-build`
```shell
gdb /path/to/vmlinux

# 进入后输入
# target remote localhost:1234
# lx-symbols 加载内核符号
# 输入 continue 开始执行内核调试
```
