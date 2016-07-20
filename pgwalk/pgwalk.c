#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/device.h>
#include<linux/cdev.h>

#define PGWLK_NAME "pgwalk"
#define PGWLK_CLASS_NAME "pgwalkc"
#define MAX_DEVNO 1

struct pgwalk_dev{
	dev_t pgw_devno;
	struct class *devclass;
	struct device *device;
	struct cdev pgwalk_cdev;
};

struct pgwalk_dev pgwalk_device;

int pgwalk_open(struct inode *inode, struct file *fp)
{
	printk(KERN_INFO"pgwalk_open\n");
	return 0;
}

ssize_t pgwalk_write(struct file *fp, const char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;

	printk(KERN_INFO"pgwalk_write offset=%d, size=%d\n", offset, sz);
	return size;
}

ssize_t pgwalk_read(struct file *fp, char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;
	printk(KERN_INFO"pgwalk_read offset=%d, size=%d\n", *offset, sz);
	return size;
}

int pgwalk_release(struct inode *inode, struct file *fp)
{
	printk(KERN_INFO"pgwalk_release\n");
	return 0;
}

/*assosciated file operations with the device*/
struct file_operations pgwalk_ops = {
	.owner = THIS_MODULE,
	.open = pgwalk_open,
	.read = pgwalk_read,
	.write = pgwalk_write,
	.release= pgwalk_release,
};

static int setup_cdev(struct cdev *pgw_cdev, struct file_operations *fops)
{
	int ret = 0;

	cdev_init(pgw_cdev, fops);
	pgw_cdev->owner = THIS_MODULE;
	cdev_add(pgw_cdev, pgwalk_device.pgw_devno, MAX_DEVNO);
	if (ret < 0)
		printk(KERN_ERR"adding pgwalk device to kernel failed\n");

	return ret;
}

/* entry point of the module. when 'insmod pgwalk.ko' this function will be executed */
static int pgwalk_init(void)
{
	int ret = 0, i = 0;
	/*first we need to get some device number
	 * [root@compute pgwalk]# ls -al /dev/pgwlk0 
	 * crw-------. 1 root root 247, 0 Jul 19 18:36 /dev/pgwlk0
	 * c=>char, 247-major number, 0- minor number. filesystem
	 * uses this number to identify the fileoperations from vfs.
	 * */
	ret = alloc_chrdev_region(&pgwalk_device.pgw_devno, 0, MAX_DEVNO, PGWLK_NAME);
	if (ret < 0){
		printk("Failed to allocate new device\n");
		goto errout_alloc;
	}

	/*our char device need to be registered to the kernel cdev_init, cdev_add will add
	 * it to the kernels char subsystem*/
	ret = setup_cdev(&pgwalk_device.pgwalk_cdev, &pgwalk_ops);
	if (ret < 0){
		goto errout_cdev;
	}

	/*which class our device belongs to
	 * [root@compute pgwalk]# ls -al /sys/class/pgwalkc/
	 * lrwxrwxrwx.  1 root root 0 Jul 20 09:32 pgwlk0 -> ../../devices/virtual/pgwalkc/pgwlk0
	 * */
	pgwalk_device.devclass = class_create(THIS_MODULE, PGWLK_CLASS_NAME);
	if (IS_ERR(pgwalk_device.devclass)){
		printk(KERN_INFO"Creating device class failed\n");
		ret = -2;
		goto errout_cc;
	}
	/*create the device node in /dev folder using udev.
	 *[root@compute pgwalk]# ls -al /dev/pgwlk0 
	 *crw-------. 1 root root 247, 0 Jul 20 09:32 /dev/pgwlk0
	 * */
	pgwalk_device.device = device_create(pgwalk_device.devclass, NULL, 
			pgwalk_device.pgw_devno, NULL, "pgwlk%d", i);
	if (!pgwalk_device.device){
		printk(KERN_INFO"Creating pgwalk device node failed\n");
		ret = -3;
		goto errout_dc;
	}

	return 0;

/* if failed handle it appropriately*/
errout_dc:
	device_destroy(pgwalk_device.devclass, pgwalk_device.pgw_devno);
	class_destroy(pgwalk_device.devclass);
errout_cc:
	cdev_del(&pgwalk_device.pgwalk_cdev);
errout_cdev:
	unregister_chrdev_region(pgwalk_device.pgw_devno, MAX_DEVNO);
errout_alloc:
	return ret;
}

/* when you rmmod the device*/
static void pgwalk_exit(void)
{
	device_destroy(pgwalk_device.devclass, pgwalk_device.pgw_devno);
	class_destroy(pgwalk_device.devclass);
	cdev_del(&pgwalk_device.pgwalk_cdev);
	unregister_chrdev_region(pgwalk_device.pgw_devno, MAX_DEVNO);
	return;
}

module_init(pgwalk_init);
module_exit(pgwalk_exit);

MODULE_AUTHOR("AJ");
MODULE_LICENSE("GPL");
