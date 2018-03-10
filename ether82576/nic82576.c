#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/dma-mapping.h>
#include "nic82576.h"

#if 0
static const struct pci_device_id pci_id_table[] = {
	{ PCI_VDEVICE(INTEL, 0x10c9) },
	{0,}
};
#endif
static const struct pci_device_id pci_id_table[] = {
	{ PCI_DEVICE(0x8086, 0x10c9) },
	{0,}
};
MODULE_DEVICE_TABLE(pci, pci_id_table);

static int nic_open(struct net_device *ndev)
{
	struct nic_adapter *adapter = netdev_priv(ndev);

	if (!adapter){
		pr_err("Opening nic socket failed\n");
	}
	pr_info("Opening nic socket\n");
	pr_info("adapter->io_addr = %p adapter->io_addr = 0x%08x\n", adapter->io_addr, *(adapter->io_addr));

	return 0;
}

static int nic_close(struct net_device *ndev)
{
	pr_info("closing nic socket\n");
	return 0;
}

static netdev_tx_t nic_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	pr_info("Doing pkt transmit\n");
	return 0;
}

static const struct net_device_ops nic_ops = {
	.ndo_open = nic_open,
	.ndo_stop = nic_close,
	.ndo_start_xmit = nic_xmit,
};

/*get the mac address from RAH(0) and RAL(0) registers*/
static int get_mac_address(struct net_device *netdev, u8 *base_addr)
{
	u32 upper = 0, lower = 0;
	u8 mac_address[6] = {0};
	int i = 0;
	
	lower = ioread32(base_addr + 0x05400);
	upper = ioread32(base_addr + 0x05404);

	for (i = 0; i < 4; i++)
		mac_address[i] = (u8)(lower >> (8 * i));

	for (i = 0; i < 2; i++)
		mac_address[i+4] = (u8)(upper >> (8 * i));

	pr_info("mac address %02x:%02x:%02x:%02x:%02x:%02x\n",  
			mac_address[0], mac_address[1], mac_address[2], 
			 mac_address[3], mac_address[4], mac_address[5]);

	/* netdev->dev_addr is used by ethernet interface address info used in eth_type_trans() */	
	memcpy(netdev->dev_addr, mac_address, netdev->addr_len);

	return 0;
}

static int nic_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
	int err = 0;
	struct net_device *netdev = NULL;
	struct nic_adapter *adapter = NULL;
	u8 dma64_capable = 0;

	err = pci_enable_device_mem(pdev);
	if (err < 0){
		goto err_pci_enable;
	}

	pr_info("Enabled the pci device\n");

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (!err){
		pr_info("Device capable of 64 bit dma\n");
		dma64_capable = 1;
	}else{
		err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
		if (!err){
			pr_info("Device capable of 32 bit dma\n");
		}else{
			pr_err("Not dma capable, aborting driver initialization\n");
			goto err_dma;
		}
	}

	pr_info("'%s' DMA capable\n", dma64_capable ? "64 bit": "32 bit");

	//check cat /proc/iomem | grep nicdrv -> name field of this call
	err = pci_request_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM), "nicdrv");
	if (err){
		pr_err("Failed to get the pci memory region\n");
		goto err_req_region;
	}

	pci_set_master(pdev);

	netdev = alloc_etherdev_mq(sizeof(struct nic_adapter), NIC_MAX_TXQS);
	if (!netdev){
		pr_err("Allocating netdev structure failed\n");
		err = -ENOMEM;
		goto err_alloc;
	}

	pci_set_drvdata(pdev, netdev);
	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pcidev = pdev;
	//this is needed. without this crashes in registe_netdev
	netdev->netdev_ops = &nic_ops;

	/* Transmit DMA from high memory
	 *
	 * On platforms where this is relevant, NETIF_F_HIGHDMA signals that
	 * ndo_start_xmit can handle skbs with frags in high memory.
	 */
	if(dma64_capable)
		netdev->features |= NETIF_F_HIGHDMA;

	adapter->io_addr = pci_iomap(pdev, 0 , 0);

	pr_info("adapter->io_addr = %p adapter->io_addr = 0x%08x\n", adapter->io_addr, ioread32(adapter->io_addr));

	get_mac_address(netdev, adapter->io_addr);

	strcpy(netdev->name, "eth%d");
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	return 0;


err_register:
err_alloc:
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
err_req_region:
err_dma:
	pci_disable_device(pdev);
err_pci_enable:
	return err;
}

static void nic_remove(struct pci_dev *pdev)
{
	struct net_device *ndev = pci_get_drvdata(pdev);
	//struct nic_adapter *adapter = netdev_priv(ndev);

	pr_info("Calling nic_remove\n");
	unregister_netdev(ndev);
	pci_release_selected_regions(pdev, pci_select_bars(pdev, IORESOURCE_MEM));
	pci_disable_device(pdev);

	return;
}

static struct pci_driver pci_nic = {
	//if name field is not present, module crashes pci_register_driver
	.name = "nicdrv",
	.id_table = pci_id_table,
	.probe = nic_probe,
	.remove= nic_remove,
};

__init static int nic_init(void)
{
	int ret = 0;

	ret = pci_register_driver(&pci_nic);
	if (ret < 0)
		pr_err("Error registering nic device to pci subsystem\n");
	return 0;
}

__exit static void nic_exit(void)
{
	pci_unregister_driver(&pci_nic);
	return;
}

module_init(nic_init);
module_exit(nic_exit);

MODULE_AUTHOR("Anil Jacob");
MODULE_LICENSE("GPL");
