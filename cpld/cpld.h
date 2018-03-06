#ifndef __T2081_CPLD_H_
#define __T2081_CPLD_H_

#if 0
struct platform_device {
	const char	*name;
	u32		id;
	struct device	dev;
	u32		num_resources;
	struct resource	*resource;
};

struct resource {
	resource_size_t start;
	resource_size_t end;
	const char *name;
	unsigned long flags;
	unsigned long desc;
	struct resource *parent, *sibling, *child;
};

#endif

#define CPLD_NAME "fsl,t2080-cpld"

struct t2080_cpld {
	struct platform_device *pdev;
	u16 * __iomem addr;
	struct resource res;
};


#endif
