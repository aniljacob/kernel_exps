#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/of_platform.h>
#include <linux/io.h>

#include "cpld.h"

#if 0
struct file_operations cpld_ops = {
	.owner = THIS_MODULE,
	.open = t2081_open,
	.release = t2081_close,
};
#endif

static const struct of_device_id cpld_match[] = {
	{
		.compatible = "fsl,t2080-cpld",
	},
	{},
};

struct t2080_cpld boardctrl;

static int t2081_cpld_probe(struct platform_device *dev)
{
	int ret = 0;
	const struct of_device_id *match;
	struct device_node *np = dev->dev.of_node;
	struct resource *res = &boardctrl.res;

	pr_info("Calling cpld_probe\n");
	match = of_match_device(cpld_match, &dev->dev);
	if (!match){
		ret = -EINVAL;
		goto err_res;
	}

	//this will give the physical address at res.start and res.end
	//defined in io_port.h. Refer cpld.c for comments
	ret = of_address_to_resource(np, 0, res); 
	if (ret) {
		pr_err("invalid address from OF\n");
		goto err_res;
	}

	if(!request_mem_region(res->start, res->end - res->start, "t2080-cpld")){
		pr_err("Failed to reserve memory '%llx'-'%llx'\n", res->start, res->end);
		goto err_res;
	}

	boardctrl.addr = (u16 *)of_iomap(dev->dev.of_node, 0);
	if (!boardctrl.addr){
		pr_err("Failed to map address for cpld\n");
		ret = -ENODEV;
		goto err_iomap;
	}

	//this also works. however second one using readw is more appropriate...
	//pr_info("Build year and month = 0x%04x build day = 0x%04x\n", *(boardctrl.addr), *(boardctrl.addr+1));
	pr_info("Build year and month = 0x%04x build day = 0x%04x\n", readw(boardctrl.addr), readw(boardctrl.addr+1));

	return 0;

err_iomap:
	release_mem_region(res->start, res->end - res->start);

err_res:

	return ret;
}

static int t2081_cpld_remove(struct platform_device *dev)
{
	struct resource *res = &boardctrl.res;
	pr_info("Calling cpld_remove\n");

	iounmap(boardctrl.addr);
	release_mem_region(res->start, res->end - res->start);

	return 0;
}

struct platform_driver cpld_drv = {
	.driver = {
		.name = CPLD_NAME,
		.owner = THIS_MODULE,
		.of_match_table = cpld_match,
	},
	.probe = t2081_cpld_probe,
	.remove = t2081_cpld_remove,
};

__init static int cpld_init(void)
{
	pr_info("In cpld init\n");

	platform_driver_register(&cpld_drv);

	return 0;
}

__exit static void cpld_exit(void)
{
	pr_info("In cpld exit\n");
	platform_driver_unregister(&cpld_drv);

	return;
}

module_init(cpld_init);
module_exit(cpld_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("");
