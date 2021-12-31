KDIR	= 	/lib/modules/$(shell uname -r)/build
PWD		:=	$(shell pwd)

modules :
	make -C $(KDIR) M=$(PWD)/demo1-coretemp modules
