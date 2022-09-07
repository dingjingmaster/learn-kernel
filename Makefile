KDIR	= 	/lib/modules/$(shell uname -r)/build
PWD		:=	$(shell pwd)

all		: aufs1 aufs coretemp


coretemp:
	make -C $(KDIR) M=$(PWD)/001-coretemp modules

aufs	:
	make -C $(KDIR) M=$(PWD)/007-aufs modules

aufs1	:
	make -C $(KDIR) M=$(PWD)/006-aufs modules


clean	:
	rm -f */*.o
	rm -f */*.mod
	rm -f */*.ko
	rm -f */*.mod.c
	rm -f */*.order
	rm -f */*.symvers
	rm -f `find -name "*.o.*"`
	rm -f `find -name "*.cmd"`

.PHONY 	: clean
