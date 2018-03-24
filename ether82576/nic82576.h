#ifndef __NIC_82576_
#define __NIC_82576_

#define NIC_MAX_TXQS 16
#define IGB_TX_PTHRESH  8
#define IGB_TX_HTHRESH  1
#define TXDCTL_QUEUE_ENABLE   0x02000000

//for setting cmd_type_len field of tx_desc
//DCMD|DTYP|MAC|RSV|DTALEN [31-24-19-17-15-0]
#define ADVTXD_DTYP_DATA 0x00300000 /*data descriptor*/
#define ADVTXD_DCMD_DEXT 0x20000000 /*advanced descriptor*/
#define ADVTXD_DCMD_IFCS 0x02000000 /*insert FCS (Ethernet CRC)*/
#define ADVTXD_DCMD_EOP  0x01000000 /* End of Packet */

#define ETH_MIN_MTU 68      /* Min IPv4 MTU per RFC791  */
#define ETH_MAX_MTU 0xFFFFU     /* 65535, same as IP_MAX_MTU    */


#define DIAG_REG_TDFH  0x03410 /*Transmit Data FIFO Head register*/
#define DIAG_REG_TDFT  0x03418 /*Transmit Data FIFO Tail register*/
#define DIAG_REG_TDFPC 0x03430 /*Transmit Data FIFO Packet Count register*/
#define DIAG_REG_TXBDC 0x035E0 /*Transmit DMA Performance Burst and Descriptor count register*/

union tx_desc{
	struct {
		__le64 addr;
		__le32 cmd_type_len;
		__le32 status;
	}read;

	struct {
		__le64 rsvd1;
		__le32 rsvd2;
		__le32 sta;
	}wb;
};

struct txring{
	struct net_device *ndev;
	struct device *dev;
	void *desc;
	u8 __iomem *tail;
	dma_addr_t dma;
	int tdbl;
	int tdh, tdl, tdt;
};

struct rx_desc_legacy{
};

struct nic_adapter{
	struct net_device *netdev;
	struct pci_dev *pcidev;
	u8 __iomem *io_addr;
	struct txring *tx_ring[NIC_MAX_TXQS];
	struct dentry *dbgfs_dentry;
};

struct nic_dbgfs_entry{
	struct nic_adapter *adapter;
	struct file_operations fops;
};

int register_nic_debugfs(struct nic_adapter *);
void unregister_nic_debufs(struct nic_adapter *);

#endif
