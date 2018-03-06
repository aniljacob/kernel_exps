obj-m:=cpld.o

PWD=$(shell pwd)
KDIR=/home/anil/T2081-BSP/t2080_linux
rtc.o:
	make -C $(KDIR) CROSS_COMPILE=powerpc64-fsl-linux- SUBDIRS=$(PWD) modules

clean:
	make -C $(KDIR) CROSS_COMPILE=powerpc64-fsl-linux- SUBDIRS=$(PWD) clean 
