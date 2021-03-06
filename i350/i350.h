#ifndef __INTEL_I350__
#define __INTEL_I350__

#define MAX_MSIX_ENTRIES 8
#define IGB_MAX_TX_QUEUES 8
#define E1000_DEV_ID_I350_COPPER        0x1521
#define E1000_DEV_ID_I350_FIBER         0x1522
#define E1000_DEV_ID_I350_SERDES        0x1523
#define E1000_DEV_ID_I350_SGMII         0x1524

#define FLG_MSIX_ENABLED 1
#define FLG_MSI_ENABLED  2

struct e1000_hw{
	u8 __iomem *hw_addr;

	u16 device_id;
	u16 subsystem_vendor_id;
	u16 subsystem_device_id;
	u16 vendor_id;
	u8  revision_id;
};

struct i350_dev{
	struct net_device *netdev;
	struct pci_dev *pdev;
	int flag;
	
	/*interrupt related structures*/
	int num_vectors;
	int num_txq;
	int num_rxq;

	struct msix_entry msix_entries[MAX_MSIX_ENTRIES];
	/*hardware information*/
	struct e1000_hw hw;
};
#endif
