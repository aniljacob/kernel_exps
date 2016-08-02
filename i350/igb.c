#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include "i350.h"

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Tuxonnst");

char igb_name[]="igb";
struct pci_device_id igb_pci_tbl[] = {
	{ PCI_VDEVICE(INTEL, E1000_DEV_ID_I350_COPPER)},
	{ PCI_VDEVICE(INTEL, E1000_DEV_ID_I350_FIBER) },
	{ PCI_VDEVICE(INTEL, E1000_DEV_ID_I350_SERDES) },
	{ PCI_VDEVICE(INTEL, E1000_DEV_ID_I350_SGMII) }
};

#if 0
/*enables the msi/msix based interrupts for the i350*/
void igb_set_intr_capability(struct net_device *netdev)
{
	int err = 0;
	struct msix_entry msix_entries[MAX_MSIX_ENTRIES] = {0,0};  

	err = pci_enable_msix_range(netdev, msix_entries){
		return;
	}

	err = pci_enable_msi_range(maxvec, maxvec){
		return;
	}
}

int igb_request_irq(int devno)
{
	err = request_irq(pdev->irq, igb_intr, IRQF_SHARED,
		netdev->name, adapter);
	if (err){
		printk(KERN_ERR"Failed to initialize interrupt");
	}

	return 0;
}

int igb_release_irq(int devno)
{
	err = request_irq(pdev->irq, igb_intr, IRQF_SHARED,
		netdev->name, adapter);
	if (err){
		printk(KERN_ERR"Failed to initialize interrupt");
	}

	return 0;
}
#endif

static int igb_open(struct net_device *netdev)
{
	printk(KERN_INFO"igb open call\n");
	return 0;
}

static int igb_close(struct net_device *netdev)
{
	printk(KERN_INFO"igb close call\n");
	return 0;
}

static int igb_ioctl(struct net_device *netdev, struct ifreq *ifr, int cmd)
{
	printk(KERN_INFO"igb ioctl call\n");
	return 0;
}

/* without this function in netdev ops, kernel will Oops. Refer the kernel panic
 * log vmcore-dmesg01.txt for details!!!*/
static netdev_tx_t igb_xmit_frame(struct sk_buff *skb, struct net_device *netdev)
{
	printk(KERN_INFO"igb xmit body call\n");
	return 0;
}

/* without this function in netdev ops, kernel will Oops. Refer the kernel panic
 * log vmcore-dmesg02.txt for details!!!*/
static struct rtnl_link_stats64 *igb_get_stats64(struct net_device *netdev,
			                        struct rtnl_link_stats64 *stats)
{
	return NULL;
}

static const struct net_device_ops igb_netdev_ops = {
	.ndo_open = igb_open,
	.ndo_stop = igb_close,
	.ndo_start_xmit = igb_xmit_frame,
	.ndo_get_stats64 = igb_get_stats64,
	.ndo_do_ioctl = igb_ioctl,
};

static int igb_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err = 0;
	struct net_device *netdev = NULL;
	struct i350_dev *adapter = NULL;
    printk(KERN_INFO"registering device=%x, vendor=%x pci device\n", ent->device, ent->vendor);
	
	netdev = alloc_etherdev_mq(sizeof(struct i350_dev), IGB_MAX_TX_QUEUES);
	if (!netdev){
		printk(KERN_ERR"Allocating netdev failed\n");
		err = -1;
		goto errout_alloc_etherdev;
	}

	SET_NETDEV_DEV(netdev, &pdev->dev);

	pci_set_drvdata(pdev, netdev);
	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	netdev->netdev_ops = &igb_netdev_ops;

	strcpy(netdev->name, "eth%d");
	err = register_netdev(netdev);
	if (err){
		printk(KERN_ERR"registering netdevice failed\n");
		goto errout_register;
	}

	return 0;
errout_register:
	free_netdev(netdev);
errout_alloc_etherdev:
	return err;
}

static void igb_remove(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
    printk(KERN_INFO"removing pci device\n");
	unregister_netdev(netdev);
	free_netdev(netdev);
	return;
}

static struct pci_driver igb_drv = {
	.name = igb_name,
	.id_table=igb_pci_tbl,
	.probe = igb_probe,
	.remove = igb_remove
};

static int igb_init(void)
{
	int ret = 0;
    printk(KERN_INFO"igb v0.1\n");
	ret = pci_register_driver(&igb_drv);
	if (ret < 0)
		printk(KERN_ERR"Registering igb device failed\n");
    return ret;
}

static void igb_exit(void)
{
    printk(KERN_INFO"exiting\n");
	pci_unregister_driver(&igb_drv);
    return;
}

module_init(igb_init);
module_exit(igb_exit);
