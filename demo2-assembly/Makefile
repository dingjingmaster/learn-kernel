src = $(wildcard *.asm)
obj = $(patsubst %.asm, %.o, $(src))
run = $(patsubst %.o, %, $(obj))

all : $(run)

test-print-arr : test-print-arr.o common.o
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
	rm -f $(run) $(obj) 
