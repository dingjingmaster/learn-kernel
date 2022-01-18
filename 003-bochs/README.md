## Bochs常用调试命令

|功能|命令|
|----|----|
|设置断点|b <物理地址>|
|查询目前已经设置过的断点|info break|
|执行程序|c|
|单步执行|s|
|执行n条命令|step n|
|查看寄存器信息|r|
|查看段寄存器信息|sreg|
|打印当前栈里的内容|print-stack|
|帮助|help|

## Bochs 运行过程

1. 安装 bochs
```shell
<xxx> install bochs
<xxx> install bochs-x
```
2. 写一个 bootloader 引导加载程序
3. 创建镜像文件
```shell
bximage
```
4. 将 bootloader 写入镜像
```shell
dd if=boot.bin<生成的二进制> of=xxx.img bs=512 conv=notrunc 
```
5. 配置 bochs
6. 开机
```shell
bochs  -f  bochsrc.conf 
c # 最后可能需要运行命令 C 才能继续执行
```

## Bochs目录&命令

- `/etc/bochs-init` 目录 `bochsrc` 是 bochs 的配置文件
- `/usr/share/bochs` 目录里几个文件是 bochs 在加载内核 image 时 bios 引导用的模拟程序
- `bximage` 命令用来产生一块 floppy disk 或 hard disk 的虚拟盘

## 教程实际操作

1. 下载初代linux内核 `http://www.oldlinux.org/Linux.old/bochs-images/`
2. 使用 `bochs` 创建 image 软盘镜像，运行 `bximage`
3. 把linux内核写入到镜像中 `dd if=boot.bin<生成的二进制> of=xxx.img bs=512 conv=notrunc`
4. 配置 bochs
5. 运行 `bochs -f xxx<配置文件>`
