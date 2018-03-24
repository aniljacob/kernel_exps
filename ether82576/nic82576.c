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

void dump_skb(struct sk_buff *skb)
{
	u16 *p = (u16 *)skb->head;

	pr_info("00: %04x %04x %04x %04x %04x %04x %04x %04x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	p += 8;
	pr_info("10: %04x %04x %04x %04x %04x %04x %04x %04x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	p += 8;
	pr_info("20: %04x %04x %04x %04x %04x %04x %04x %04x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
	p += 8;
	pr_info("30: %04x %04x %04x %04x %04x %04x %04x %04x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);

	return;
}

static void dump_stat_regs(struct nic_adapter *adapter)
{
	int i = 0;
	u32 __iomem *stat_reg_start = (u32 *)(adapter->io_addr + 0x4000);
	u32 val = 0;

	for (i = 0; i < 0xD8; i++){
		val = readl(stat_reg_start + i);
		if (val)
			pr_info("%02x: 0x%08x\n", i, val);
	}

	return;
}

static void dump_diag_reg(struct nic_adapter *adapter)
{
	u8 *__iomem diag_reg_start = adapter->io_addr;

	pr_info("TDFH 0x%08x\n", readl(diag_reg_start + DIAG_REG_TDFH));
	pr_info("TDFT 0x%08x\n", readl(diag_reg_start + DIAG_REG_TDFT));
	pr_info("TDFPC 0x%08x\n", readl(diag_reg_start + DIAG_REG_TDFPC));
	pr_info("TXBDC 0x%08x\n", readl(diag_reg_start + DIAG_REG_TXBDC));

	return;
}

//Use the dma memory from the nic_setup_txq call for setting up the 
//TDBA (base lo/hi) 0xE000/0xE004, TDLEN(length) 0xE008, TDH(head) 0xE010, TDT(tail) 0xE018 registers
static int nic_config_txq_reg(struct nic_adapter *adapter, struct txring *tx_ring)
{
	u8 __iomem *base = adapter->io_addr;
	u32 txdctl = 0;

	tx_ring->tail = base + 0xE018;

	//disable the txq
	writel(0, base + 0xE028);

	writel(tx_ring->dma & 0x00000000ffffffffULL, base + 0xE000);
	writel(tx_ring->dma >> 32, base + 0xE004);
	writel(tx_ring->tdbl, base + 0xE008);
	writel(tx_ring->tdh, base + 0xE010);
	writel(0, tx_ring->tail);

	//enable the txq
	txdctl |= IGB_TX_PTHRESH;
	txdctl |= IGB_TX_HTHRESH << 8;
	txdctl |= 16 << 16;
	txdctl |= TXDCTL_QUEUE_ENABLE;
	writel(txdctl, base + 0xE028);

	return 0;
}

//allocate a dma-able memory a consistant one in RAM for setting up
//the TDBAL (base), TDBAH (base) TDLEN(length), TDH(head), TDT(tail) registers
static int nic_setup_txq(struct net_device *ndev, int index)
{
	int ret = 0;
	struct nic_adapter *adapter = netdev_priv(ndev);
	//This throwed an error with failure in dma allocation as below
	//SIOCSIFFLAGS: Cannot allocate memory
	//struct device *dev = &ndev->dev;
	struct device *dev = &adapter->pcidev->dev;
	struct txring *tx_ring = NULL;

	if (index > NIC_MAX_TXQS){
		pr_err("Index '%d' greater than '%d'\n", index, NIC_MAX_TXQS);
		ret = -EINVAL;
		goto err_alloc;
	}

	tx_ring = kzalloc(sizeof(struct txring), GFP_KERNEL);
	if (adapter->tx_ring[index]){
		pr_err("Couldnt create the tx_ring pointer\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	tx_ring->tdbl = 256 * sizeof(union tx_desc);
	tx_ring->tdbl = ALIGN(tx_ring->tdbl, 4096);
	pr_info("tx_ring->tdbl = %d\n", tx_ring->tdbl);
	tx_ring->desc = dma_alloc_coherent(dev, tx_ring->tdbl, &tx_ring->dma, GFP_KERNEL);
	if(!tx_ring->desc){
		pr_err("Failed to get dma memory for txq\n");
		ret = -ENOMEM;
		goto err_dma_alloc;
	}

	tx_ring->dev = dev;
	tx_ring->ndev = ndev;

	nic_config_txq_reg(adapter, tx_ring);
	adapter->tx_ring[index] = tx_ring;


	return 0;

err_dma_alloc:
	kfree(tx_ring);

err_alloc:
	return ret;
}

static void nic_cleanup_txq(struct net_device *ndev, int index)
{
	struct nic_adapter *adapter = netdev_priv(ndev);
	struct txring *tx_ring = adapter->tx_ring[index];
	u8 __iomem *base = adapter->io_addr;

	//disable the txq
	writel(0,  base + 0xE028);

	if(tx_ring)
		kfree(tx_ring);

	dma_free_coherent(tx_ring->dev, tx_ring->tdbl, tx_ring->desc, tx_ring->dma);


	return;
}

static int nic_open(struct net_device *ndev)
{
	int ret = 0;
	struct nic_adapter *adapter = netdev_priv(ndev);

	if (!adapter){
		pr_err("Opening nic socket failed\n");
	}
	pr_info("Opening nic socket\n");
	pr_info("adapter->io_addr = %p adapter->io_addr = 0x%08x\n", adapter->io_addr, readl(adapter->io_addr));

	ret = nic_setup_txq(ndev, 0);

	return ret;
}

static int nic_close(struct net_device *ndev)
{
	pr_info("closing nic socket\n");
	nic_cleanup_txq(ndev, 0);
	return 0;
}

static netdev_tx_t nic_xmit(struct sk_buff *skb, struct net_device *ndev)
{
	struct nic_adapter *adapter = netdev_priv(ndev);
	dma_addr_t dma;
	int err = 0, i = 0;
	u32 size = 0;
	struct txring *tx_ring = adapter->tx_ring[i];
	union tx_desc *desc = tx_ring->desc;
	//u32 cmd_type = ADVTXD_DTYP_DATA | ADVTXD_DCMD_DEXT | ADVTXD_DCMD_IFCS;
	u32 cmd_type = ADVTXD_DTYP_DATA | ADVTXD_DCMD_DEXT;

	//i am trying to send only one packet, so set this as EOP

	pr_info("Doing pkt transmit\n");
	//dump_skb(skb);
	size = skb_headlen(skb);
	cmd_type |= ADVTXD_DCMD_EOP;
	dma = dma_map_single(tx_ring->dev, skb->data, size, DMA_TO_DEVICE);
	if (dma_mapping_error(tx_ring->dev, dma)){
		pr_err("Unable to do streaming dma mapping\n");
		err = -EIO;
		goto err_stream_dma;
	}
	desc->read.addr = cpu_to_le64(dma);
	desc->read.cmd_type_len = cpu_to_le32(cmd_type ^ size);

	wmb();
	writel(0, tx_ring->tail);
	mmiowb();

	//dump_stat_regs(adapter);
	//dump_stat_regs(adapter);
	dump_diag_reg(adapter);
	//pr_info("0x%08x\n", readl(adapter->io_addr + 0x040D4));
	return 0;

err_stream_dma:

	return err;
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

static int nic_setup_irq(struct nic_adapter *adapter)
{
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
	SET_NETDEV_DEV(netdev, &pdev->dev);
	adapter = netdev_priv(netdev);
	adapter->netdev = netdev;
	adapter->pcidev = pdev;
	//this is needed. without this crashes in registe_netdev
	netdev->netdev_ops = &nic_ops;

	netdev->mem_start = pci_resource_start(pdev, 0);
    netdev->mem_end = pci_resource_end(pdev, 0);

	/* Transmit DMA from high memory
	 *
	 * On platforms where this is relevant, NETIF_F_HIGHDMA signals that
	 * ndo_start_xmit can handle skbs with frags in high memory.
	 */
	if(dma64_capable)
		netdev->features |= NETIF_F_HIGHDMA;

	adapter->io_addr = pci_iomap(pdev, 0 , 0);

	get_mac_address(netdev, adapter->io_addr);

	strcpy(netdev->name, "eth%d");
	err = register_netdev(netdev);
	if (err)
		goto err_register;

	pr_info("command = 0x%08x status = 0x%08x\n", ioread32(adapter->io_addr), ioread32(adapter->io_addr+8));
#if 0
	err = register_nic_debugfs(adapter);
	if (err < 0){
		pr_err("Failed to create debufs entry for %s\n", netdev->name);
	}
#endif

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

	//unregister_nic_debufs(adapter);
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
