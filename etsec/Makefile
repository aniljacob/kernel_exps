obj-m:=etsec.o
KDIR=/home/anil/f125_exps/kernel-src/

etsec.o:etsec.c
	make -C $(KDIR) SUBDIRS=$(PWD) modules
clean:
	make -C $(KDIR) SUBDIRS=$(PWD) clean
