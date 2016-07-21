obj-m += demopps.o
KERNELDIR=/lib/modules/${shell uname -r}/build

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

app:
	gcc -o test_demopps -g -Wall test_demopps.c

.PHONY:clean
clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.* 
	rm -f *.mod.c
	rm -rf .demopps*
	rm -rf .tmp_versions 
	rm -f modules.order
