KDIR	= 	/lib/modules/$(shell uname -r)/build
PWD		:=	$(shell pwd)

all		: modules

modules :
	make -C $(KDIR) M=$(PWD)/demo1-coretemp modules

clean	:
	rm */*.o
	rm */*.mod
	rm */*.ko
	rm */*.mod.c
	rm */*.order
	rm */*.symvers

.PHONY 	: clean
