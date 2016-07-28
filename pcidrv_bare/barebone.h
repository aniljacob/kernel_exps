#ifndef __INTEL_I350__
#define __INTEL_I350__

#define MAX_MSIX_ENTRIES 8
#define E1000_DEV_ID_I350_COPPER        0x1521
#define E1000_DEV_ID_I350_FIBER         0x1522
#define E1000_DEV_ID_I350_SERDES        0x1523
#define E1000_DEV_ID_I350_SGMII         0x1524

struct i350_dev{
	struct msix_entry msix_entries[MAX_MSIX_ENTRIES];
};
#endif
