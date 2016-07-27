obj-m += pgwalk.o
KERNELDIR=/lib/modules/${shell uname -r}/build

default:
	$(MAKE) -C $(KERNELDIR) SUBDIRS=$(PWD) modules

app:
	gcc -o app_pgwalk -g -Wall app_pgwalk.c

.PHONY:clean
clean:
	rm -f *.ko
	rm -f *.o
	rm -f Module.* 
	rm -f *.mod.c
	rm -rf .tmp_versions
	rm -f .pgwalk*
	rm -f modules.order
