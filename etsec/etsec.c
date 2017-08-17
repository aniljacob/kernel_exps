#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>

/*
 * dts entry
enet2: ethernet@26000 {
		   cell-index = <2>;
		   device_type = "network";
		   model = "eTSEC";
		   compatible = "gianfar";
		   reg = <0x26000 0x1000>;
		   local-mac-address = [ 00 00 00 00 00 00 ];
		   interrupts = <31 2 32 2 33 2>;
		   interrupt-parent = <&mpic>;
		   tbi-handle = <&tbi2>;
		   phy-handle = <&phy3>;
		   phy-connection-type = "sgmii"; // was rgmii-id
		   idiot = "adarsh";
};
*/

/*probe - we will initialize everything here
 * 1. NAPI 2. irq 3. ethtool 4. watchdog 5. ioremap of memory map.
 * 
 * */
static int etsec_probe(struct platform_device *ofdev)
{
	printk(KERN_INFO"[Anil] probing etsec device\n");
	return 0;
}

/*
 * remove function when we exit the driver
 * */
static int etsec_remove(struct platform_device *ofdev)
{
	printk(KERN_INFO"[Anil] removing etsec device\n");
	return 0;
}
/*
 * the table to match the compatible field of dtb 
 * Used as a field to identify the device to execute the probe, remove function
 *
 * */

static struct of_device_id etsec_table[] = {
	{
		.type = "network",
		.compatible = "gianfar",
	},
	{},
};

MODULE_DEVICE_TABLE(of, etsec_table);

/*since etsec is an SOC chip, we need to use platform_register_driver
 * when this is invoked, the corresponding probe function will be called
 * as well
 * */
static struct platform_driver etsec_drv = {
	.driver = {
		.name = "etsecAjD",
		.owner = THIS_MODULE,
		.of_match_table = etsec_table,
	},
	.probe = etsec_probe,
	.remove = etsec_remove,
};

/*
 * Etsec module entry point
 * */

static int etsec_init(void)
{
	printk(KERN_INFO"[Anil]Initializing etsec module of P2020\n");

	return platform_driver_register(&etsec_drv);
}

/*
 * Etsec module exit point
 * */

static void etsec_exit(void)
{
	printk(KERN_INFO"[Anil]Exiting etsec module of P2020\n");

	platform_driver_unregister(&etsec_drv);
	return;
}

module_init(etsec_init);
module_exit(etsec_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("AjD");
