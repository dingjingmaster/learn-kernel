src = $(wildcard *.asm)
obj = $(patsubst %.asm, %.o, $(src))
run = $(patsubst %.o, %, $(obj))

all : $(run)

# -lc 表示使用 C 库, 纯的汇编不需要链接它
% : %.o
	ld -o $@ $<

%.o : %.asm
	as -g -o $@ $<

#%.o : %.s
#	as -o $@ $<

#common.s : common.c
#	gcc -o $@ -S $<

.PHONY : clean all

clean :
	@rm -f *.o common.s
	rm -f $(run) $(obj)
