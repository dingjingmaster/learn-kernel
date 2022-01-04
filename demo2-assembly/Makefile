src = $(wildcard *.asm)
obj = $(patsubst %.asm, %.o, $(src))
run = $(patsubst %.o, %, $(obj))

all : $(run)

% : %.o
	ld -o $@ $<

%.o : %.asm
	as -o $@ $< 

.PHONY : clean all

clean :
	@rm -f *.o
	rm -f $(run) $(obj)
