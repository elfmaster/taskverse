obj-m += kernobjs.o

all:
	make -C linux-headers CONFIG_DEBUG_INFO=y M=$(PWD)
	#make -C /lib/modules/$(shell uname -r)/build CONFIG_DEBUG_INFO=y M=$(PWD) modules

clean:
	rm -f *.mod.c *.o *.ko Module.markers Module.symvers  modules.order

