#include <linux/module.h>
#include <linux/init.h>
#include <linux/pci.h>
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
	struct msix_entry msix_entries[MAX_MSIX_ENTRIES] = 

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

static int igb_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
    printk(KERN_INFO"registering device=%x, vendor=%x pci device\n", ent->device, ent->vendor);
	return 0;
}

static void igb_remove(struct pci_dev *pdev)
{
    printk(KERN_INFO"removing pci device\n");
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
