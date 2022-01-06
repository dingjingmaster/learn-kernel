src = $(wildcard *.asm)
obj = $(patsubst %.asm, %.o, $(src))
run = $(patsubst %.o, %, $(obj))

src1 = $(wildcard common-*.asm)
obj1 = $(patsubst %.asm, %.o, $(src1))
run1 = $(patsubst common-%.asm, common-%, $(src1))

all : $(run) $(run1)


common-% : common-%.o common.o
	ld -o $@ $^ -dynamic-linker /lib64/ld-linux-x86-64.so.2 -lc

% : %.o
	ld -o $@ $< 

%.o : %.asm
	as -g -o $@ $<

%.o1 : %.s
	as -g -o $@ $<

common.o : common.c
	gcc -g -o $@ -c $<

.PHONY : clean all

clean :
	@rm -f *.o
	rm -f $(run) $(run1)
