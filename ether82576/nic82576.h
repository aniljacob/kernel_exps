#ifndef __NIC_82576_
#define __NIC_82576_

#define NIC_MAX_TXQS 16

struct nic_adapter{
	struct net_device *netdev;
	struct pci_dev *pcidev;
	u8 __iomem *io_addr;
};

union tx_desc{
	struct {
		u64 addr;
		u32 cmd_type_len;
		u32 status;
	}read;

	struct {
		u64 rsvd1;
		u32 rsvd2;
		u32 sta;
	}wb;
};

struct rx_desc_legacy{
};

#endif
