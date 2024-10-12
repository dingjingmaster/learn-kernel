# learn kernel
> 主要记录自己linux内核学习过程中的一些例子，每个文件夹一个课题

## 内核调试
1. 编译rootfs make -C ./000-rootfs
2. 编译内核生成 bzImage 并且复制到当前目录
3. 运行：./run-debug.sh
4. 开启另一个终端
```shell
gdb ./bzImage

# 进入后输入
# target remote localhost:1234
# 输入 continue 开始执行内核调试
```
