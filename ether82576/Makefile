obj-m := nic82576.o

KDIR=/lib/modules/`uname -r`/build

default:
	$(MAKE) -C ${KDIR} SUBDIRS=${PWD} modules

clean:
	$(MAKE) -C ${KDIR} SUBDIRS=${PWD} clean
