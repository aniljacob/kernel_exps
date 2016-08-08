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

/*enables the msi/msix based interrupts for the i350*/
int igb_set_intr_capability(struct net_device *netdev)
{
	int err = 0, i = 0;
	struct i350_dev *adapter = NULL;
	int num_vectors = MAX_MSIX_ENTRIES;

	adapter = netdev_priv(netdev);
	if (!adapter || !adapter->pdev){
		printk(KERN_ERR"No adapter registered in the netdev\n");
		return -1;
	}

	memset(adapter->msix_entries, 0, sizeof(adapter->msix_entries));
	for (i = 0; i < num_vectors; i++)
		adapter->msix_entries[i].entry = i;

	err = pci_enable_msix(adapter->pdev, adapter->msix_entries, 
			num_vectors);
	if (err == 0){
		adapter->flag |= FLG_MSIX_ENABLED;
		for (i = 0; i < adapter->num_vectors; i++)
			printk(KERN_INFO"irq vector = %d\n", adapter->msix_entries[i].vector);
		return 0;
	}

#if 0
	err = pci_enable_msi(adapter->pdev);
	if (err > 0){
		adapter->flag |= FLG_MSI_ENABLED;
		return 2;
	}
#endif

	return 0;
}

int igb_reset_intr_capability(struct net_device *netdev)
{
	struct i350_dev *adapter = netdev_priv(netdev);

	if (!adapter || !adapter->pdev){
		printk(KERN_ERR"Something is wrong, shouldnt see this. No igb interrupt reset!!!\n");
	}

	if (adapter->flag & FLG_MSIX_ENABLED){
		printk(KERN_INFO"disabling pci msix\n");
		pci_disable_msix(adapter->pdev);
	}
#if 0
	else if (adapter->flag & FLG_MSI_ENABLED)
		pci_disable_msi(adapter->pdev);
#endif

	return 0;
}

#if 0
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
	err = release_irq(pdev->irq, igb_intr, IRQF_SHARED,
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
	int err = 0, pci_using_dac = 0;
	struct net_device *netdev = NULL;
	struct i350_dev *adapter = NULL;
	struct e1000_hw *hw =NULL;

    printk(KERN_INFO"registering device=%x, vendor=%x pci device\n", ent->device, ent->vendor);

	/* wake up the device*/
	err = pci_enable_device_mem(pdev);
	if (err)
		return err;

	pci_using_dac = 0;
	/*check if the device is capable for 64 bit dma transactions
	 * dma_set_mask_and_coherent = dma_set_mask - for streaming dma +
	 * dma_set_coherent_mask - for coherent dma*/
	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (!err) {
		pci_using_dac = 1;
	} else {
		/*check if the device is capable for 32 bit dma transactions*/
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (err) {
			dev_err(&pdev->dev,
					"No usable DMA configuration, aborting\n");
			goto errout_dma;
		}
	}

	/* pci request the iomem region from the BAR0*/
	err = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), 
					igb_name);
	if (err)
		goto errout_pcireg;

	/*enables pci configuration space command registers Bit2-Bus Master
	 * this enables the DMA from the device*/
	pci_set_master(pdev);

	/*save the pcie configuration space of the device before suspending*/
	pci_save_state(pdev);

	/*creates a complex heirarchy of netdev, link it to pci device and use our own i350_dev
	 * structure to store pcidevice, and netdev and use it from rest of the netdev ops
	 * this function allocates the net_device object with our i350_dev as private member
	 * */
	netdev = alloc_etherdev_mq(sizeof(struct i350_dev), IGB_MAX_TX_QUEUES);
	if (!netdev){
		printk(KERN_ERR"Allocating netdev failed\n");
		err = -1;
		goto errout_alloc_etherdev;
	}

	/*set the netdevs pci device field with the pdev from probe
	 * Probe invoked automatically upon driver registration and assosciated
	 * pci device will come along with it*/
	SET_NETDEV_DEV(netdev, &pdev->dev);

	/*set pci structures device data as netdev*/
	pci_set_drvdata(pdev, netdev);
	/*fill our i350_dev with information of pci device and net_devices.
	 * we can retrieve it later using container_of macro */
	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pdev = pdev;
	hw = &adapter->hw;
	netdev->netdev_ops = &igb_netdev_ops;

	/* param2 bar no = 0
	 * param3 length = 0 => entire bar memory needs to map
	 * returns _iomem pointer, can use function ioread/iowrite functions to access the 
	 * device registers*/
	hw->hw_addr = pci_iomap(pdev, 0, 0);
	if (!hw->hw_addr){
		printk(KERN_ERR"Mapping PCI memory region failed\n");
		goto errout_iomap;
	}

	igb_set_intr_capability(netdev);
	if (err < 0){
		goto errout_intr;
	}

	/*Now eth interfaces will be available for udev to play with
	 * it will be renamed by udev if the rule is set to rename it*/
	strcpy(netdev->name, "eth%d");
	err = register_netdev(netdev);
	if (err){
		printk(KERN_ERR"registering netdevice failed\n");
		goto errout_register;
	}

	return 0;

errout_register:
errout_intr:
	pci_iounmap(pdev, hw->hw_addr);
errout_iomap:
	free_netdev(netdev);
errout_alloc_etherdev:
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
errout_pcireg:
errout_dma:
	pci_disable_device(pdev);

	return err;
}

static void igb_remove(struct pci_dev *pdev)
{
	struct net_device *netdev = pci_get_drvdata(pdev);
	struct i350_dev *adapter = netdev_priv(netdev);
	struct e1000_hw *hw = &adapter->hw;

    printk(KERN_INFO"removing pci device\n");
	/*when free was called vmcore-dmesg04.txt happened. It was a BUG_ON inside the 
	 * function since I am not satisfying the BUG_ON(dev->reg_state != NETREG_UNREGISTERED);
	 * condition*/
	unregister_netdev(netdev);
	igb_reset_intr_capability(netdev);
	/*need to unmap the memory we mapped using pci_iomap call*/
	pci_iounmap(pdev, hw->hw_addr);
	/*refer vmcore-dmesg02.txt. crash happened when this freeing of netdev was not done.
	 * or i assume so*/
	free_netdev(netdev);
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);
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
