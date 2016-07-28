obj-m += barebone.o 
KERNELDIR=/lib/modules/${shell uname -r}/build

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules


.PHONY:clean
clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.* 
	rm -f *.mod.c
	rm -rf .barebone.*
	rm modules.order
	rm  -rf .tmp_versions
