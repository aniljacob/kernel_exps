#include<linux/kernel.h>
#include<linux/module.h>
#include<linux/fs.h>
#include<linux/types.h>
#include<linux/device.h>
#include<linux/cdev.h>
#include<linux/poll.h>
#include<linux/sched.h>

#define DEMOPPS_NAME "demopps"
#define DEMOPPS_CLASS_NAME "demopps"
#define MAX_DEVNO 1

struct dps_dev{
	dev_t dps_devno;
	struct class *devclass;
	struct device *device;
	struct cdev dps_cdev;
	struct timer_list pps_timer;
	wait_queue_head_t waitq;
	int flag;
	char name[32];
};

struct dps_dev dps_device;

int dps_open(struct inode *inode, struct file *fp)
{
	/*need to assign the private_data of file for rest of the operations
	 * with the devices desc structure. This way we can share the members
	 * of the per specific devices. For this to work we have to embed the 
	 * cdev structure inside the device desc structure. All the subsequent
	 * fops can use this private_data to do their things*/
	struct dps_dev *dev = container_of(inode->i_cdev, struct dps_dev, dps_cdev);
	fp->private_data = dev;

	printk(KERN_INFO"dps_open\n");
	return 0;
}

ssize_t dps_write(struct file *fp, const char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;

	printk(KERN_INFO"dps_write offset=%d, size=%d\n", offset, sz);
	return size;
}

ssize_t dps_read(struct file *fp, char __user *buf, size_t sz, loff_t *offset)
{
	size_t size = 0;
	struct dps_dev *dpd = fp->private_data;

	printk(KERN_INFO"dps_read offset=%d, size=%d, %s\n", *offset, sz, dpd->name);

	dpd->flag = 0;
	printk(KERN_INFO"blocking read\n");
	wait_event_interruptible(dps_device.waitq, dpd->flag);
	printk(KERN_INFO"unblocked read\n");

	return size;
}

static unsigned int dps_poll(struct file *file, struct poll_table_struct *wait)
{
	struct dps_dev *dpd = file->private_data;

	dpd->flag = 0;
	wait_event_interruptible(dps_device.waitq, dpd->flag);
	//poll_wait(file, &dpd->waitq, wait);
	printk(KERN_INFO"Some poll mechanism\n");

	return (POLLIN | POLLRDNORM );
}

int dps_release(struct inode *inode, struct file *fp)
{
	printk(KERN_INFO"dps_release\n");
	return 0;
}

/*assosciated file operations with the device*/
struct file_operations dps_ops = {
	.owner = THIS_MODULE,
	.open = dps_open,
	.read = dps_read,
	.write = dps_write,
	.poll = dps_poll,
	.release= dps_release,
};

/* a timer function for setting up a new timer*/
static void pps_timer_event(unsigned long argp)
{
	/*enable the printk below. it will print this at 2s intervals*/
	printk(KERN_INFO"experiment with timers\n", jiffies);
	/* reactivate the timer so that it will continue running */
	mod_timer(&dps_device.pps_timer, jiffies + 1 * HZ);
	/*wakes up the poll thread waiting on the waitqueue*/
	dps_device.flag = 1;
	wake_up_interruptible(&dps_device.waitq);
 	//kill_fasync(&dps_device.waitq, SIGIO, POLL_IN);
}

/* creating a new timer in linux kernel the function above is the one which 
 * will be executed upon the expiration of timer*/
static void setup_pps_timer(void)
{
	setup_timer(&dps_device.pps_timer, pps_timer_event, 0);
	mod_timer(&dps_device.pps_timer, jiffies + HZ);
}

static int setup_cdev(struct cdev *dps_cdev, struct file_operations *fops)
{
	int ret = 0;

	cdev_init(dps_cdev, fops);
	dps_cdev->owner = THIS_MODULE;
	cdev_add(dps_cdev, dps_device.dps_devno, MAX_DEVNO);
	if (ret < 0)
		printk(KERN_ERR"adding dps device to kernel failed\n");

	return ret;
}

/* entry point of the module. when 'insmod dps.ko' this function will be executed */
static int dps_init(void)
{
	int ret = 0, i = 0;

	/*ignore it this is a test field*/
	sprintf(dps_device.name,"%s", "AnilJacob");

	/*first we need to get some device number
	 * [root@compute dps]# ls -al /dev/dpslk0 
	 * crw-------. 1 root root 247, 0 Jul 19 18:36 /dev/dpslk0
	 * c=>char, 247-major number, 0- minor number. filesystem
	 * uses this number to identify the fileoperations from vfs.
	 * */
	ret = alloc_chrdev_region(&dps_device.dps_devno, 0, MAX_DEVNO, DEMOPPS_NAME);
	if (ret < 0){
		printk("Failed to allocate new device\n");
		goto errout_alloc;
	}

	/*our char device need to be registered to the kernel cdev_init, cdev_add will add
	 * it to the kernels char subsystem*/
	ret = setup_cdev(&dps_device.dps_cdev, &dps_ops);
	if (ret < 0){
		goto errout_cdev;
	}

	/*which class our device belongs to
	 * [root@compute dps]# ls -al /sys/class/dpsc/
	 * lrwxrwxrwx.  1 root root 0 Jul 20 09:32 dpslk0 -> ../../devices/virtual/dpsc/dpslk0
	 * */
	dps_device.devclass = class_create(THIS_MODULE, DEMOPPS_CLASS_NAME);
	if (IS_ERR(dps_device.devclass)){
		printk(KERN_INFO"Creating device class failed\n");
		ret = -2;
		goto errout_cc;
	}
	/*create the device node in /dev folder using udev.
	 *[root@compute dps]# ls -al /dev/dpsctl0 
	 *crw-------. 1 root root 247, 0 Jul 20 09:32 /dev/dpslk0
	 * */
	dps_device.device = device_create(dps_device.devclass, NULL, 
			dps_device.dps_devno, NULL, "dpsctl%d", i);
	if (!dps_device.device){
		printk(KERN_INFO"Creating dps device node failed\n");
		ret = -3;
		goto errout_dc;
	}

	init_waitqueue_head(&dps_device.waitq);

	setup_pps_timer();

	return 0;

/* if failed handle it appropriately*/
errout_dc:
	device_destroy(dps_device.devclass, dps_device.dps_devno);
	class_destroy(dps_device.devclass);
errout_cc:
	cdev_del(&dps_device.dps_cdev);
errout_cdev:
	unregister_chrdev_region(dps_device.dps_devno, MAX_DEVNO);
errout_alloc:
	return ret;
}

/* when you rmmod the device*/
static void dps_exit(void)
{
	/*delete the timer before leaving. _sync version to synchronize between multiprocessor*/
	del_timer_sync(&dps_device.pps_timer);
	device_destroy(dps_device.devclass, dps_device.dps_devno);
	class_destroy(dps_device.devclass);
	cdev_del(&dps_device.dps_cdev);
	unregister_chrdev_region(dps_device.dps_devno, MAX_DEVNO);
	return;
}

module_init(dps_init);
module_exit(dps_exit);

MODULE_AUTHOR("AJ");
MODULE_LICENSE("GPL");
