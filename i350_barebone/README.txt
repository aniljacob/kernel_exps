For a barebone i350 driver this is sufficient.
Took from the i350 main code just to show different stages of development
This driver does the following
	1. Registers the pci driver for i350
	2. Invokes the probe function
	3. Registers the netdev operations
	4. Registers the netdev to kernel and creates userspace ethx interfaces
	5. ifconfig command works

Before inserting the igb.ko
[root@compute ~]# ifconfig | grep mtu
enp0s25: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp13s0f0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
enp13s0f1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
enp4s0f0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp4s0f1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp7s0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500

After inserting the igb.ko

[root@compute i350]# ifconfig | grep UP
enp0s25: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp11s0f0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp11s0f1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp11s0f2: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp11s0f3: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp13s0f0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
enp13s0f1: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500
enp4s0f0: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp4s0f1: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1500
enp7s0: flags=4099<UP,BROADCAST,MULTICAST>  mtu 1500

Encountered different kernel panics as mentioned by the following files

vmcore-dmesg01.txt:
	tx method was not defined in netdev_ops. It was invoked without a check once the netdev is registered.

vmcore-dmesg02.txt
	get stats not defined in netdev_ops. Not sure this is a valid crash. May be linked to crash 
	vmcore-dmesg04.txt
	
vmcore-dmesg03.txt
vmcore-dmesg04.txt
	uregister_netdev was not called before free_netdev call
